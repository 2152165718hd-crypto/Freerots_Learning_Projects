#ifndef DELAY_H
#define DELAY_H

#include <stdint.h>
#include "./SYSTEM/sys/sys.h"

#ifdef __cplusplus
extern "C" {
#endif

void delay_init(void);
void delay_us(uint32_t us);
void delay_ms(uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif
