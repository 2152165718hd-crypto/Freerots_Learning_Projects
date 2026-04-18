#ifndef SYS_H
#define SYS_H

#include <stdint.h>
#include "stm32f4xx_hal.h"

/*
 * SYSTEM 模块只放“芯片级公共能力”：
 * 1. 系统时钟树配置；
 * 2. 复位、休眠、全局中断控制；
 * 3. 可恢复临界区。
 *
 * 这里不再保留社区代码里的 RTOS 适配宏和第三方内核钩子。
 * 本项目已经使用 FreeRTOS，中断进入/退出不需要应用层手工通知内核；
 * 真正需要和内核交互的中断，应在具体驱动里使用 xxxFromISR API。
 */

#ifdef __cplusplus
extern "C" {
#endif

HAL_StatusTypeDef sys_clock_init_168mhz(void);
void sys_soft_reset(void);
void sys_wfi(void);
uint32_t sys_irq_save(void);
void sys_irq_restore(uint32_t primask);

#ifdef __cplusplus
}
#endif

#endif
