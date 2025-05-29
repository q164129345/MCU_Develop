/**
 * @file    bsp_usart_hal.h
 * @brief   STM32F1ϵ�� USART + DMA + RingBuffer HAL��ײ������ӿڣ���ʵ�����ɱ仺������
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
 * @brief USART�����ṹ�壨DMA + RingBuffer + ͳ�ƣ�
 */
typedef struct
{
    volatile uint8_t   txDMABusy;        /**< DMA����æ��־��1�������У�0�����У� */
    volatile uint64_t  rxMsgCount;       /**< ͳ�ƽ����ֽ����� */
    volatile uint64_t  txMsgCount;       /**< ͳ�Ʒ����ֽ����� */
    volatile uint16_t  dmaRxLastPos;     /**< DMA���ջ������ϴδ�����λ�� */
    volatile uint32_t  errorDMATX;       /**< DMA���ʹ���ͳ�� */
    volatile uint32_t  errorDMARX;       /**< DMA���մ���ͳ�� */
    volatile uint32_t  errorRX;          /**< ���ڽ��մ���ͳ�� */

    DMA_HandleTypeDef   *hdma_rx;        /**< HAL���DMA RX��� */
    DMA_HandleTypeDef   *hdma_tx;        /**< HAL���DMA TX��� */
    UART_HandleTypeDef  *huart;          /**< HAL���UART��� */

    /* RX���� */
    uint8_t             *rxDMABuffer;    /**< DMA���ջ����� */
    uint8_t             *rxRBBuffer;     /**< ����RingBuffer������ */
    uint16_t            rxBufSize;       /**< DMA����/RX Ringbuffer��������С */
    lwrb_t              rxRB;            /**< ����RingBuffer��� */

    /* TX���� */
    uint8_t             *txDMABuffer;    /**< DMA���ͻ����� */
    uint8_t             *txRBBuffer;     /**< ����RingBuffer������ */
    uint16_t            txBufSize;       /**< DMA����/TX Ringbuffer��������С */
    lwrb_t              txRB;            /**< ����RingBuffer��� */
    
} USART_Driver_t;

/**
  * @brief  ������ʽ������ NUL ��β���ַ���
  */
void USART_SendString_Blocking(USART_Driver_t* const usart, const char* str);
/**
  * @brief  USART����DMA�жϴ�����
  */
void USART_DMA_TX_Interrupt_Handler(USART_Driver_t *usart);
/**
  * @brief  USART����DMA�жϴ�����
  */
void USART_DMA_RX_Interrupt_Handler(USART_Driver_t *usart);
/**
 * @brief  USART���н����жϴ�������֧��DMA+RingBuffer�������ڶ�ʵ����
 */
void USART_RX_IDLE_Interrupt_Handler(USART_Driver_t *usart);
/**
  * @brief  ������д��ָ��USART�����ķ��� RingBuffer��
  */
uint8_t USART_Put_TxData_To_Ringbuffer(USART_Driver_t *usart, const void* data, uint16_t len);
/**
 * @brief  USARTģ�鶨ʱ����������������ѭ��1ms���ڻص�
 */
void USART_Module_Run(USART_Driver_t *usart);
/**
  * @brief  ��ȡUSART����RingBuffer�еĿɶ��ֽ���
  */
uint32_t USART_Get_The_Existing_Amount_Of_Data(USART_Driver_t *usart);
/**
  * @brief  ��USART����RingBuffer�ж�ȡһ���ֽ�����
  */
uint8_t USART_Take_A_Piece_Of_Data(USART_Driver_t *usart, uint8_t* data);
/**
  * @brief  DMA����������Զ��ָ�������������ͳ�ƣ�
  */
void USART_DMA_Error_Recover(USART_Driver_t *usart, uint8_t dir);
/**
 * @brief   ��ʼ��USART����������DMA��RingBuffer���ж�
 */
void USART_Config(USART_Driver_t *usart,
                  uint8_t *rxDMABuffer, uint8_t *rxRBBuffer, uint16_t rxBufSize,
                  uint8_t *txDMABuffer, uint8_t *txRBBuffer, uint16_t txBufSize);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_USART_HAL_H */
