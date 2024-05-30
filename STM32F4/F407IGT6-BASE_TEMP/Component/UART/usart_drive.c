#include "usart_drive.h"
#include "usart.h"

/**
 * @brief ����USART DMA����
 * 
 * @param me usart_drive����ָ��
 */
static void USART_Start_DMA_Receive(struct usart_drive* me)
{
    if (NULL == me) return;
    if (HAL_UART_Receive_DMA(me->huart, (uint8_t *)me->rxData, USART_RX_BUF_SIZE) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
 * @brief ���÷�����ɱ�־λ
 * 
 * @param me usart_drive����ָ��
 * @param flag ��־λֵ
 */
static void USART_Set_Flag_Tx_Complete(struct usart_drive* me, uint8_t flag)
{ 
    if (NULL == me) return; 
    me->flagTxComplete = flag > 0 ? true : false; 
}

/**
 * @brief ��ȡ������ɱ�־λ
 * 
 * @param me usart_drive����ָ��
 * @return uint8_t ��־λֵ
 */
static uint8_t USART_Get_Flag_Tx_Complete(struct usart_drive* me)
{ 
    if (NULL == me) return false;
    return me->flagTxComplete;
}

/**
 * @brief ����USART DMA����
 * 
 * @param me usart_drive����ָ��
 * @param pData ָ�����ݻ�������ָ��
 * @param Size ���ݴ�С
 * @return HAL_StatusTypeDef HAL״̬
 */
static HAL_StatusTypeDef USART_Start_DMA_Transmit(struct usart_drive* me, uint8_t *pData, uint16_t Size)
{
    if (NULL == me) return HAL_ERROR;
    if (me->flagTxComplete == 0) {
        // ��һ�δ��仹δ���
        return HAL_BUSY;
    }
    me->flagTxComplete = 0;  // �����־λ
    return HAL_UART_Transmit_DMA(me->huart, pData, Size);
}

/**
 * @brief UART�����жϻص�����
 * 
 * @param me usart_drive����ָ��
 */
static void USER_UART_IDLE_Callback(struct usart_drive* me)
{
    if (NULL == me) return;
    static uint32_t msgNum = 0;
    if (me->huart->Instance == USART1)
    {
        if (RESET != __HAL_UART_GET_FLAG(me->huart, UART_FLAG_IDLE)) // �ж��ǲ��ǿ����ж�
        {
            // ��������жϱ�־
            __HAL_UART_CLEAR_IDLEFLAG(me->huart);
            // ֹͣDMA����
            HAL_UART_DMAStop(me->huart);
            // ������յ����ֽ���
            me->receivedBytes = USART_RX_BUF_SIZE - __HAL_DMA_GET_COUNTER(me->huart->hdmarx);
            
            // �����յ����ֽ����Ƿ񳬹���������С
            if (me->receivedBytes > 0 && me->receivedBytes <= USART_RX_BUF_SIZE) {
                // ��ӡ������Ϣ
                char tempBuffer[USART_RX_BUF_SIZE + 1];
                memcpy(tempBuffer, (unsigned char*)me->rxData, me->receivedBytes);
                tempBuffer[me->receivedBytes] = '\0'; // ȷ���ַ����Կ��ַ���β
                printf("Msg number:%d \n",++msgNum);
                printf("Received string: %s, Bytes: %d\n", tempBuffer, me->receivedBytes); // ��ӡ�ַ���
                // ��16���ƴ�ӡ����
                printf("Received data in hex: ");
                for (uint16_t i = 0; i < me->receivedBytes; i++) {
                    printf("%02X ", tempBuffer[i]);
                }
                printf("\n");
            } else {
                // �������������
                printf("Received data overflow! Bytes: %d\n", me->receivedBytes);
                me->receivedBytes = 0; // ���ý����ֽ���
            }
            // ������յ������ݣ������ݷ���ringbuffer����RTOS����Ϣ���У���Ҫ���ж���������ģ���������������
            
            // ���Ը�����Ŀ����������ݴ����߼�
            USART_Start_DMA_Receive(me); // ��������DMA����
        }
    }
}

/**
 * @brief USART���������ʼ��
 * 
 * @param me usart_drive����ָ��
 * @param huart UART���
 */
void Usart_Drive_Object_Init(struct usart_drive* me, UART_HandleTypeDef *huart)
{
    if (huart == NULL) return;
    
    me->huart = huart; // ������
    
    me->flagTxComplete = 0x01;
    
    /* ���󷽷���ʼ�� */
    me->DMA_Sent = USART_Start_DMA_Transmit;
    me->Get_Flag_Tx_Complete = USART_Get_Flag_Tx_Complete;
    me->Set_Flag_Tx_Complete = USART_Set_Flag_Tx_Complete;
    me->User_IDLE_Callback = USER_UART_IDLE_Callback;
    
    __HAL_UART_ENABLE_IT(me->huart, UART_IT_IDLE); // �������տ����ж�
    USART_Start_DMA_Receive(me); // ����DMA����
}

