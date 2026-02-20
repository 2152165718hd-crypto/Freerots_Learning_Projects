/*
 * FreeRTOS V202212.01
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/*-----------------------------------------------------------
 * Application specific definitions.
 *
 * These definitions should be adjusted for your particular hardware and
 * application requirements.
 *
 * THESE PARAMETERS ARE DESCRIBED WITHIN THE 'CONFIGURATION' SECTION OF THE
 * FreeRTOS API DOCUMENTATION AVAILABLE ON THE FreeRTOS.org WEB SITE.
 *
 * See http://www.freertos.org/a00110.html
 *----------------------------------------------------------*/

#define configUSE_PREEMPTION 1                         /* 启用抢占式调度 */
#define configUSE_IDLE_HOOK 0                          /* 禁用空闲任务钩子函数 */
#define configUSE_TICK_HOOK 0                          /* 禁用时钟节拍钩子函数 */
#define configCPU_CLOCK_HZ ((unsigned long)72000000)   /* CPU频率，72MHz */
#define configTICK_RATE_HZ ((TickType_t)1000)          /* 系统节拍频率，1000Hz(1ms) */
#define configMAX_PRIORITIES (5)                       /* 最大任务优先级数量 */
#define configMINIMAL_STACK_SIZE ((unsigned short)128) /* 最小任务堆栈大小，单位字 */
#define configTOTAL_HEAP_SIZE ((size_t)(17 * 1024))    /* 堆内存总大小，17KB */
#define configMAX_TASK_NAME_LEN (16)                   /* 任务名称最大长度 */
#define configUSE_16_BIT_TICKS 0                       /* 使用32位时钟计数 */
#define configIDLE_SHOULD_YIELD 1                      /* 空闲任务主动让出CPU */

/* Set the following definitions to 1 to include the API function, or zero
to exclude the API function. */

#define INCLUDE_vTaskPrioritySet 1      /* 包含设置任务优先级API */
#define INCLUDE_uxTaskPriorityGet 1     /* 包含获取任务优先级API */
#define INCLUDE_vTaskDelete 1           /* 包含删除任务API */
#define INCLUDE_vTaskCleanUpResources 0 /* 不包含任务清理资源API */
#define INCLUDE_vTaskSuspend 1          /* 包含任务挂起/恢复API */
#define INCLUDE_vTaskDelayUntil 1       /* 包含绝对延时API */
#define INCLUDE_vTaskDelay 1            /* 包含相对延时API */

/* This is the raw value as per the Cortex-M3 NVIC.  Values can be 255
(lowest) to 0 (1?) (highest). */
#define configKERNEL_INTERRUPT_PRIORITY 255 /* 内核中断优先级（最低） */
/* !!!! configMAX_SYSCALL_INTERRUPT_PRIORITY must not be set to zero !!!!
See http://www.FreeRTOS.org/RTOS-Cortex-M3-M4.html. */
#define configMAX_SYSCALL_INTERRUPT_PRIORITY 191 /* 系统调用最高中断优先级，优先级11 */

/* This is the value being used as per the ST library which permits 16
priority values, 0 to 15.  This must correspond to the
configKERNEL_INTERRUPT_PRIORITY setting.  Here 15 corresponds to the lowest
NVIC value of 255. */
#define configLIBRARY_KERNEL_INTERRUPT_PRIORITY 15 /* ST库使用的内核中断优先级值 */

#define xPortPendSVHandler PendSV_Handler /* 映射PendSV中断处理函数 */
#define vPortSVCHandler SVC_Handler       /* 映射SVC中断处理函数 */
#define INCLUDE_xTaskGetSchedulerState 1  /* 包含获取调度器状态API */

#define INCLUDE_vTaskSuspend 1   /* 包含任务挂起/恢复API */
#define INCLUDE_xResumeFromISR 1 /* 包含中断中恢复任务API */

/* 开启跟踪task信息 */
#define configUSE_TRACE_FACILITY 1
#define configUSE_STATS_FORMATTING_FUNCTIONS 1

#endif /* FREERTOS_CONFIG_H */
