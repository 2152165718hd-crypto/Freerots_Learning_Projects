/**
 ****************************************************************************************************
 * @file        usart.c
 * @version     V1.1
 * @date        2023-06-05
 * @brief       STM32通用串口驱动模块（单串口版）
 *              实现串口初始化、底层收发驱动，重定义fputc函数支持printf串口打印，无需启用MicroLIB
 *              支持中断方式接收串口数据，实现基于回车换行的帧数据解析，适配操作系统中断处理
 * @attention   1. 依赖系统时钟配置，串口时钟需在系统时钟初始化后完成配置
 *              2. 串口引脚、编号通过宏定义配置，需在usart.h中修改对应宏
 *              3. 接收功能由USART_EN_RX宏控制，关闭后仅保留串口发送和printf功能
 ****************************************************************************************************
 * @version log
 * V1.0 20211103
 * 初始版本发布，实现串口初始化、printf支持、中断接收基础功能
 * V1.1 20230605
 * 1. 移除USART_UX_IRQHandler()函数中的冗余中断处理代码
 * 2. 优化HAL_UART_RxCpltCallback()接收回调函数逻辑，提升帧解析稳定性
 ****************************************************************************************************
 */

#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/usart/usart.h"

/* 条件编译：启用操作系统时，引入OS头文件 */
/* 条件编译：启用操作系统时，引入OS头文件 */
#if SYS_SUPPORT_OS
/* FreeRTOS下无需额外头文件（当前不调用任何FreeRTOS API） */
#endif

/******************************************************************************************/
/* 重定义底层函数，支持printf串口打印，无需启用MicroLIB库
 * 核心：重定义fputc函数，将printf的输出重定向到串口硬件
 * 适配MDK-AC5/AC6编译器，关闭半主机模式避免调试依赖
 ******************************************************************************************/
#if 1

#if (__ARMCC_VERSION >= 6010050)            /* 适配MDK-AC6编译器 */
__asm(".global __use_no_semihosting\n\t");  /* 声明不使用半主机模式，脱离调试器运行 */
__asm(".global __ARM_use_no_argv \n\t");    /* AC6编译器需声明main无参数，避免半主机模式启动 */

#else                                       /* 适配MDK-AC5编译器 */
/* 声明不使用半主机模式，需同时定义__FILE结构体 */
#pragma import(__use_no_semihosting)

/* 定义__FILE结构体，AC5编译器关闭半主机模式的必要声明 */
struct __FILE
{
    int handle;
    /* 仅使用printf串口打印时，无需实现复杂的文件处理逻辑 */
};

#endif

/**
 * @brief  关闭半主机模式的必要函数重定义
 * @note   编译器要求：关闭半主机模式时，必须重定义_ttywrch/_sys_exit/_sys_command_string
 *         避免编译器报未定义错误，适配AC5/AC6所有版本
 */
int _ttywrch(int ch)
{
    ch = ch;  /* 空实现，仅满足编译器语法要求 */
    return ch;
}

/**
 * @brief  重定义系统退出函数，关闭半主机模式使用
 * @param  x: 退出码（未使用，仅满足语法要求）
 * @retval 无
 */
void _sys_exit(int x)
{
    x = x;  /* 空实现，避免程序退出时调用半主机模式接口 */
}

/**
 * @brief  重定义系统命令字符串函数，关闭半主机模式使用
 * @param  cmd: 命令缓冲区（未使用）
 * @param  len: 缓冲区长度（未使用）
 * @retval NULL: 固定返回空，满足编译器语法要求
 */
char *_sys_command_string(char *cmd, int len)
{
    return NULL;
}

/* 定义标准输出文件句柄，关联printf到串口输出（FILE在stdio.h中声明） */
FILE __stdout;

/**
 * @brief  重定义fputc函数，实现printf串口打印底层驱动
 * @note   MDK编译器中，printf函数最终会调用fputc输出字符
 *         本函数将字符逐个发送到串口，完成printf重定向
 * @param  ch: 要发送的字符（ASCII码）
 * @param  f: 文件句柄（此处为__stdout，未实际使用）
 * @retval ch: 发送成功的字符
 */
