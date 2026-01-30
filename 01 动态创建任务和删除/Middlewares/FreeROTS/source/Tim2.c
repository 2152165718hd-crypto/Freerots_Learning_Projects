/**
 * @file    Tim2.c
 * @author  STM32 Development Team
 * @date    2026-01-29
 * @brief   TIM2定时器初始化和中断处理模块
 * 
 * @details
 *   - TIM2配置为1ms中断间隔
 *   - 优先级设置为15，不高于FreeRTOS内核优先级
 *   - 提供系统Tick时钟支持
 */

#include "./FreeROTS/source/Tim2.h"

/* TIM2句柄定义 */
TIM_HandleTypeDef htim2;

/**
 * @brief   错误处理函数
 * @param   void
 * @return  void
 * @note    系统发生不可恢复的错误时调用此函数
 */
void Error_Handler(void)
{
    /* 禁用所有中断，防止进一步的系统错误 */
    __disable_irq();

    /* 无限循环，等待系统复位 */
    while (1)
    {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
    }
}

/**
 * @brief   TIM2初始化函数
 * @param   void
 * @return  void
 * @note    
 *   - 启用TIM2时钟
 *   - 设置预分频器使计数器工作频率为1MHz
 *   - 设置自动重装载值使中断周期为1ms
 *   - 启用中断并配置优先级
 */
void MX_TIM2_Init(void)
{
    /* 启用TIM2时钟 */
    __HAL_RCC_TIM2_CLK_ENABLE();

    /* 初始化TIM2实例 */
    htim2.Instance = TIM2;
    
    /* 设置预分频器：使时钟频率分频至1MHz */
    htim2.Init.Prescaler = (SystemCoreClock / 1000000) - 1;
    
    /* 配置计数模式为向上计数 */
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    
    /* 设置自动重装载值：1000次计数 = 1ms中断周期 */
    htim2.Init.Period = 1000 - 1;
    
    /* 禁用自动重装载预加载 */
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    /* 初始化基础定时器 */
    if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
    {
        Error_Handler();
    }

    /* 启动定时器并使能中断 */
    if (HAL_TIM_Base_Start_IT(&htim2) != HAL_OK)
    {
        Error_Handler();
    }
    
    /* 设置中断优先级为15（不高于FreeRTOS内核优先级） */
    HAL_NVIC_SetPriority(TIM2_IRQn, 15, 0);
    
    /* 启用TIM2中断 */
    HAL_NVIC_EnableIRQ(TIM2_IRQn);
}

/**
 * @brief   TIM2中断服务程序（ISR）
 * @param   void
 * @return  void
 * @note    由硬件自动调用，处理TIM2中断事件
 */
void TIM2_IRQHandler(void)
{
    /* 调用HAL中断处理函数 */
    HAL_TIM_IRQHandler(&htim2);
}

/**
 * @brief   定时器周期中断回调函数
 * @param   htim: 指向TIM_HandleTypeDef结构体的指针
 * @return  void
 * @note    
 *   - 当定时器达到自动重装载值时调用
 *   - 负责更新系统Tick计数
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    /* 判断是否为TIM2的中断 */
    if (htim == &htim2)
    {
        /* 增加系统Tick计数 */
        HAL_IncTick();
    }
}

/**
 * @brief   HAL库Tick初始化函数
 * @param   TickPriority: 中断优先级（此处未使用）
 * @return  HAL_StatusTypeDef: 初始化状态
 * @note    
 *   - 告知HAL库由TIM2提供系统时基
 *   - 防止HAL库使用SysTick
 */
HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
    /* 避免编译器警告 */
    (void)TickPriority;
    
    /* 返回成功状态 */
    return HAL_OK;
}
