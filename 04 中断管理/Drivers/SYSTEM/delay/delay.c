/**
 ****************************************************************************************************
 * @file        delay.c
 * @version     V1.2 (FreeRTOS适配版)
 * @date        2026-01-29
 * @brief       基于STM32 SysTick定时器实现的微秒/毫秒延时驱动模块
 *              支持FreeRTOS操作系统适配，同时重定义HAL库延时函数HAL_Delay
 *              HAL库使用TIM2作为tick源，SysTick完全交给FreeRTOS使用
 ****************************************************************************************************
 */

#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/delay/delay.h"

#if SYS_SUPPORT_OS
#include "FreeRTOS.h"
#include "task.h"
#include "portmacro.h"      // 必须包含，提供portENTER_CRITICAL等宏
extern void xPortSysTickHandler(void);
#endif

/* 微秒延时的倍频系数 */
static uint32_t g_fac_us = 0;

#if SYS_SUPPORT_OS
/* 每个tick对应的毫秒数（仅OS时使用） */
static uint16_t g_fac_ms = 0;
#endif

#if SYS_SUPPORT_OS
/**
 * @brief  SysTick中断服务函数（FreeRTOS专用）
 * @note   只调用FreeRTOS的SysTick处理函数
 *         不调用HAL_IncTick()，因为HAL使用TIM2作为时间基准
 */
void SysTick_Handler(void)
{
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
    {
        xPortSysTickHandler();
    }
}
#endif

/**
 * @brief  延时模块初始化函数
 * @param  sysclk: 系统核心时钟频率（MHz）
 * @retval 无
 */
void delay_init(uint16_t sysclk)
{
    g_fac_us = sysclk;                              // 微秒系数 = 主频MHz

#if SYS_SUPPORT_OS
    uint32_t reload;
    reload = sysclk * (1000000UL / configTICK_RATE_HZ);   // 每tick的计数周期
    g_fac_ms = 1000 / configTICK_RATE_HZ;                 // 每tick多少ms

    SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;           // 使能中断
    SysTick->LOAD  = reload - 1UL;                       // 重装值（-1确保精确）
    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;            // 开启SysTick
#endif
}

/**
 * @brief  微秒级延时函数
 * @param  nus: 延时微秒数
 * @retval 无
 */
void delay_us(uint32_t nus)
{
    uint32_t ticks;
    uint32_t told, tnow, tcnt = 0;
    uint32_t reload = SysTick->LOAD;

    ticks = nus * g_fac_us;                         // 需要计数的tick数

#if SYS_SUPPORT_OS
    portENTER_CRITICAL();                           // 进入临界区（禁用中断，确保精度）
#endif

    told = SysTick->VAL;
    while (1)
    {
        tnow = SysTick->VAL;
        if (tnow != told)
        {
            if (tnow < told)
                tcnt += told - tnow;
            else
                tcnt += reload - tnow + told;
            told = tnow;
            if (tcnt >= ticks)
                break;
        }
    }

#if SYS_SUPPORT_OS
    portEXIT_CRITICAL();                            // 退出临界区
#endif
}

/**
 * @brief  毫秒级延时函数
 * @param  nms: 延时毫秒数
 * @retval 无
 */
void delay_ms(uint16_t nms)
{
#if SYS_SUPPORT_OS
    /* 在任务中且调度器已运行时，使用非阻塞vTaskDelay */
    if ((__get_IPSR() == 0) && (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING))
    {
        if (nms >= g_fac_ms)                        // 大于等于1个tick
        {
            vTaskDelay(pdMS_TO_TICKS(nms));         // FreeRTOS非阻塞延时
        }
        nms %= g_fac_ms;                            // 剩余不足1tick的部分用忙等待
    }
#endif

    /* 剩余时间或中断中：使用忙等待 */
    if (nms > 0)
    {
        delay_us((uint32_t)nms * 1000);
    }
}

/**
 * @brief  重定义HAL_Delay（与HAL库统一）
 */
void HAL_Delay(uint32_t Delay)
{
    delay_ms((uint16_t)Delay);
}

