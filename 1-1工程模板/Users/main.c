#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/usart/usart.h"
#include "./SYSTEM/delay/delay.h"
#include "./Hardware/LCD/LCD.h"
#include "FreeRTOS.h"
#include "task.h"

static void app_task(void *argument);

int main(void)
{
    HAL_Init();

    if (sys_clock_init_168mhz() != HAL_OK)
    {
        while (1)
        {
        }
    }

    delay_init();

    if (usart1_init(115200U) != HAL_OK)
    {
        while (1)
        {
        }
    }

    printf("SYSTEM drivers ready\r\n");

    LCD_init();

    if (xTaskCreate(app_task, "app", 768U, NULL, 2U, NULL) != pdPASS)
    {
        printf("app task create failed\r\n");
        while (1)
        {
        }
    }

    vTaskStartScheduler();

    while (1)
    {
    }
}

static void app_task(void *argument)
{
    uint8_t frame[USART1_RX_FRAME_SIZE];
    size_t len;

    (void)argument;

    while (1)
    {
        len = usart1_read_frame(frame, sizeof(frame), pdMS_TO_TICKS(1000U));
        if (len > 0U)
        {
            printf("RX frame %lu bytes: ", (uint32_t)len);
            (void)usart1_write(frame, len, pdMS_TO_TICKS(100U));
            printf("\r\n");
        }
        else
        {
            printf("heartbeat, HAL tick = %lu\r\n", HAL_GetTick());
        }
    }
}