int fputc(int ch, FILE *f)
{
    while ((USART_UX->SR & 0X40) == 0);     /* 等待串口发送完成（TXE位置1） */
    USART_UX->DR = (uint8_t)ch;             /* 将字符写入串口数据寄存器，触发发送 */
    return ch;
}
#endif
/******************************************************************************************/

/* 条件编译：是否使能串口接收功能（由USART_EN_RX宏控制，在usart.h中定义） */
#if USART_EN_RX

/* 串口接收缓冲区，最大存储USART_REC_LEN个字节的帧数据 */
uint8_t g_usart_rx_buf[USART_REC_LEN];

/**
 * 串口接收状态寄存器（16位），各位功能定义：
 * bit15：帧接收完成标志位（1=接收完成，0=未完成）
 * bit14：接收到回车符标志位（0x0d，1=已接收，0=未接收）
 * bit13~0：已接收有效字节数计数器（0~USART_REC_LEN-1，超出则重置）
 */
uint16_t g_usart_rx_sta = 0;

uint8_t g_rx_buffer[RXBUFFERSIZE];  /* HAL库中断接收临时缓冲区，每次接收1个字节 */
UART_HandleTypeDef g_uart1_handle;  /* UART外设句柄，HAL库操作串口的核心结构体 */

/**
 * @brief       串口初始化函数
 * @note        1. 基于HAL库配置串口基本参数，支持收发模式
 *              2. 初始化完成后自动开启串口接收中断，触发第一次接收
 *              3. 串口波特率精度依赖系统时钟配置，需保证串口时钟源正确
 * @param       baudrate: 串口波特率（如9600、115200、38400等）
 * @retval      无
 */
void usart_init(uint32_t baudrate)
{
    /* UART外设参数初始化配置 */
    g_uart1_handle.Instance = USART_UX;                                       /* 绑定要初始化的串口外设（USART_UX） */
    g_uart1_handle.Init.BaudRate = baudrate;                                  /* 配置串口波特率 */
    g_uart1_handle.Init.WordLength = UART_WORDLENGTH_8B;                      /* 数据位：8位 */
    g_uart1_handle.Init.StopBits = UART_STOPBITS_1;                           /* 停止位：1位 */
    g_uart1_handle.Init.Parity = UART_PARITY_NONE;                            /* 校验位：无 */
    g_uart1_handle.Init.HwFlowCtl = UART_HWCONTROL_NONE;                      /* 硬件流控：关闭 */
    g_uart1_handle.Init.Mode = UART_MODE_TX_RX;                               /* 工作模式：收发一体 */
    HAL_UART_Init(&g_uart1_handle);                                           /* 调用HAL库完成串口底层初始化 */

    /* 开启串口中断接收，接收1个字节后触发中断，回调HAL_UART_RxCpltCallback */
    HAL_UART_Receive_IT(&g_uart1_handle, (uint8_t *)g_rx_buffer, RXBUFFERSIZE);
}

/**
 * @brief       UART外设MSP初始化函数（底层硬件初始化）
 * @note        1. 由HAL_UART_Init()自动调用，无需手动调用
 *              2. 实现串口对应的GPIO、时钟、中断的底层配置
 *              3. 所有硬件相关配置均通过宏定义实现，便于移植
 * @param       huart: UART外设句柄指针
 * @retval      无
 */
