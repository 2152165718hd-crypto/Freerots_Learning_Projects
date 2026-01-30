#ifndef __TIM2_H
#define __TIM2_H

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"

extern TIM_HandleTypeDef htim2;

void MX_TIM2_Init(void);

#endif /* __TIM2_H */

