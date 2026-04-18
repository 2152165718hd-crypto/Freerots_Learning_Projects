#include "./SYSTEM/delay/delay.h"
#include "FreeRTOS.h"
#include "task.h"

static uint32_t s_cycles_per_us = 1U;
static uint8_t s_delay_ready = 0U;

static uint8_t delay_is_in_isr(void)
{
    return (__get_IPSR() != 0U) ? 1U : 0U;
}

void delay_init(void)
{
    /*
     * DWT->CYCCNT 是 Cortex-M 内核自带的 CPU 周期计数器。
     * 它和 SysTick、TIM7 都没有资源冲突：
     * - SysTick 交给 FreeRTOS 作为系统节拍；
     * - TIM7 交给 HAL 作为 HAL_GetTick/HAL_Delay 的 1 ms timebase；
     * - DWT 只用于很短的微秒级忙等，例如 LCD/传感器初始化时序。
     */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0U;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    s_cycles_per_us = SystemCoreClock / 1000000U;
    if (s_cycles_per_us == 0U)
    {
        s_cycles_per_us = 1U;
    }

    s_delay_ready = 1U;
}

void delay_us(uint32_t us)
{
    uint32_t start;
    uint32_t target_cycles;
    uint32_t max_us_per_round;
    uint32_t current_us;

    if (us == 0U)
    {
        return;
    }

    if (s_delay_ready == 0U)
    {
        delay_init();
    }

    max_us_per_round = 0xFFFFFFFFU / s_cycles_per_us;

    while (us > 0U)
    {
        current_us = us;
        if (current_us > max_us_per_round)
        {
            current_us = max_us_per_round;
        }

        /*
         * 这里使用无符号减法判断时间差，即使 CYCCNT 发生 32 位回绕，
         * 只要单轮等待时间没有超过 32 位计数器可表达的周期数，
         * 判断仍然成立。分段等待还能避免 us * cycles_per_us 乘法溢出。
         */
        target_cycles = current_us * s_cycles_per_us;
        start = DWT->CYCCNT;

        while ((uint32_t)(DWT->CYCCNT - start) < target_cycles)
        {
        }

        us -= current_us;
    }
}

void delay_ms(uint32_t ms)
{
    TickType_t ticks;

    if (ms == 0U)
    {
        return;
    }

    /*
     * 任务上下文中，毫秒级等待应让出 CPU，让其他任务运行。
     * 所以调度器已经运行时使用 vTaskDelay()，这才是 RTOS 程序的正确写法。
     */
    if ((delay_is_in_isr() == 0U) &&
        (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING))
    {
        ticks = pdMS_TO_TICKS(ms);
        if (ticks == 0U)
        {
            ticks = 1U;
        }

        vTaskDelay(ticks);
        return;
    }

    /*
     * 调度器启动前仍然允许 HAL_Delay()，此时 HAL tick 由 TIM7 提供。
     * 中断上下文不应该做毫秒级阻塞等待；如果误用，只允许 1 ms 以内
     * 的短忙等，避免长时间锁死中断。
     */
    if (delay_is_in_isr() != 0U)
    {
        if (ms <= 1U)
        {
            delay_us(ms * 1000U);
        }
        return;
    }

    HAL_Delay(ms);
}
