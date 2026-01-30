/**
 ****************************************************************************************************
 * @file        usart.h
 * @version     V1.1
 * @date        2023-06-05
 * @brief       STM32通用串口驱动模块头文件（单串口版）
 *              配套usart.c使用，定义串口硬件相关宏、接收配置宏，声明串口初始化函数及全局变量
 *              实现串口硬件参数的宏定义化，便于移植和修改，支持printf重定向、中断接收功能配置
 * @attention   1. 串口引脚、外设编号、时钟使能均通过宏定义实现，修改宏即可适配不同串口/引脚
 *              2. 接收功能由USART_EN_RX宏控制，关闭后仅保留发送和printf功能
 *              3. 全局变量供上层业务调用，用于获取串口接收数据和接收状态
 ****************************************************************************************************
 * @version log
 * V1.0 20211103
 * 初始版本发布，定义串口基础宏、全局变量，声明串口初始化函数
 * V1.1 20230605
 * 1. 移除冗余的中断服务函数声明注释，简化头文件内容
 * 2. 优化宏定义结构，增强串口硬件参数的可移植性
 ****************************************************************************************************
 */

#ifndef __USART_H
#define __USART_H

#include "stdio.h"
#include "./SYSTEM/sys/sys.h"

/******************************************************************************************/
/* 串口硬件相关宏定义 - 全宏定义化，修改此处即可适配不同串口/引脚，默认适配USART1/PA9-TX/PA10-RX
 * 包含：TX/RX引脚、GPIO时钟、串口外设、中断通道、串口时钟等底层硬件配置
 ******************************************************************************************/
/* 串口TX引脚配置：GPIO口、引脚号、时钟使能宏 */
#define USART_TX_GPIO_PORT                  GPIOA
#define USART_TX_GPIO_PIN                   GPIO_PIN_9
#define USART_TX_GPIO_CLK_ENABLE()          do{ __HAL_RCC_GPIOA_CLK_ENABLE(); }while(0)   /* PA口时钟使能，do-while避免宏展开语法错误 */

/* 串口RX引脚配置：GPIO口、引脚号、时钟使能宏 */
#define USART_RX_GPIO_PORT                  GPIOA
#define USART_RX_GPIO_PIN                   GPIO_PIN_10
#define USART_RX_GPIO_CLK_ENABLE()          do{ __HAL_RCC_GPIOA_CLK_ENABLE(); }while(0)   /* PA口时钟使能，do-while避免宏展开语法错误 */

/* 串口外设核心配置：外设编号、中断号、中断服务函数、外设时钟使能宏 */
#define USART_UX                            USART1
#define USART_UX_IRQn                       USART1_IRQn
#define USART_UX_IRQHandler                 USART1_IRQHandler
#define USART_UX_CLK_ENABLE()               do{ __HAL_RCC_USART1_CLK_ENABLE(); }while(0)  /* USART1外设时钟使能，do-while避免宏展开语法错误 */

/******************************************************************************************/
/* 串口接收功能配置宏 - 控制接收缓冲区大小、是否使能接收、临时缓冲区大小 */
#define USART_REC_LEN               200         /* 串口最大接收字节数，可根据业务需求修改 */
#define USART_EN_RX                 1           /* 串口接收使能开关：1=使能中断接收，0=关闭接收仅保留发送 */
#define RXBUFFERSIZE                1           /* HAL库中断接收临时缓冲区大小，固定为1（每次接收1个字节） */

/******************************************************************************************/
/* 全局变量声明 - 供usart.c底层驱动和上层业务代码共同调用 */
extern UART_HandleTypeDef g_uart1_handle;       /* HAL库UART核心句柄，用于HAL库串口相关函数调用 */
extern uint8_t  g_usart_rx_buf[USART_REC_LEN];  /* 串口接收数据缓冲区，存储接收到的帧数据（最大USART_REC_LEN字节） */
extern uint16_t g_usart_rx_sta;                 /* 串口接收状态寄存器，16位变量，各位对应不同接收状态（详见usart.c注释） */
extern uint8_t  g_rx_buffer[RXBUFFERSIZE];      /* HAL库中断接收临时缓冲区，仅用于底层中断接收，每次存1个字节 */

/******************************************************************************************/
/* 函数声明 - 串口驱动对外提供的核心接口 */
/**
 * @brief  串口初始化函数声明
 * @note   基于HAL库配置串口基本参数，初始化完成后自动开启接收中断（若USART_EN_RX=1）
 * @param  bound: 串口波特率，支持常见波特率（如9600、115200、38400等）
 * @retval 无
 * @attention 需在系统时钟初始化完成后调用，保证波特率配置精度
 */
void usart_init(uint32_t bound);

#endif

