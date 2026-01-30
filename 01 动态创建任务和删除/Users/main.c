#include "stm32f1xx_hal.h"
#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/delay/delay.h"
#include "FreeROTS_Demo.h"

int main(void)
{
    HAL_Init();                         /* 初始化HAL库 */
    sys_stm32_clock_init(RCC_PLL_MUL9); /* 设置时钟, 72Mhz */
    usart_init(115200);                 /* 初始化串口为115200 */
    delay_init(72);                     /* 延时初始化 */
    MX_TIM2_Init();                     /* 初始化TIM2用于HAL tick */

    Key_Init(); /* 初始化按键 */
    LED_Init();
    OLED_Init();

    FreeROTS_Start(); /* 启动FreeRTOS */

    while (1)
    {
    }
}
