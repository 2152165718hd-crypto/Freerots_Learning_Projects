#include "./Hardware/KEY/KEY.h"

#define KEY1_PIN GPIO_PIN_1
#define KEY2_PIN GPIO_PIN_11


void Key_Init(void)
{
    // 初始化按键GPIO
    __HAL_RCC_GPIOB_CLK_ENABLE(); // 使能GPIOB时钟

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = KEY1_PIN | KEY2_PIN; // 假设按键连接在PB1和PB11
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;    // 输入模式
    GPIO_InitStruct.Pull = GPIO_PULLUP;        // 拉上拉电阻
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

uint8_t Key_Scan(void)
{
    static uint8_t key1_last_state = 1; // 上次按键状态，1表示未按下
    static uint8_t key2_last_state = 1; // 上次按键状态，1表示未按下
    uint8_t key_num = 0;

    uint8_t key1_current_state = HAL_GPIO_ReadPin(GPIOB, KEY1_PIN);
    uint8_t key2_current_state = HAL_GPIO_ReadPin(GPIOB, KEY2_PIN);

    // 检测KEY1按键
    if (key1_last_state == 1 && key1_current_state == 0) // 按键按下
    {
        key_num = 1; // KEY1按下标志
    }
    key1_last_state = key1_current_state;

    // 检测KEY2按键
    if (key2_last_state == 1 && key2_current_state == 0) // 按键按下
    {
        key_num = 2; // KEY2按下标志
    }
    key2_last_state = key2_current_state;

    return key_num; // 返回按键状态码
}

