#include "./SYSTEM/usart/usart.h"
#include <string.h>
#include "queue.h"
#include "semphr.h"
#include "task.h"

#if (__ARMCC_VERSION >= 6010050)
__asm(".global __use_no_semihosting\n\t");
__asm(".global __ARM_use_no_argv \n\t");
#else
#pragma import(__use_no_semihosting)
struct __FILE
{
    int handle;
};
#endif

FILE __stdout;

int _ttywrch(int ch)
{
    (void)ch;
    return ch;
}

void _sys_exit(int x)
{
    (void)x;
}

char *_sys_command_string(char *cmd, int len)
{
    (void)cmd;
    (void)len;
    return NULL;
}

typedef struct
{
    uint16_t len;
    uint8_t data[USART1_RX_FRAME_SIZE];
} usart1_rx_frame_t;

static UART_HandleTypeDef s_uart1;
static DMA_HandleTypeDef s_dma_rx;
static DMA_HandleTypeDef s_dma_tx;

static uint8_t s_rx_dma_buffer[2U][USART1_RX_DMA_BUFFER_SIZE];
static uint8_t s_rx_dma_index;

static StaticQueue_t s_rx_queue_control;
static uint8_t s_rx_queue_storage[USART1_RX_FRAME_QUEUE_LENGTH * sizeof(usart1_rx_frame_t)];
static QueueHandle_t s_rx_queue;

static uint8_t s_tx_ring[USART1_TX_RING_SIZE];
static volatile uint16_t s_tx_head;
static volatile uint16_t s_tx_tail;
static volatile uint16_t s_tx_dma_len;
static volatile uint8_t s_tx_dma_active;

static StaticSemaphore_t s_tx_mutex_control;
static StaticSemaphore_t s_tx_space_control;
static SemaphoreHandle_t s_tx_mutex;
static SemaphoreHandle_t s_tx_space;

static usart1_stats_t s_stats;
static uint8_t s_usart1_ready;

static uint8_t usart1_in_isr(void)
{
    return (__get_IPSR() != 0U) ? 1U : 0U;
}

static uint8_t usart1_scheduler_running(void)
{
    return (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) ? 1U : 0U;
}

static TickType_t usart1_remaining_ticks(TickType_t start, TickType_t timeout)
{
    TickType_t elapsed;

    if (timeout == portMAX_DELAY)
    {
        return portMAX_DELAY;
    }

    elapsed = xTaskGetTickCount() - start;
    if (elapsed >= timeout)
    {
        return 0U;
    }

    return timeout - elapsed;
}

static uint16_t usart1_tx_used(void)
{
    uint16_t head = s_tx_head;
    uint16_t tail = s_tx_tail;

    if (head >= tail)
    {
        return (uint16_t)(head - tail);
    }

    return (uint16_t)(USART1_TX_RING_SIZE - tail + head);
}

static uint16_t usart1_tx_free(void)
{
    return (uint16_t)(USART1_TX_RING_SIZE - 1U - usart1_tx_used());
}

static HAL_StatusTypeDef usart1_start_rx_dma(void)
{
    HAL_StatusTypeDef status;

    status = HAL_UARTEx_ReceiveToIdle_DMA(&s_uart1,
                                          s_rx_dma_buffer[s_rx_dma_index],
                                          USART1_RX_DMA_BUFFER_SIZE);
    if ((status == HAL_OK) && (s_uart1.hdmarx != NULL))
    {
        /*
         * ReceiveToIdle DMA 默认会在半满、全满、空闲三类事件都回调。
         * 对“按空闲分帧”的串口协议来说，半满回调不是完整帧，关闭 HT
         * 可以减少一次无意义中断，也能避免应用误把半包当成协议帧。
         */
        __HAL_DMA_DISABLE_IT(s_uart1.hdmarx, DMA_IT_HT);
    }

    return status;
}

