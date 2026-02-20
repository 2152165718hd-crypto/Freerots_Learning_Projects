#ifndef FREERTOS_DEMO_H
#define FREERTOS_DEMO_H

#include ".\Hardware\TIMER\Timer.h"
#include "./SYSTEM/usart/usart.h"
#include "./Hardware/LED/LED.h"
#include "./Hardware/OLED/OLED.h"
#include "./Hardware/KEY/KEY.h"
#include "./SYSTEM/delay/delay.h"
#include "stm32f1xx_hal.h"

void FreeROTS_Start(void);

#endif
