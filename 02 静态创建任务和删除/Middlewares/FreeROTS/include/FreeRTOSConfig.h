#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* 调度器配置 */
#define configUSE_PREEMPTION            1   // 启用抢占式调度器(1=启用, 0=协作式)
#define configIDLE_SHOULD_YIELD         1   // 空闲任务在有同优先级任务时主动让出CPU(1=允许, 0=不允许)

/* 钩子函数配置 */
#define configUSE_IDLE_HOOK             0   // 禁用空闲任务钩子函数(1=启用, 0=禁用)，启用需实现vApplicationIdleHook()
#define configUSE_TICK_HOOK             0   // 禁用时钟节拍钩子函数(1=启用, 0=禁用)，启用需实现vApplicationTickHook()

/* 系统时钟配置 */
#define configCPU_CLOCK_HZ              ( ( unsigned long ) 72000000 )  // CPU核心时钟频率，此处为72MHz
#define configTICK_RATE_HZ              ( ( TickType_t ) 1000 )        // 系统时钟节拍频率，1000Hz表示时钟节拍周期1ms

/* 任务配置 */
#define configMAX_PRIORITIES            ( 5 )   // 系统支持的最大任务优先级数，优先级范围0~4(数值越大优先级越高)
#define configMINIMAL_STACK_SIZE        ( ( unsigned short ) 128 )    // 空闲任务的最小栈空间大小，单位：字(32位系统下1字=4字节)
#define configMAX_TASK_NAME_LEN         ( 16 )  // 任务名最大长度，包含结束符'\0'

/* 内存配置 */
#define configTOTAL_HEAP_SIZE           ( ( size_t ) ( 17 * 1024 ) )  // 动态内存堆总大小，17KB，用于动态创建任务/队列/信号量等

/* 调试与跟踪配置 */
#define configUSE_TRACE_FACILITY        0   // 禁用跟踪调试功能(1=启用, 0=禁用)，用于可视化任务运行状态
#define configUSE_16_BIT_TICKS          0   // 禁用16位时钟节拍类型(1=启用, 0=禁用)，0表示使用32位TickType_t，支持更长定时

/* FreeRTOS核心API使能配置 */
#define INCLUDE_vTaskPrioritySet        1   // 启用任务优先级设置API
#define INCLUDE_uxTaskPriorityGet       1   // 启用任务优先级获取API
#define INCLUDE_vTaskDelete             1   // 启用任务删除API
#define INCLUDE_vTaskCleanUpResources   0   // 禁用任务资源清理API，与vTaskDelete配合使用
#define INCLUDE_vTaskSuspend            1   // 启用任务挂起/恢复API
#define INCLUDE_vTaskDelayUntil         1   // 启用绝对延时API(精准定时)
#define INCLUDE_vTaskDelay              1   // 启用相对延时API(简易定时)
#define INCLUDE_xTaskGetSchedulerState  1   // 启用调度器状态获取API(运行/挂起/未启动)

/* Cortex-M3内核中断优先级配置 (NVIC原生优先级，0=最高，255=最低) */
#define configKERNEL_INTERRUPT_PRIORITY         255  // 内核中断( PendSV/SVC )的优先级，设置为最低255
/* 系统调用可屏蔽的最高中断优先级，禁止设为0！
* 优先级数值 ≤ 此值的中断，不能调用FreeRTOS的API函数
* 191 = 0xB0，对应NVIC优先级分组4下的第11级
*/
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    191

/* ST库中断优先级映射 (适配ST标准库，0=最高，15=最低) */
#define configLIBRARY_KERNEL_INTERRUPT_PRIORITY 15  // 库层面内核中断优先级，与255原生优先级对应

/* Cortex-M3内核异常处理函数映射，对接FreeRTOS底层实现 */
#define xPortPendSVHandler  PendSV_Handler  // 挂起服务例程，用于任务上下文切换
#define vPortSVCHandler     SVC_Handler     // 系统调用服务例程，用于启动调度器/任务创建

/* 任务内存分配方式配置 */
#define configSUPPORT_STATIC_ALLOCATION   1   // 启用静态内存分配(1=启用, 0=禁用)，支持无堆内存创建任务/对象

/* 软件定时器功能配置 */
#define configUSE_TIMERS                    1   // 启用软件定时器功能(1=启用, 0=禁用)
#define configTIMER_TASK_PRIORITY           ( configMAX_PRIORITIES - 1 )    // 软件定时器任务优先级，设为次最高
#define configTIMER_QUEUE_LENGTH            5   // 软件定时器命令队列长度，支持5个未处理命令
#define configTIMER_TASK_STACK_DEPTH        ( configMINIMAL_STACK_SIZE * 2) // 软件定时器任务栈大小，2倍最小栈空间

#endif /* FREERTOS_CONFIG_H */
