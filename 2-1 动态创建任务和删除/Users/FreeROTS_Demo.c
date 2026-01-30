#/*******************************************************************************
#* 文件: Users/FreeROTS_Demo.c
#* 描述: FreeRTOS 示例程序。
#*       包含系统启动任务和两个示例任务：LED 翻转任务与打印时钟任务。
#* 功能:
#*   - FreeROTS_Start(): 创建启动任务并启动调度器。
#*   - Start_Task(): 创建 vTask1 和 vTask2 后自删除。
#*   - vTask1(): 每 500ms 翻转 LED（GPIOC PIN13）。
#*   - vTask2(): 每 500ms 打印 RTOS 与 HAL 时间戳。
#* 日期: 2026-01-30
#******************************************************************************/

#include "FreeROTS_Demo.h"

/* 启动任务配置 */
#define START_TASK_PRIORITY 1
#define START_TASK_STACK_SIZE 128
TaskHandle_t start_task_handler;
StackType_t start_task_stack[START_TASK_STACK_SIZE];
void Start_Task(void *pvParameters);

/* 任务1（LED1 翻转）配置 */
#define TASK1_PRIORITY 1
#define TASK1_STACK_SIZE 128
TaskHandle_t task1_handler;
StackType_t task1_stack[TASK1_STACK_SIZE];
void vTask1(void *pvParameters);
/* 任务2（LED2 翻转）配置 */
#define TASK2_PRIORITY 1
#define TASK2_STACK_SIZE 128
TaskHandle_t task2_handler;
StackType_t task2_stack[TASK2_STACK_SIZE];
void vTask2(void *pvParameters);

/* 任务3（打印时钟）配置 */
#define TASK3_PRIORITY 1
#define TASK3_STACK_SIZE 128
TaskHandle_t task3_handler;
StackType_t task3_stack[TASK3_STACK_SIZE];
void vTask3(void *pvParameters);

/* 任务4 (按键扫描)配置 */
#define TASK4_PRIORITY 2
#define TASK4_STACK_SIZE 128
TaskHandle_t task4_handler;
StackType_t task4_stack[TASK4_STACK_SIZE];
void vTask4(void *pvParameters);


void FreeROTS_Start(void)
{
    xTaskCreate(
        Start_Task,
        "Start_Task",
        START_TASK_STACK_SIZE,
        NULL,
        START_TASK_PRIORITY,
        &start_task_handler);
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
    xTaskCreate(
        vTask1,
        "vTask1",
        TASK1_STACK_SIZE,
        NULL,
        TASK1_PRIORITY,
        &task1_handler);
    xTaskCreate(
        vTask2,
        "Task2",
        TASK2_STACK_SIZE,
        NULL,
        TASK2_PRIORITY,
        &task2_handler);
    xTaskCreate(
        vTask3,
        "Task3",
        TASK3_STACK_SIZE,
        NULL,
        TASK3_PRIORITY,
        &task3_handler);
    xTaskCreate(
        vTask4,
        "Task4",
        TASK4_STACK_SIZE,
        NULL,
        TASK4_PRIORITY,
        &task4_handler);
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
        printf("Task3 is running  RTOS tick: %lu \r\n", xTaskGetTickCount());
        printf("RTOS tick: %lu    HAL tick: %lu\r\n", xTaskGetTickCount(), HAL_GetTick());
        vTaskDelay(pdMS_TO_TICKS(1000)); /* 延时 1000ms */
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


