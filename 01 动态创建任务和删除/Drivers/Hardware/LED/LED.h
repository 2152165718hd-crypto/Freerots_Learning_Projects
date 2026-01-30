#ifndef __LED_H__
#define __LED_H__

#include "stm32f1xx_hal.h"
#define LED_PIN1 GPIO_PIN_0
#define LED_PIN2 GPIO_PIN_1

void LED_Init(void);
void LED_On(uint16_t GPIO_Pin);
void LED_Off(uint16_t GPIO_Pin);
void LED_Toggle(uint16_t GPIO_Pin);

#endif
