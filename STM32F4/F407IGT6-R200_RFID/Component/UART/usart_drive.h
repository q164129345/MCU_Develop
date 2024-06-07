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
    /* ��Ա */
    UART_HandleTypeDef* huart;                  // HAL������
    USART_TypeDef*      huartInstance;          // ���������ڴ��ַ
    volatile uint8_t    rxData[USART_RX_BUF_SIZE]; // DMA���ջ�����
    volatile uint8_t    flagTxComplete;         // ��־λ���������
    volatile int16_t    receivedBytes;          // ���յ��ֽ���
    
    QUEUE_HandleTypeDef queueHandler;               // ringbuffer���
    volatile uint8_t    queueBuffer[Q_BUFFER_SIZE]; // ringbuffer����
    
    /* ���� */
    void (*User_IDLE_Callback) (struct usart_drive* me); // �����жϴ���
    HAL_StatusTypeDef (*DMA_Sent) (struct usart_drive* me, uint8_t *pData, uint16_t Size); // DMA����
    HAL_StatusTypeDef (*Serial_Sent) (struct usart_drive* me, uint8_t *pData, uint16_t Size); // ���з���
    
    // ������
    void (*Set_Flag_Tx_Complete) (struct usart_drive* me, uint8_t flag);  // ���ñ�־λ(������ɣ�
    // ��ȡ��
    uint8_t (*Get_Flag_Tx_Complete) (struct usart_drive* me);             // ��ȡ��־λ��������ɣ�
    uint16_t (*Get_The_Number_Of_Data_In_Queue) (struct usart_drive* me); // ��ȡringbuffer�����Ϣ����
    USART_TypeDef* (*Get_Huart_Instance) (struct usart_drive* me);        // ��ȡ����������ڴ��ַ
    
}Usart_Drive;

// ����ʵ����
void Usart_Drive_Object_Init(struct usart_drive* me, UART_HandleTypeDef *huart, USART_TypeDef* const instance);


#ifdef __cplusplus
} 
#endif



#endif /* __USART1_DRIVE_H */
