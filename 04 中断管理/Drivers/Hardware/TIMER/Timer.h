#ifndef __TIMER_H
#define __TIMER_H

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "./SYSTEM/usart/usart.h"
#include "./Hardware/LED/LED.h"

void TIM2_Init(void);
void TIM3_Init(void);
void TIM4_Init(void);

#endif /* __TIMER_H */
