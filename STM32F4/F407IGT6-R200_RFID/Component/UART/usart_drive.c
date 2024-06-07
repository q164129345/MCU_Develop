#include "usart_drive.h"
#include "usart.h"

#define DEBUG 1
#if DEBUG == 1
#define usart_printf printf
#else
#define usart_printf(format, ...)
#endif

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
 * @brief ��ȡ����������ڴ��ַ
 * 
 * @param me usart_drive����ָ��
 * @@return USART_TypeDef* ����������ڴ��ַ
 */
static USART_TypeDef* Usart_Get_Huart_Instance(struct usart_drive* me)
{
    return me->huartInstance;
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
    HAL_StatusTypeDef result;
    uint32_t startTick = HAL_GetTick(); // ��ȡ��ǰʱ���
    
    if (me->flagTxComplete == 0) {
        // ��һ�δ��仹δ���
        return HAL_BUSY;
    }
    me->flagTxComplete = 0;  // �����־λ
    result = HAL_UART_Transmit_DMA(me->huart, pData, Size);
    
    while(0 == me->flagTxComplete) {
        if((HAL_GetTick() - startTick) > 2000U) { // ��ʱʱ��2S
            return HAL_TIMEOUT; // ��ʱ����
        }
    }
    return result;
}

/**
 * @brief ����USART ���з���
 * 
 * @param me usart_drive����ָ��
 * @param pData ָ�����ݻ�������ָ��
 * @param Size ���ݴ�С
 * @return HAL_StatusTypeDef HAL״̬
 */
static HAL_StatusTypeDef USART_Start_Transmit(struct usart_drive* me, uint8_t *pData, uint16_t Size)
{
    if (NULL == me) return HAL_ERROR;
    return HAL_UART_Transmit(me->huart, pData, Size, 0xFFFFFFFF);
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
    if (me->huart->Instance == me->huartInstance)
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
#if DEBUG == 1
                // ��ӡ������Ϣ
                char tempBuffer[USART_RX_BUF_SIZE + 1];
                memcpy(tempBuffer, (unsigned char*)me->rxData, me->receivedBytes);
                tempBuffer[me->receivedBytes] = '\0'; // ȷ���ַ����Կ��ַ���β
                usart_printf("Msg number:%d \n",++msgNum);
                usart_printf("Received string: %s, Bytes: %d\n", tempBuffer, me->receivedBytes); // ��ӡ�ַ���
                // ��16���ƴ�ӡ����
                usart_printf("Received data in hex: ");
                for (uint16_t i = 0; i < me->receivedBytes; i++) {
                    usart_printf("%02X ", tempBuffer[i]);
                }
                usart_printf("\n");
#endif
            } else {
                // �������������
                printf("Received data overflow! Bytes: %d\n", me->receivedBytes);
                me->receivedBytes = 0; // ���ý����ֽ���
            }
            // ������յ������ݣ������ݷ���ringbuffer����RTOS����Ϣ���У���Ҫ���ж���������ģ���������������
            if (me->receivedBytes > 0) {
                uint16_t freeSpace = Q_BUFFER_SIZE - Queue_Count(&me->queueHandler); // ����ʣ��ռ�
                if (me->receivedBytes > freeSpace) {
                    printf("Ringbuffer is full and needs to be emptied.\n");
                    Queue_Clear(&me->queueHandler); // ��λringbuffer��дָ��
                    memset((uint8_t*)me->queueBuffer, 0, Q_BUFFER_SIZE); // ringbuffer������0����ѡ��
                }
                uint8_t pushNum = Queue_Push_Array(&me->queueHandler, (uint8_t*)me->rxData, me->receivedBytes);
                //printf(" %d bytes data push into ringbuffer.\n", pushNum);
            }
            // ���Ը�����Ŀ����������ݴ����߼�
            USART_Start_DMA_Receive(me); // ��������DMA����
        }
    }
}

/**
 * @brief ��ȡringbuffer�����Ϣ����
 * 
 * @param me usart_drive����ָ��
 * @@return uint16_t ringbuffer����Ϣ����
 */
static uint16_t Usart_Get_The_Number_Of_Data_In_Queue(struct usart_drive* me)
{
    return Queue_Count(&me->queueHandler);
}

/**
 * @brief USART���������ʼ��
 * 
 * @param me usart_drive����ָ��
 * @param huart UART���
 */
void Usart_Drive_Object_Init(struct usart_drive* me, UART_HandleTypeDef *huart, USART_TypeDef* const instance)
{
    if (huart == NULL) return;
    
    me->huart = huart; // ������
    
    me->flagTxComplete = 0x01;
    me->huartInstance = instance;
    
    /* ���󷽷���ʼ�� */
    me->DMA_Sent = USART_Start_DMA_Transmit;
    me->Serial_Sent = USART_Start_Transmit;
    me->User_IDLE_Callback = USER_UART_IDLE_Callback;
    // ������
    me->Set_Flag_Tx_Complete = USART_Set_Flag_Tx_Complete;
    // ��ȡ��
    me->Get_Flag_Tx_Complete = USART_Get_Flag_Tx_Complete;
    me->Get_The_Number_Of_Data_In_Queue = Usart_Get_The_Number_Of_Data_In_Queue;
    me->Get_Huart_Instance = Usart_Get_Huart_Instance;
    
    Queue_Init(&me->queueHandler, (uint8_t*)me->queueBuffer, Q_BUFFER_SIZE);
    __HAL_UART_ENABLE_IT(me->huart, UART_IT_IDLE); // �������տ����ж�
    USART_Start_DMA_Receive(me); // ����DMA����
}

