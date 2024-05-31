#ifndef _USART1_DRIVE_H_
#define _USART1_DRIVE_H_

#include "stm32f4xx_hal.h"
#include "main.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "stdbool.h"
#include "queue.h"

#ifdef __cplusplus
extern "C" {
#endif

#define USART_RX_BUF_SIZE 128
#define Q_BUFFER_SIZE 256

typedef struct usart_drive{
    /* 成员 */
    UART_HandleTypeDef* huart;                  // HAL对象句柄
    volatile uint8_t    rxData[USART_RX_BUF_SIZE]; // DMA接收缓存区
    volatile uint8_t    flagTxComplete;         // 标志位：发送完成
    volatile int16_t    receivedBytes;          // 接收的字节数
    
    QUEUE_HandleTypeDef queueHandler;               // ringbuffer句柄
    volatile uint8_t    queueBuffer[Q_BUFFER_SIZE]; // ringbuffer缓存
    
    /* 方法 */
    void (*User_IDLE_Callback) (struct usart_drive* me); // 空闲中断处理
    HAL_StatusTypeDef (*DMA_Sent) (struct usart_drive* me, uint8_t *pData, uint16_t Size); // DMA发送
    
    // 设置器
    void (*Set_Flag_Tx_Complete) (struct usart_drive* me, uint8_t flag); // 设置标志位(发送完成）
    // 获取器
    uint8_t (*Get_Flag_Tx_Complete) (struct usart_drive* me); // 获取标志位（发送完成）
    
}Usart_Drive;

// 对象实例化
void Usart_Drive_Object_Init(struct usart_drive* me, UART_HandleTypeDef *huart);


#ifdef __cplusplus
} 
#endif



#endif /* __USART1_DRIVE_H */