void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef gpio_init_struct;  /* GPIO初始化结构体 */

    if (huart->Instance == USART_UX)                            /* 仅初始化指定的串口外设 */
    {
        USART_TX_GPIO_CLK_ENABLE();                             /* 使能串口TX引脚对应的GPIO时钟 */
        USART_RX_GPIO_CLK_ENABLE();                             /* 使能串口RX引脚对应的GPIO时钟 */
        USART_UX_CLK_ENABLE();                                  /* 使能串口外设本身的时钟 */

        /* 配置TX引脚：复用推挽输出 */
        gpio_init_struct.Pin = USART_TX_GPIO_PIN;               /* 绑定TX引脚 */
        gpio_init_struct.Mode = GPIO_MODE_AF_PP;                /* 模式：复用推挽输出 */
        gpio_init_struct.Pull = GPIO_PULLUP;                    /* 上拉使能，避免电平飘移 */
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;          /* 引脚速度：高速 */
        HAL_GPIO_Init(USART_TX_GPIO_PORT, &gpio_init_struct);
                
        /* 配置RX引脚：复用输入 */
        gpio_init_struct.Pin = USART_RX_GPIO_PIN;               /* 绑定RX引脚 */
        gpio_init_struct.Mode = GPIO_MODE_AF_INPUT;             /* 模式：复用输入 */
        HAL_GPIO_Init(USART_RX_GPIO_PORT, &gpio_init_struct);   /* 初始化RX引脚 */
        
#if USART_EN_RX
        HAL_NVIC_EnableIRQ(USART_UX_IRQn);                      /* 使能串口对应的中断通道 */
        HAL_NVIC_SetPriority(USART_UX_IRQn, 3, 3);              /* 配置中断优先级：抢占优先级3，子优先级3 */
#endif
    }
}

/**
 * @brief       串口接收完成中断回调函数
 * @note        1. 由HAL_UART_IRQHandler()自动调用，每次接收1个字节触发
 *              2. 实现基于「回车(0x0d)+换行(0x0a)」的帧数据解析
 *              3. 接收完成后自动重新开启中断，实现连续接收
 * @param       huart: UART外设句柄指针
 * @retval      无
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART_UX)                    /* 仅处理指定串口的接收中断 */
    {
        if ((g_usart_rx_sta & 0x8000) == 0)             /* 帧接收未完成时处理数据 */
        {
            if (g_usart_rx_sta & 0x4000)                /* 已接收到回车符(0x0d) */
            {
                if (g_rx_buffer[0] != 0x0a)             /* 未接收到换行符(0x0a)，判定为接收错误 */
                {
                    g_usart_rx_sta = 0;                 /* 重置接收状态，重新开始接收 */
                }
                else                                    /* 接收到换行符(0x0a)，帧接收完成 */
                {
                    g_usart_rx_sta |= 0x8000;           /* 置位接收完成标志位(bit15) */
                }
            }
            else                                        /* 未接收到回车符(0x0d)，继续接收数据 */
            {
                if (g_rx_buffer[0] == 0x0d)             /* 接收到回车符(0x0d)，置位对应标志位 */
                    g_usart_rx_sta |= 0x4000;
                else                                    /* 接收到普通数据，存入接收缓冲区 */
                {
                    g_usart_rx_buf[g_usart_rx_sta & 0X3FFF] = g_rx_buffer[0];
                    g_usart_rx_sta++;                    /* 有效字节数计数器+1 */

                    /* 超出缓冲区最大长度，重置接收状态 */
                    if (g_usart_rx_sta > (USART_REC_LEN - 1))
                    {
                        g_usart_rx_sta = 0;             /* 接收溢出，重新开始接收 */
                    }
                }
            }
        }
        /* 重新开启串口中断接收，实现连续数据接收 */
        HAL_UART_Receive_IT(&g_uart1_handle, (uint8_t *)g_rx_buffer, RXBUFFERSIZE);
    }
}

/**
 * @brief       串口外设中断服务函数
 * @note        1. 串口中断的入口函数，由中断向量表跳转执行
 *              2. 启用OS时，需先进入/退出中断嵌套，保证OS任务调度正常
 *              3. 底层逻辑由HAL库函数HAL_UART_IRQHandler()处理，本函数仅做封装
 * @param       无
 * @retval      无
 */
/**
 * @brief       串口外设中断服务函数
 * @note        1. 串口中断的入口函数，由中断向量表跳转执行
 *              2. FreeRTOS下HAL_UART_IRQHandler不调用任何FreeRTOS API，无需OSIntEnter/Exit
 *              3. 底层逻辑由HAL库函数HAL_UART_IRQHandler()处理
 * @param       无
 * @retval      无
 */
void USART_UX_IRQHandler(void)
{
    HAL_UART_IRQHandler(&g_uart1_handle);   /* 直接调用HAL库中断处理函数 */
}
#endif