static void usart1_tx_start_next_locked(void)
{
    uint16_t linear_len;
    HAL_StatusTypeDef status;

    if ((s_tx_dma_active != 0U) || (s_tx_head == s_tx_tail) || (s_usart1_ready == 0U))
    {
        return;
    }

    if (s_tx_head > s_tx_tail)
    {
        linear_len = (uint16_t)(s_tx_head - s_tx_tail);
    }
    else
    {
        linear_len = (uint16_t)(USART1_TX_RING_SIZE - s_tx_tail);
    }

    s_tx_dma_active = 1U;
    s_tx_dma_len = linear_len;

    status = HAL_UART_Transmit_DMA(&s_uart1, &s_tx_ring[s_tx_tail], linear_len);
    if (status != HAL_OK)
    {
        s_tx_dma_active = 0U;
        s_tx_dma_len = 0U;
        s_stats.tx_dma_errors++;
    }
}

static void usart1_tx_kick(void)
{
    uint32_t primask;

    primask = sys_irq_save();
    usart1_tx_start_next_locked();
    sys_irq_restore(primask);
}

static void usart1_queue_rx_frame_from_isr(const uint8_t *data, uint16_t size, BaseType_t *higher_priority_task_woken)
{
    usart1_rx_frame_t frame;

    if ((size == 0U) || (s_rx_queue == NULL))
    {
        return;
    }

    if (size > USART1_RX_FRAME_SIZE)
    {
        frame.len = USART1_RX_FRAME_SIZE;
        s_stats.rx_truncated++;
    }
    else
    {
        frame.len = size;
    }

    memcpy(frame.data, data, frame.len);

    if (xQueueSendFromISR(s_rx_queue, &frame, higher_priority_task_woken) != pdPASS)
    {
        s_stats.rx_queue_overflow++;
        return;
    }

    s_stats.rx_frames++;
    s_stats.rx_bytes += frame.len;
}

HAL_StatusTypeDef usart1_init(uint32_t baudrate)
{
    HAL_StatusTypeDef status;

    if (s_rx_queue == NULL)
    {
        s_rx_queue = xQueueCreateStatic(USART1_RX_FRAME_QUEUE_LENGTH,
                                        sizeof(usart1_rx_frame_t),
                                        s_rx_queue_storage,
                                        &s_rx_queue_control);
    }

    if (s_tx_mutex == NULL)
    {
        s_tx_mutex = xSemaphoreCreateMutexStatic(&s_tx_mutex_control);
    }

    if (s_tx_space == NULL)
    {
        s_tx_space = xSemaphoreCreateBinaryStatic(&s_tx_space_control);
    }

    if ((s_rx_queue == NULL) || (s_tx_mutex == NULL) || (s_tx_space == NULL))
    {
        return HAL_ERROR;
    }

    s_tx_head = 0U;
    s_tx_tail = 0U;
    s_tx_dma_len = 0U;
    s_tx_dma_active = 0U;
    s_rx_dma_index = 0U;
    memset(&s_stats, 0, sizeof(s_stats));

    s_uart1.Instance = USART1;
    s_uart1.Init.BaudRate = baudrate;
    s_uart1.Init.WordLength = UART_WORDLENGTH_8B;
    s_uart1.Init.StopBits = UART_STOPBITS_1;
    s_uart1.Init.Parity = UART_PARITY_NONE;
    s_uart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    s_uart1.Init.Mode = UART_MODE_TX_RX;
    s_uart1.Init.OverSampling = UART_OVERSAMPLING_16;

    status = HAL_UART_Init(&s_uart1);
    if (status != HAL_OK)
    {
        return status;
    }

    s_usart1_ready = 1U;
    status = usart1_start_rx_dma();
    if (status != HAL_OK)
    {
        s_stats.rx_dma_restart_errors++;
        s_usart1_ready = 0U;
    }

    return status;
}

