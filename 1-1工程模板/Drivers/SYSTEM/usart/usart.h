#ifndef USART_H
#define USART_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "./SYSTEM/sys/sys.h"
#include "FreeRTOS.h"

/*
 * 本模块只适配当前工程的 USART1：
 * - TX: PA9
 * - RX: PA10
 * - RX DMA: DMA2 Stream2 Channel4
 * - TX DMA: DMA2 Stream7 Channel4
 *
 * 设计目标不是“能 printf 就行”，而是给后续协议层提供稳定边界：
 * 接收侧按串口空闲中断划分一帧，任务通过 usart1_read_frame() 取帧；
 * 发送侧使用环形缓冲和 DMA，任务写日志时不会一直占用 CPU 等待字节发送。
 */

#define USART1_RX_DMA_BUFFER_SIZE     256U
#define USART1_RX_FRAME_SIZE          256U
#define USART1_RX_FRAME_QUEUE_LENGTH  8U
#define USART1_TX_RING_SIZE           1024U

typedef struct
{
    uint32_t rx_frames;
    uint32_t rx_bytes;
    uint32_t rx_queue_overflow;
    uint32_t rx_truncated;
    uint32_t rx_dma_restart_errors;
    uint32_t uart_errors;
    uint32_t tx_bytes;
    uint32_t tx_dma_transfers;
    uint32_t tx_dma_errors;
    uint32_t tx_ring_overflow;
    uint32_t tx_timeouts;
} usart1_stats_t;

#ifdef __cplusplus
extern "C" {
#endif

HAL_StatusTypeDef usart1_init(uint32_t baudrate);
size_t usart1_write(const void *data, size_t len, TickType_t timeout);
size_t usart1_read_frame(uint8_t *out, size_t out_size, TickType_t timeout);
void usart1_get_stats(usart1_stats_t *stats);
void usart1_clear_stats(void);
int fputc(int ch, FILE *f);

#ifdef __cplusplus
}
#endif

#endif
