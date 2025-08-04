/**
 * @file    bsp_usart_hal.h
 * @brief   STM32F1系列 USART + DMA + RingBuffer HAL库底层驱动接口（多实例、可变缓冲区）
 * @author  Wallace.zhang
 * @version 2.0.0
 * @date    2025-05-23
 */
#ifndef __BSP_USART_HAL_H
#define __BSP_USART_HAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "usart.h"
#include "dma.h"
#include "lwrb/lwrb.h"

/**
 * @brief USART驱动结构体（DMA + RingBuffer + 统计）
 */
typedef struct
{
    volatile uint8_t   txDMABusy;        /**< DMA发送忙标志（1：发送中，0：空闲） */
    volatile uint64_t  rxMsgCount;       /**< 统计接收字节总数 */
    volatile uint64_t  txMsgCount;       /**< 统计发送字节总数 */
    volatile uint16_t  dmaRxLastPos;     /**< DMA接收缓冲区上次处理到的位置 */
    volatile uint32_t  errorDMATX;       /**< DMA发送错误统计 */
    volatile uint32_t  errorDMARX;       /**< DMA接收错误统计 */
    volatile uint32_t  errorRX;          /**< 串口接收错误统计 */

    DMA_HandleTypeDef   *hdma_rx;        /**< HAL库的DMA RX句柄 */
    DMA_HandleTypeDef   *hdma_tx;        /**< HAL库的DMA TX句柄 */
    UART_HandleTypeDef  *huart;          /**< HAL库的UART句柄 */

    /* RX方向 */
    uint8_t             *rxDMABuffer;    /**< DMA接收缓冲区 */
    uint8_t             *rxRBBuffer;     /**< 接收RingBuffer缓存区 */
    uint16_t            rxBufSize;       /**< DMA接收/RX Ringbuffer缓冲区大小 */
    lwrb_t              rxRB;            /**< 接收RingBuffer句柄 */

    /* TX方向 */
    uint8_t             *txDMABuffer;    /**< DMA发送缓冲区 */
    uint8_t             *txRBBuffer;     /**< 发送RingBuffer缓存区 */
    uint16_t            txBufSize;       /**< DMA发送/TX Ringbuffer缓冲区大小 */
    lwrb_t              txRB;            /**< 发送RingBuffer句柄 */
    
} USART_Driver_t;

/**
  * @brief  阻塞方式发送以 NUL 结尾的字符串
  */
void USART_SendString_Blocking(USART_Driver_t* const usart, const char* str);
/**
  * @brief  USART发送DMA中断处理函数
  */
void USART_DMA_TX_Interrupt_Handler(USART_Driver_t *usart);
/**
  * @brief  USART接收DMA中断处理函数
  */
void USART_DMA_RX_Interrupt_Handler(USART_Driver_t *usart);
/**
 * @brief  USART空闲接收中断处理函数（支持DMA+RingBuffer，适用于多实例）
 */
void USART_RX_IDLE_Interrupt_Handler(USART_Driver_t *usart);
/**
  * @brief  将数据写入指定USART驱动的发送 RingBuffer中
  */
uint8_t USART_Put_TxData_To_Ringbuffer(USART_Driver_t *usart, const void* data, uint16_t len);
/**
 * @brief  USART模块定时任务处理函数，建议主循环1ms周期回调
 */
void USART_Module_Run(USART_Driver_t *usart);
/**
  * @brief  获取USART接收RingBuffer中的可读字节数
  */
uint32_t USART_Get_The_Existing_Amount_Of_Data(USART_Driver_t *usart);
/**
  * @brief  从USART接收RingBuffer中读取一个字节数据
  */
uint8_t USART_Take_A_Piece_Of_Data(USART_Driver_t *usart, uint8_t* data);
/**
  * @brief  DMA传输错误后的自动恢复操作（含错误统计）
  */
void USART_DMA_Error_Recover(USART_Driver_t *usart, uint8_t dir);
/**
 * @brief   初始化USART驱动，配置DMA、RingBuffer与中断
 */
void USART_Config(USART_Driver_t *usart,
                  uint8_t *rxDMABuffer, uint8_t *rxRBBuffer, uint16_t rxBufSize,
                  uint8_t *txDMABuffer, uint8_t *txRBBuffer, uint16_t txBufSize);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_USART_HAL_H */