size_t usart1_write(const void *data, size_t len, TickType_t timeout)
{
    const uint8_t *src = (const uint8_t *)data;
    size_t written = 0U;
    TickType_t start_tick;
    TickType_t wait_ticks;

    if ((src == NULL) || (len == 0U) || (s_usart1_ready == 0U))
    {
        return 0U;
    }

    /*
     * 中断里不能调用 HAL_UART_Transmit() 这类阻塞接口：
     * 1. 它会在 ISR 中忙等，拉长中断占用时间；
     * 2. 如果串口状态正被 DMA 使用，阻塞发送还可能破坏 TX 状态机；
     * 3. FreeRTOS 任务无法在 ISR 忙等期间获得 CPU。
     * 因此本模块约定：任务或启动阶段可以打印，中断中不要 printf。
     */
    if (usart1_in_isr() != 0U)
    {
        s_stats.tx_timeouts++;
        return 0U;
    }

    /*
     * 调度器启动前，printf 常用于早期启动日志。此时还没有任务切换，
     * 直接使用 HAL 阻塞发送最简单，也不会和其他任务竞争串口。
     */
    if (usart1_scheduler_running() == 0U)
    {
        while (written < len)
        {
            uint16_t chunk = (uint16_t)((len - written) > 0xFFFFU ? 0xFFFFU : (len - written));
            if (HAL_UART_Transmit(&s_uart1, (uint8_t *)&src[written], chunk, HAL_MAX_DELAY) != HAL_OK)
            {
                break;
            }
            written += chunk;
        }
        return written;
    }

    start_tick = xTaskGetTickCount();

    if (xSemaphoreTake(s_tx_mutex, timeout) != pdTRUE)
    {
        s_stats.tx_timeouts++;
        return 0U;
    }

    while (written < len)
    {
        if (usart1_tx_free() > 0U)
        {
            s_tx_ring[s_tx_head] = src[written];
            s_tx_head = (uint16_t)((s_tx_head + 1U) % USART1_TX_RING_SIZE);
            written++;
            s_stats.tx_bytes++;
            usart1_tx_kick();
            continue;
        }

        usart1_tx_kick();
        s_stats.tx_ring_overflow++;

        wait_ticks = usart1_remaining_ticks(start_tick, timeout);
        if (wait_ticks == 0U)
        {
            s_stats.tx_timeouts++;
            break;
        }

        if (xSemaphoreTake(s_tx_space, wait_ticks) != pdTRUE)
        {
            s_stats.tx_timeouts++;
            break;
        }
    }

    xSemaphoreGive(s_tx_mutex);
    return written;
}

size_t usart1_read_frame(uint8_t *out, size_t out_size, TickType_t timeout)
{
    usart1_rx_frame_t frame;
    size_t copy_len;

    if ((out == NULL) || (out_size == 0U) || (s_rx_queue == NULL))
    {
        return 0U;
    }

    if (xQueueReceive(s_rx_queue, &frame, timeout) != pdPASS)
    {
        return 0U;
    }

    copy_len = frame.len;
    if (copy_len > out_size)
    {
        copy_len = out_size;
    }

    memcpy(out, frame.data, copy_len);
    return copy_len;
}

void usart1_get_stats(usart1_stats_t *stats)
{
    uint32_t primask;

    if (stats == NULL)
    {
        return;
    }

    primask = sys_irq_save();
    *stats = s_stats;
    sys_irq_restore(primask);
}

void usart1_clear_stats(void)
{
    uint32_t primask;

    primask = sys_irq_save();
    memset(&s_stats, 0, sizeof(s_stats));
    sys_irq_restore(primask);
}

int fputc(int ch, FILE *f)
{
    uint8_t byte = (uint8_t)ch;

    (void)f;
    (void)usart1_write(&byte, 1U, portMAX_DELAY);
    return ch;
}

