#include "FreeROTS_Demo.h"

/* 启动任务配置 - 静态分配方式（适配configSUPPORT_STATIC_ALLOCATION=1） */
#define START_TASK_PRIORITY 0                        // 启动任务的优先级，FreeRTOS中数值越大优先级越高
#define START_TASK_STACK_SIZE 128                    // 启动任务的栈空间大小，单位：字
TaskHandle_t start_task_handler;                     // 启动任务句柄
StackType_t start_task_stack[START_TASK_STACK_SIZE]; // 启动任务静态栈数组
StaticTask_t start_task_tcb;                         // 启动任务TCB
void Start_Task(void *pvParameters);                 // 启动任务函数声明，FreeRTOS任务入口函数

/* 任务1（LED1 翻转）配置 */
#define TASK1_PRIORITY 1
#define TASK1_STACK_SIZE 128
TaskHandle_t task1_handler;
StackType_t task1_stack[TASK1_STACK_SIZE];
StaticTask_t task1_tcb;
void vTask1(void *pvParameters);

/* 任务2（LED2 翻转）配置 */
#define TASK2_PRIORITY 2
#define TASK2_STACK_SIZE 128
TaskHandle_t task2_handler;
StackType_t task2_stack[TASK2_STACK_SIZE];
StaticTask_t task2_tcb;
void vTask2(void *pvParameters);

/* 任务3（打印时钟）配置 */
#define TASK3_PRIORITY 3
#define TASK3_STACK_SIZE 128
TaskHandle_t task3_handler;
StackType_t task3_stack[TASK3_STACK_SIZE];
StaticTask_t task3_tcb;
void vTask3(void *pvParameters);

/* 任务4 (按键扫描)配置 */
#define TASK4_PRIORITY 4
#define TASK4_STACK_SIZE 128
TaskHandle_t task4_handler;
StackType_t task4_stack[TASK4_STACK_SIZE];
StaticTask_t task4_tcb;
void vTask4(void *pvParameters);

// 空闲任务配置
#define IDLE_TASK_STACK_SIZE 128
StaticTask_t idle_task_tcb;
StackType_t idle_task_stack[IDLE_TASK_STACK_SIZE];

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t **ppxIdleTaskStackBuffer,
                                   uint32_t *pulIdleTaskStackSize)
{
    *ppxIdleTaskTCBBuffer = &idle_task_tcb;
    *ppxIdleTaskStackBuffer = &idle_task_stack[0];
    *pulIdleTaskStackSize = IDLE_TASK_STACK_SIZE;
}

// 定时器任务配置
#define TIMER_TASK_STACK_SIZE configTIMER_TASK_STACK_DEPTH
StaticTask_t timer_task_tcb;
StackType_t timer_task_stack[TIMER_TASK_STACK_SIZE];

void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
                                    StackType_t **ppxTimerTaskStackBuffer,
                                    uint32_t *pulTimerTaskStackSize)

{
    *ppxTimerTaskTCBBuffer = &timer_task_tcb;
    *ppxTimerTaskStackBuffer = &timer_task_stack[0];
    *pulTimerTaskStackSize = TIMER_TASK_STACK_SIZE;
}

void FreeROTS_Start(void)
{
    start_task_handler = xTaskCreateStatic(
        (TaskFunction_t)Start_Task,
        (char *)"Start_Task",
        (uint32_t)START_TASK_STACK_SIZE,
        (void *)NULL,
        (UBaseType_t)START_TASK_PRIORITY,
        (StackType_t *)start_task_stack,
        (StaticTask_t *)&start_task_tcb);

    printf("Before scheduler start\r\n");
    vTaskStartScheduler();
    printf("After scheduler start - should NEVER reach here!\r\n");
}

/**
 * 函数: Start_Task
 * 描述: 系统启动任务，负责创建其他任务后自删除。
 */
