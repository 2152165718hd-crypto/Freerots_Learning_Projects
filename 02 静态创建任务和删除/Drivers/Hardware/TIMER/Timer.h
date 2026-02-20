#ifndef __TIMER_H
#define __TIMER_H

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"

extern TIM_HandleTypeDef htim2;

void TIM2_Init(void);

#endif /* __TIMER_H */