void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef gpio;

    if (huart->Instance != USART1)
    {
        return;
    }

    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_DMA2_CLK_ENABLE();

    gpio.Pin = GPIO_PIN_9 | GPIO_PIN_10;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &gpio);

    /*
     * USART1_RX -> DMA2 Stream2 Channel4。
     * 使用 NORMAL 模式而不是 CIRCULAR：每次空闲事件形成一帧后重新启动 DMA，
     * 这样驱动可以天然得到“帧边界”，协议层不需要再猜什么时候一帧结束。
     */
    s_dma_rx.Instance = DMA2_Stream2;
    s_dma_rx.Init.Channel = DMA_CHANNEL_4;
    s_dma_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    s_dma_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    s_dma_rx.Init.MemInc = DMA_MINC_ENABLE;
    s_dma_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    s_dma_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    s_dma_rx.Init.Mode = DMA_NORMAL;
    s_dma_rx.Init.Priority = DMA_PRIORITY_HIGH;
    s_dma_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    (void)HAL_DMA_Init(&s_dma_rx);
    __HAL_LINKDMA(huart, hdmarx, s_dma_rx);

    /*
     * USART1_TX -> DMA2 Stream7 Channel4。
     * 发送侧 DMA 直接读取 TX ring 中的一段连续数据。一次 DMA 完成后，
     * 中断回调推进 tail，再启动下一段，因此应用层可以连续 printf。
     */
    s_dma_tx.Instance = DMA2_Stream7;
    s_dma_tx.Init.Channel = DMA_CHANNEL_4;
    s_dma_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    s_dma_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    s_dma_tx.Init.MemInc = DMA_MINC_ENABLE;
    s_dma_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    s_dma_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    s_dma_tx.Init.Mode = DMA_NORMAL;
    s_dma_tx.Init.Priority = DMA_PRIORITY_MEDIUM;
    s_dma_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    (void)HAL_DMA_Init(&s_dma_tx);
    __HAL_LINKDMA(huart, hdmatx, s_dma_tx);

    /*
     * FreeRTOS 规则：会调用 FromISR API 的中断，抢占优先级数值必须 >=
     * configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY。本工程该值为 5，
     * 因此这里统一设置为 6，保证 USART/DMA ISR 可以安全唤醒任务。
     */
    HAL_NVIC_SetPriority(USART1_IRQn, 6U, 0U);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
    HAL_NVIC_SetPriority(DMA2_Stream2_IRQn, 6U, 0U);
    HAL_NVIC_EnableIRQ(DMA2_Stream2_IRQn);
    HAL_NVIC_SetPriority(DMA2_Stream7_IRQn, 6U, 0U);
    HAL_NVIC_EnableIRQ(DMA2_Stream7_IRQn);
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    BaseType_t higher_priority_task_woken = pdFALSE;
    HAL_UART_RxEventTypeTypeDef event;
    uint8_t current_index;
    HAL_StatusTypeDef status;

    if (huart->Instance != USART1)
    {
        return;
    }

    event = HAL_UARTEx_GetRxEventType(huart);
    if (event == HAL_UART_RXEVENT_HT)
    {
        /*
         * HT 是 DMA 半满事件，不代表串口空闲，也不代表一帧结束。
         * 正常情况下我们已经关闭 HT 中断；这里保留判断是为了防止
         * HAL 版本差异或极早期竞态导致半包被误投递给协议层。
         */
        return;
    }

    if ((event == HAL_UART_RXEVENT_TC) && (Size >= USART1_RX_FRAME_SIZE))
    {
        /*
         * TC 表示 staging buffer 被写满后触发回调。对“按空闲分帧”的
         * 串口协议来说，这通常意味着一帧可能超过了本模块允许的 256 字节，
         * 因此统计为截断风险，方便后续根据协议实际长度调整缓冲区。
         */
        s_stats.rx_truncated++;
    }

    current_index = s_rx_dma_index;
    usart1_queue_rx_frame_from_isr(s_rx_dma_buffer[current_index],
                                   Size,
                                   &higher_priority_task_woken);

    s_rx_dma_index ^= 1U;
    status = usart1_start_rx_dma();
    if (status != HAL_OK)
    {
        s_stats.rx_dma_restart_errors++;
    }

    portYIELD_FROM_ISR(higher_priority_task_woken);
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance != USART1)
    {
        return;
    }

    s_stats.uart_errors++;
    (void)HAL_UART_DMAStop(huart);
    if (usart1_start_rx_dma() != HAL_OK)
    {
        s_stats.rx_dma_restart_errors++;
    }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    BaseType_t higher_priority_task_woken = pdFALSE;

    if (huart->Instance != USART1)
    {
        return;
    }

    s_tx_tail = (uint16_t)((s_tx_tail + s_tx_dma_len) % USART1_TX_RING_SIZE);
    s_tx_dma_len = 0U;
    s_tx_dma_active = 0U;
    s_stats.tx_dma_transfers++;

    if (s_tx_space != NULL)
    {
        (void)xSemaphoreGiveFromISR(s_tx_space, &higher_priority_task_woken);
    }

    usart1_tx_start_next_locked();
    portYIELD_FROM_ISR(higher_priority_task_woken);
}

void USART1_IRQHandler(void)
{
    HAL_UART_IRQHandler(&s_uart1);
}

void DMA2_Stream2_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&s_dma_rx);
}

void DMA2_Stream7_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&s_dma_tx);
}