void Start_Task(void *pvParameters)
{
    taskENTER_CRITICAL(); /* 进入临界区，防止任务创建过程中被抢占 */
    task1_handler = xTaskCreateStatic(
        (TaskFunction_t)vTask1,
        (char *)"vTask1",
        (uint32_t)TASK1_STACK_SIZE,
        (void *)NULL,
        (UBaseType_t)TASK1_PRIORITY,
        (StackType_t *)task1_stack,
        (StaticTask_t *)&task1_tcb);
    task2_handler = xTaskCreateStatic(
        (TaskFunction_t)vTask2,
        (char *)"vTask2",
        (uint32_t)TASK2_STACK_SIZE,
        (void *)NULL,
        (UBaseType_t)TASK2_PRIORITY,
        (StackType_t *)task2_stack,
        (StaticTask_t *)&task2_tcb);
    task3_handler = xTaskCreateStatic(
        (TaskFunction_t)vTask3,
        (char *)"vTask3",
        (uint32_t)TASK3_STACK_SIZE,
        (void *)NULL,
        (UBaseType_t)TASK3_PRIORITY,
        (StackType_t *)task3_stack,
        (StaticTask_t *)&task3_tcb);
    task4_handler = xTaskCreateStatic(
        (TaskFunction_t)vTask4,
        (char *)"vTask4",
        (uint32_t)TASK4_STACK_SIZE,
        (void *)NULL,
        (UBaseType_t)TASK4_PRIORITY,
        (StackType_t *)task4_stack,
        (StaticTask_t *)&task4_tcb);
    vTaskDelete(start_task_handler); /* 创建完成后删除自身 */
    taskEXIT_CRITICAL();             /* 退出临界区 */
}

/**
 * 函数: vTask1
 * 描述: LED1翻转任务，周期 500ms。
 */
void vTask1(void *pvParameters)
{
    while (1)
    {
        printf("Task1 is running  RTOS tick: %lu  \r\n", xTaskGetTickCount());
        LED_Toggle(LED_PIN1);
        vTaskDelay(pdMS_TO_TICKS(500)); /* 延时 500ms */
    }
}

void vTask2(void *pvParameters)
{
    while (1)
    {
        printf("Task2 is running  RTOS tick: %lu  \r\n", xTaskGetTickCount());
        LED_Toggle(LED_PIN2);
        vTaskDelay(pdMS_TO_TICKS(500)); /* 延时 500ms */
    }
}

/**
 * 函数: vTask3
 * 描述: 周期性打印 RTOS 与 HAL 毫秒计数，周期 500ms。
 */
void vTask3(void *pvParameters)
{
    while (1)
    {
        printf("Task3 is running  RTOS tick: %lu  HAL tick: %lu\r\n",
               xTaskGetTickCount(), HAL_GetTick());
        vTaskDelay(pdMS_TO_TICKS(500)); /* 延时 500ms */
    }
}

void vTask4(void *pvParameters)
{
    uint8_t key_num = 0;
    while (1)
    {
        printf("Task4 is running  RTOS tick: %lu  \r\n", xTaskGetTickCount());
        /* 按键扫描逻辑 */
        key_num = Key_Scan();
        if (key_num == 1)
        {
            OLED_ShowString(2, 1, "KEY1 Pressed!");
            printf("KEY1 Pressed!\r\n");
            if (task1_handler != NULL)
            {
                vTaskDelete(task1_handler); /* 删除任务1 */
                task1_handler = NULL;       /* 清空任务句柄 */
                OLED_ShowString(1, 1, "Task1 Deleted");
                printf("Task1 Deleted\r\n");
                vTaskDelay(pdMS_TO_TICKS(500)); /* 延时 500ms */
                OLED_ShowString(1, 1, "             ");
            }
            vTaskDelay(pdMS_TO_TICKS(500)); /* 延时 500ms */
            OLED_ShowString(2, 1, "             ");
        }
        else if (key_num == 2)
        {
            OLED_ShowString(2, 1, "KEY2 Pressed!");
            printf("KEY2 Pressed!\r\n");
            if (task2_handler != NULL)
            {
                vTaskDelete(task2_handler); /* 删除任务2 */
                task2_handler = NULL;       /* 清空任务句柄 */
                OLED_ShowString(1, 1, "Task2 Deleted");
                printf("Task2 Deleted\r\n");
                vTaskDelay(pdMS_TO_TICKS(500)); /* 延时 500ms */
                OLED_ShowString(1, 1, "             ");
            }
            vTaskDelay(pdMS_TO_TICKS(500)); /* 延时 500ms */
            OLED_ShowString(2, 1, "             ");
        }
        vTaskDelay(pdMS_TO_TICKS(100)); /* 延时 100ms */
    }
}
