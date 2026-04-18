#include "FreeRTOS.h"
#include "task.h"

/*
 * 给 FreeRTOS 的 Idle 任务提供静态内存。
 *
 * 只要 configSUPPORT_STATIC_ALLOCATION = 1，FreeRTOS 就要求应用层提供这个函数。
 * Idle 任务是内核必须创建的任务，负责在没有其他任务运行时占住 CPU，
 * 也负责回收被删除的动态任务内存。
 *
 * 这里的 idle_tcb 和 idle_stack 必须是 static：
 * 函数返回以后，FreeRTOS 还会一直使用这两块内存。
 */
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t **ppxIdleTaskStackBuffer,
                                   uint32_t *pulIdleTaskStackSize)
{
    static StaticTask_t idle_tcb;
    static StackType_t idle_stack[configMINIMAL_STACK_SIZE];

    *ppxIdleTaskTCBBuffer = &idle_tcb;
    *ppxIdleTaskStackBuffer = idle_stack;
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

/*
 * 给 FreeRTOS 软件定时器服务任务提供静态内存。
 *
 * 本工程 configUSE_TIMERS = 1，所以 FreeRTOS 会创建一个 timer service task。
 * 当 configSUPPORT_STATIC_ALLOCATION = 1 时，这个任务的 TCB 和栈也需要由应用层提供。
 *
 * 如果以后把 configUSE_TIMERS 改成 0，这个函数就不再需要。
 */
void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
                                    StackType_t **ppxTimerTaskStackBuffer,
                                    uint32_t *pulTimerTaskStackSize)
{
    static StaticTask_t timer_tcb;
    static StackType_t timer_stack[configTIMER_TASK_STACK_DEPTH];

    *ppxTimerTaskTCBBuffer = &timer_tcb;
    *ppxTimerTaskStackBuffer = timer_stack;
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}
