#include "./SYSTEM/sys/sys.h"

/*
 * STM32F407 当前工程的时钟目标：
 * - 外部晶振 HSE = 8 MHz；
 * - PLLM = 8，把 8 MHz 分频到 1 MHz 作为 PLL 输入；
 * - PLLN = 336，把 1 MHz 倍频到 336 MHz VCO；
 * - PLLP = 2，系统时钟 SYSCLK = 336 / 2 = 168 MHz；
 * - PLLQ = 7，USB/随机数等 48 MHz 域约为 336 / 7 = 48 MHz；
 * - AHB  = 168 MHz；
 * - APB1 = 42 MHz，APB1 定时器时钟 = 84 MHz；
 * - APB2 = 84 MHz，APB2 定时器时钟 = 168 MHz。
 *
 * 这个函数固定服务当前板级工程，不再暴露 plln/pllm/pllp/pllq 给应用层。
 * 移植到别的板子时，应先确认 HSE_VALUE 和目标主频，再修改这里。
 */
HAL_StatusTypeDef sys_clock_init_168mhz(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};
    HAL_StatusTypeDef status;

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState = RCC_HSE_ON;
    osc.PLL.PLLState = RCC_PLL_ON;
    osc.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLM = 8U;
    osc.PLL.PLLN = 336U;
    osc.PLL.PLLP = RCC_PLLP_DIV2;
    osc.PLL.PLLQ = 7U;

    status = HAL_RCC_OscConfig(&osc);
    if (status != HAL_OK)
    {
        return status;
    }

    clk.ClockType = RCC_CLOCKTYPE_SYSCLK |
                    RCC_CLOCKTYPE_HCLK |
                    RCC_CLOCKTYPE_PCLK1 |
                    RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV4;
    clk.APB2CLKDivider = RCC_HCLK_DIV2;

    status = HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_5);
    if (status != HAL_OK)
    {
        return status;
    }

    /*
     * HAL_RCC_ClockConfig 会更新 SystemCoreClock。
     * 这里再显式调用一次 CMSIS 更新函数，目的是让调试窗口、第三方代码
     * 和后续 delay_init() 看到的 SystemCoreClock 都保持一致。
     */
    SystemCoreClockUpdate();

    if (HAL_GetREVID() == 0x1001U)
    {
        __HAL_FLASH_PREFETCH_BUFFER_ENABLE();
    }

    return HAL_OK;
}

void sys_soft_reset(void)
{
    NVIC_SystemReset();
}

void sys_wfi(void)
{
    __WFI();
}

/*
 * 保存并关闭普通可屏蔽中断。
 *
 * 返回值是进入临界区之前的 PRIMASK。调用者退出临界区时必须把该值传给
 * sys_irq_restore()，这样才能正确处理“进入函数前本来就关中断”的嵌套场景。
 */
uint32_t sys_irq_save(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

void sys_irq_restore(uint32_t primask)
{
    __set_PRIMASK(primask);
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
    (void)file;
    (void)line;

    while (1)
    {
    }
}
#endif
