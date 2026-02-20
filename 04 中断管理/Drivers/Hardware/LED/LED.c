#include "./Hardware/LED/LED.h"



void LED_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = LED_PIN1 | LED_PIN2;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    /* Ensure LED is off to start (PC13 is typically active low on some boards) */
    HAL_GPIO_WritePin(GPIOA, LED_PIN1, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOA, LED_PIN2, GPIO_PIN_SET);
}

void LED_On(uint16_t GPIO_Pin)
{
    HAL_GPIO_WritePin(GPIOA, GPIO_Pin, GPIO_PIN_RESET); // Active low
}

void LED_Off(uint16_t GPIO_Pin)
{
    HAL_GPIO_WritePin(GPIOA, GPIO_Pin, GPIO_PIN_SET); // Active low
}

void LED_Toggle(uint16_t GPIO_Pin)
{
    HAL_GPIO_TogglePin(GPIOA, GPIO_Pin);
}
