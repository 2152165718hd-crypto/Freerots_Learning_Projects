#include "stm32f4xx_hal.h"

static TIM_HandleTypeDef htim7;

HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
    RCC_ClkInitTypeDef clkconfig;
    uint32_t flash_latency;
    uint32_t apb1_prescaler;
    uint32_t tim_clock;
    uint32_t prescaler;
    HAL_StatusTypeDef status;

    if (TickPriority >= (1UL << __NVIC_PRIO_BITS))
    {
        return HAL_ERROR;
    }

    __HAL_RCC_TIM7_CLK_ENABLE();

    HAL_RCC_GetClockConfig(&clkconfig, &flash_latency);
    apb1_prescaler = clkconfig.APB1CLKDivider;

    if (apb1_prescaler == RCC_HCLK_DIV1)
    {
        tim_clock = HAL_RCC_GetPCLK1Freq();
    }
    else
    {
        tim_clock = 2U * HAL_RCC_GetPCLK1Freq();
    }

    prescaler = (tim_clock / 1000000U) - 1U;

    htim7.Instance = TIM7;
    htim7.Init.Period = 1000U - 1U;
    htim7.Init.Prescaler = prescaler;
    htim7.Init.ClockDivision = 0U;
    htim7.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim7.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    status = HAL_TIM_Base_Init(&htim7);
    if (status == HAL_OK)
    {
        HAL_NVIC_SetPriority(TIM7_IRQn, TickPriority, 0U);
        HAL_NVIC_EnableIRQ(TIM7_IRQn);
        uwTickPrio = TickPriority;

        status = HAL_TIM_Base_Start_IT(&htim7);
    }

    return status;
}

void HAL_SuspendTick(void)
{
    __HAL_TIM_DISABLE_IT(&htim7, TIM_IT_UPDATE);
}

void HAL_ResumeTick(void)
{
    __HAL_TIM_ENABLE_IT(&htim7, TIM_IT_UPDATE);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM7)
    {
        HAL_IncTick();
    }
}

void TIM7_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&htim7);
}
