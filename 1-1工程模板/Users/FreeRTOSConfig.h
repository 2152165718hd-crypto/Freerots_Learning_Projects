/*
 * FreeRTOS V202212.01
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * FreeRTOSConfig.h 是工程级配置文件。
 * 这里的宏决定调度方式、Tick 频率、内存管理、中断优先级和可用 API。
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#ifndef __ASSEMBLER__
    #include <stdint.h>
    extern uint32_t SystemCoreClock;
#endif

/* 调度与基础时基 */
#define configUSE_PREEMPTION                    1   /* 抢占式调度。 */
#define configUSE_IDLE_HOOK                     0   /* 空闲钩子关闭。 */
#define configUSE_TICK_HOOK                     0   /* Tick 钩子关闭。 */

/*
 * 关键参数：CPU 时钟源，供内核计算 SysTick 重装载值。
 * 通常直接使用 SystemCoreClock，确保与 SystemClock_Config 一致。
 */
#define configCPU_CLOCK_HZ                      ( SystemCoreClock )

/*
 * 关键参数：RTOS Tick 频率。
 * 1000 表示 1ms 一次 Tick；数值越高，延时粒度越细，但中断开销也更高。
 */
#define configTICK_RATE_HZ                      ( ( TickType_t ) 1000 )

/*
 * 关键参数：任务优先级总数（0 ~ configMAX_PRIORITIES-1）。
 * 当前为 5，即可用优先级 0~4，数字越大优先级越高。
 */
#define configMAX_PRIORITIES                    ( 5 )

/* 关键参数：空闲任务最小栈深度（单位 StackType_t，不是字节）。 */
#define configMINIMAL_STACK_SIZE                ( ( unsigned short ) 130 )

/*
 * 关键参数：RTOS 堆大小（heap_4.c）。
 * 动态创建任务、队列、信号量、软件定时器都从这里分配。
 */
#define configTOTAL_HEAP_SIZE                   ( ( size_t ) ( 75 * 1024 ) )

/* 内存分配模式 */
#define configSUPPORT_STATIC_ALLOCATION         1   /* 允许静态创建 RTOS 对象。 */
#define configSUPPORT_DYNAMIC_ALLOCATION        1   /* 允许动态创建 RTOS 对象。 */

/* 任务与调试特性 */
#define configMAX_TASK_NAME_LEN                 ( 10 )  /* 任务名最大长度（含 '\0'）。 */
#define configUSE_TRACE_FACILITY                1       /* 保留基础 trace 字段。 */
#define configUSE_16_BIT_TICKS                  0       /* TickType_t 使用 32 位。 */
#define configIDLE_SHOULD_YIELD                 1       /* 同优先级下 Idle 可让出 CPU。 */

/* 同步与健壮性 */
#define configUSE_MUTEXES                       1   /* 启用互斥量。 */
#define configQUEUE_REGISTRY_SIZE               8   /* 队列/信号量调试注册表大小。 */
#define configCHECK_FOR_STACK_OVERFLOW          0   /* 关闭栈溢出检查。 */
#define configUSE_RECURSIVE_MUTEXES             1   /* 启用递归互斥量。 */
#define configUSE_MALLOC_FAILED_HOOK            0   /* 关闭内存分配失败钩子。 */
#define configUSE_APPLICATION_TASK_TAG          0   /* 关闭应用任务标签。 */
#define configUSE_COUNTING_SEMAPHORES           1   /* 启用计数信号量。 */
#define configGENERATE_RUN_TIME_STATS           0   /* 关闭运行时间统计。 */

/* 软件定时器 */
#define configUSE_TIMERS                        1                                   /* 启用软件定时器。 */
#define configTIMER_TASK_PRIORITY               ( 2 )                               /* 定时器服务任务优先级。 */
#define configTIMER_QUEUE_LENGTH                10                                  /* 定时器命令队列长度。 */
#define configTIMER_TASK_STACK_DEPTH            ( configMINIMAL_STACK_SIZE * 2 )    /* 定时器服务任务栈深度。 */

/* API 开关（1=参与编译） */
#define INCLUDE_vTaskPrioritySet                1   /* 允许修改任务优先级。 */
#define INCLUDE_uxTaskPriorityGet               1   /* 允许读取任务优先级。 */
#define INCLUDE_vTaskDelete                     1   /* 允许删除任务。 */
#define INCLUDE_vTaskCleanUpResources           1   /* 兼容清理接口（通常无需手动调用）。 */
#define INCLUDE_vTaskSuspend                    1   /* 允许挂起/恢复任务。 */
#define INCLUDE_vTaskDelayUntil                 1   /* 允许周期性延时。 */
#define INCLUDE_vTaskDelay                      1   /* 允许相对延时。 */
#define INCLUDE_xTaskGetSchedulerState          1   /* 允许读取调度器状态。 */

/*
 * 关键参数：NVIC 优先级位宽。
 * STM32F407 通常是 4 位（0~15），位宽错误会导致中断优先级配置失效。
 */
#ifdef __NVIC_PRIO_BITS
    #define configPRIO_BITS                     __NVIC_PRIO_BITS   /* CMSIS 提供的优先级位宽。 */
#else
    #define configPRIO_BITS                     4                  /* 默认 4 位优先级。 */
#endif

/*
 * 关键参数：中断优先级边界（数值越小，抢占优先级越高）。
 * 1) configLIBRARY_LOWEST_INTERRUPT_PRIORITY 设为最低优先级（通常 15）。
 * 2) configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 定义可调用 FromISR API 的最高边界。
 *    当前为 5，表示仅优先级 5~15 的中断可调用 FreeRTOS FromISR API。
 * 3) 优先级 0~4 的中断不能调用任何 FreeRTOS API。
 */
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY         0xf  /* 内核可使用的最低中断优先级。 */
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY    5    /* FromISR API 可用中断的最高边界。 */
#define configKERNEL_INTERRUPT_PRIORITY                 ( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )        /* 写入 NVIC 的内核中断优先级值。 */
#define configMAX_SYSCALL_INTERRUPT_PRIORITY            ( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )   /* 写入 BASEPRI 的屏蔽边界值（不能为 0）。 */

#define configASSERT( x ) if( ( x ) == 0 ) { taskDISABLE_INTERRUPTS(); for( ;; ); }  /* 调试断言：失败后关中断死循环。 */

/* 异常处理函数映射到 CMSIS 标准名 */
#define vPortSVCHandler                 SVC_Handler      /* 启动首个任务。 */
#define xPortPendSVHandler              PendSV_Handler   /* 任务上下文切换。 */
#define xPortSysTickHandler             SysTick_Handler  /* RTOS Tick 中断。 */

#endif /* FREERTOS_CONFIG_H */
