/**
 ****************************************************************************************************
 * @file        sys.c
 * @version     V1.0
 * @date        2020-04-17
 * @brief       STM32系统核心功能驱动模块
 *              实现系统时钟初始化、中断向量表重映射、总中断控制、低功耗模式、软复位等核心功能
 *              基于HAL库开发，适配STM32F103系列芯片，为上层模块提供基础的系统硬件操作接口
 ****************************************************************************************************
 * @version log
 * V1.0 20211103
 * 初始版本发布，实现系统时钟、中断、低功耗、复位等核心功能的基础驱动
 ****************************************************************************************************
 */

#include "./SYSTEM/sys/sys.h"

/**
 * @brief       重映射NVIC中断向量表
 * @note        STM32中断向量表默认在Flash起始地址，该函数用于将其重映射到RAM或Flash其他区域
 *              向量表偏移地址需满足256字节对齐要求，由VTOR寄存器的位限制决定
 * @param       baseaddr: 中断向量表基地址（Flash起始/RAM起始等）
 * @param       offset: 向量表偏移量（单位：字节，必须为0x200的整数倍，即256字节对齐）
 * @retval      无
 */
void sys_nvic_set_vector_table(uint32_t baseaddr, uint32_t offset)
{
    /* 配置NVIC向量表寄存器VTOR，bit[8:0]保留，偏移量需256字节对齐 */
    SCB->VTOR = baseaddr | (offset & (uint32_t)0xFFFFFE00);
}

/**
 * @brief       使系统进入WFI睡眠模式
 * @note        执行WFI(Wait For Interrupt)指令，系统进入低功耗睡眠状态，等待任意中断唤醒
 *              唤醒后继续执行WFI指令后的代码，适合空闲时降低功耗
 * @param       无
 * @retval      无
 */
void sys_wfi_set(void)
{
    __ASM volatile("wfi");  /* 内嵌汇编执行WFI指令 */
}

/**
 * @brief       关闭全局中断（不含Fault和NMI中断）
 * @note        执行CPSID I汇编指令，屏蔽普通中断，FAULT、NMI等不可屏蔽中断仍生效
 *              用于临界区保护，防止关键代码被中断打断
 * @param       无
 * @retval      无
 */
void sys_intx_disable(void)
{
    __ASM volatile("cpsid i");  /* 内嵌汇编关闭全局中断 */
}

/**
 * @brief       开启全局中断
 * @note        执行CPSIE I汇编指令，恢复普通中断的响应，与sys_intx_disable配对使用
 *              临界区代码执行完成后需调用，恢复系统中断响应
 * @param       无
 * @retval      无
 */
void sys_intx_enable(void)
{
    __ASM volatile("cpsie i");  /* 内嵌汇编开启全局中断 */
}

/**
 * @brief       设置系统主栈指针(MSP)
 * @note        直接修改CPU的主栈指针，适用于MDK环境下的系统启动、程序重定位等场景
 *              栈指针地址需指向合法的RAM区域，避免栈溢出
 * @param       addr: 主栈指针的目标地址（RAM区域合法地址）
 * @retval      无
 */
void sys_msr_msp(uint32_t addr)
{
    __set_MSP(addr);    /* 调用库函数设置主栈指针MSP */
}

/**
 * @brief       使系统进入待机(Standby)模式
 * @note        待机模式是STM32深度低功耗模式，除备份域外所有外设断电，仅能通过WKUP引脚/复位唤醒
 *              唤醒后系统重新启动，而非继续执行原代码，功耗远低于睡眠模式
 * @param       无
 * @retval      无
 */
void sys_standby(void)
{
    __HAL_RCC_PWR_CLK_ENABLE();    /* 使能电源管理(PWR)外设时钟 */
    SET_BIT(PWR->CR, PWR_CR_PDDS); /* 设置PWR_CR寄存器PDDS位，配置进入待机模式 */
}

/**
 * @brief       系统软件复位
 * @note        调用NVIC系统复位函数，触发芯片软复位，效果与硬件复位一致
 *              复位后系统从启动代码开始执行，用于系统异常时的自恢复
 * @param       无
 * @retval      无
 */
void sys_soft_reset(void)
{
    NVIC_SystemReset(); /* 调用库函数执行系统软件复位 */
}

/**
 * @brief       STM32F103系统时钟初始化函数
 * @note        1. 基于外部高速晶振(HSE)配置PLL倍频，作为系统核心时钟
 *              2. 系统启动时已由SystemInit()初始化时钟，本函数用于重配置自定义主频
 *              3. 初始化失败会进入死循环，需检查HSE晶振和参数配置
 * @param       plln: PLL倍频系数，取值范围：2~16（STM32F103限定）
 *              例：plln=9时，8MHz HSE经PLL倍频后为72MHz，为F103最大主频
 * @retval      无
 */
void sys_stm32_clock_init(uint32_t plln)
{
    HAL_StatusTypeDef ret = HAL_ERROR;
    RCC_OscInitTypeDef rcc_osc_init = {0};  /* 振荡器初始化结构体，初始化为0 */
    RCC_ClkInitTypeDef rcc_clk_init = {0};  /* 时钟树初始化结构体，初始化为0 */

    /* 配置振荡器：HSE开启 + PLL倍频 */
    rcc_osc_init.OscillatorType = RCC_OSCILLATORTYPE_HSE;       /* 选择振荡器类型为HSE（外部高速晶振） */
    rcc_osc_init.HSEState = RCC_HSE_ON;                         /* 使能HSE */
    rcc_osc_init.HSEPredivValue = RCC_HSE_PREDIV_DIV1;          /* HSE预分频系数：1分频（不分频） */
    rcc_osc_init.PLL.PLLState = RCC_PLL_ON;                     /* 使能PLL锁相环 */
    rcc_osc_init.PLL.PLLSource = RCC_PLLSOURCE_HSE;             /* PLL时钟源选择HSE */
    rcc_osc_init.PLL.PLLMUL = plln;                             /* 设置PLL倍频系数 */
    ret = HAL_RCC_OscConfig(&rcc_osc_init);                     /* 调用HAL库初始化振荡器 */

    if (ret != HAL_OK)
    {
        while (1);  /* 振荡器初始化失败，进入死循环（可在此添加错误处理） */
    }

    /* 配置时钟树：PLL作为系统时钟，配置AHB/APB1/APB2分频 */
    rcc_clk_init.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
    rcc_clk_init.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;        /* 系统时钟(SYSCLK)源选择PLL输出 */
    rcc_clk_init.AHBCLKDivider = RCC_SYSCLK_DIV1;               /* AHB总线时钟分频：1分频（与SYSCLK同频） */
    rcc_clk_init.APB1CLKDivider = RCC_HCLK_DIV2;                /* APB1总线时钟分频：2分频（最大42MHz） */
    rcc_clk_init.APB2CLKDivider = RCC_HCLK_DIV1;                /* APB2总线时钟分频：1分频（最大72MHz） */
    /* 配置时钟树并设置FLASH延时：2个等待周期(2WS)，适配72MHz主频 */
    ret = HAL_RCC_ClockConfig(&rcc_clk_init, FLASH_LATENCY_2);  

    if (ret != HAL_OK)
    {
        while (1);  /* 时钟树初始化失败，进入死循环（可在此添加错误处理） */
    }
}

