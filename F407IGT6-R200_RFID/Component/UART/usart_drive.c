#include "usart_drive.h"
#include "usart.h"

#define DEBUG 1
#if DEBUG == 1
#define usart_printf printf
#else
#define usart_printf(format, ...)
#endif

/**
 * @brief 启动USART DMA接收
 * 
 * @param me usart_drive对象指针
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
 * @brief 获取串口外设的内存地址
 * 
 * @param me usart_drive对象指针
 * @@return USART_TypeDef* 串口外设的内存地址
 */
static USART_TypeDef* Usart_Get_Huart_Instance(struct usart_drive* me)
{
    return me->huartInstance;
}

/**
 * @brief 设置发送完成标志位
 * 
 * @param me usart_drive对象指针
 * @param flag 标志位值
 */
static void USART_Set_Flag_Tx_Complete(struct usart_drive* me, uint8_t flag)
{ 
    if (NULL == me) return; 
    me->flagTxComplete = flag > 0 ? true : false; 
}

/**
 * @brief 获取发送完成标志位
 * 
 * @param me usart_drive对象指针
 * @return uint8_t 标志位值
 */
static uint8_t USART_Get_Flag_Tx_Complete(struct usart_drive* me)
{ 
    if (NULL == me) return false;
    return me->flagTxComplete;
}

/**
 * @brief 启动USART DMA发送
 * 
 * @param me usart_drive对象指针
 * @param pData 指向数据缓冲区的指针
 * @param Size 数据大小
 * @return HAL_StatusTypeDef HAL状态
 */
static HAL_StatusTypeDef USART_Start_DMA_Transmit(struct usart_drive* me, uint8_t *pData, uint16_t Size)
{
    if (NULL == me) return HAL_ERROR;
    HAL_StatusTypeDef result;
    uint32_t startTick = HAL_GetTick(); // 获取当前时间戳
    
    if (me->flagTxComplete == 0) {
        // 上一次传输还未完成
        return HAL_BUSY;
    }
    me->flagTxComplete = 0;  // 清除标志位
    result = HAL_UART_Transmit_DMA(me->huart, pData, Size);
    
    while(0 == me->flagTxComplete) {
        if((HAL_GetTick() - startTick) > 2000U) { // 超时时间2S
            return HAL_TIMEOUT; // 超时处理
        }
    }
    return result;
}

/**
 * @brief 启动USART 串行发送
 * 
 * @param me usart_drive对象指针
 * @param pData 指向数据缓冲区的指针
 * @param Size 数据大小
 * @return HAL_StatusTypeDef HAL状态
 */
static HAL_StatusTypeDef USART_Start_Transmit(struct usart_drive* me, uint8_t *pData, uint16_t Size)
{
    if (NULL == me) return HAL_ERROR;
    return HAL_UART_Transmit(me->huart, pData, Size, 0xFFFFFFFF);
}

/**
 * @brief UART空闲中断回调函数
 * 
 * @param me usart_drive对象指针
 */
static void USER_UART_IDLE_Callback(struct usart_drive* me)
{
    if (NULL == me) return;
    static uint32_t msgNum = 0;
    if (me->huart->Instance == me->huartInstance)
    {
        if (RESET != __HAL_UART_GET_FLAG(me->huart, UART_FLAG_IDLE)) // 判断是不是空闲中断
        {
            // 清除空闲中断标志
            __HAL_UART_CLEAR_IDLEFLAG(me->huart);
            // 停止DMA接收
            HAL_UART_DMAStop(me->huart);
            // 计算接收到的字节数
            me->receivedBytes = USART_RX_BUF_SIZE - __HAL_DMA_GET_COUNTER(me->huart->hdmarx);
            
            // 检查接收到的字节数是否超过缓冲区大小
            if (me->receivedBytes > 0 && me->receivedBytes <= USART_RX_BUF_SIZE) {
#if DEBUG == 1
                // 打印调试信息
                char tempBuffer[USART_RX_BUF_SIZE + 1];
                memcpy(tempBuffer, (unsigned char*)me->rxData, me->receivedBytes);
                tempBuffer[me->receivedBytes] = '\0'; // 确保字符串以空字符结尾
                usart_printf("Msg number:%d \n",++msgNum);
                usart_printf("Received string: %s, Bytes: %d\n", tempBuffer, me->receivedBytes); // 打印字符串
                // 以16进制打印出来
                usart_printf("Received data in hex: ");
                for (uint16_t i = 0; i < me->receivedBytes; i++) {
                    usart_printf("%02X ", tempBuffer[i]);
                }
                usart_printf("\n");
#endif
            } else {
                // 处理接收溢出情况
                printf("Received data overflow! Bytes: %d\n", me->receivedBytes);
                me->receivedBytes = 0; // 重置接收字节数
            }
            // 处理接收到的数据，将数据放入ringbuffer或者RTOS的消息队列，不要在中断里解析报文，并运行其他程序
            if (me->receivedBytes > 0) {
                uint16_t freeSpace = Q_BUFFER_SIZE - Queue_Count(&me->queueHandler); // 计算剩余空间
                if (me->receivedBytes > freeSpace) {
                    printf("Ringbuffer is full and needs to be emptied.\n");
                    Queue_Clear(&me->queueHandler); // 复位ringbuffer读写指针
                    memset((uint8_t*)me->queueBuffer, 0, Q_BUFFER_SIZE); // ringbuffer缓存清0（可选）
                }
                uint8_t pushNum = Queue_Push_Array(&me->queueHandler, (uint8_t*)me->rxData, me->receivedBytes);
                //printf(" %d bytes data push into ringbuffer.\n", pushNum);
            }
            // 可以根据项目需求添加数据处理逻辑
            USART_Start_DMA_Receive(me); // 重新启动DMA接收
        }
    }
}

/**
 * @brief 获取ringbuffer里的消息数量
 * 
 * @param me usart_drive对象指针
 * @@return uint16_t ringbuffer的消息数量
 */
static uint16_t Usart_Get_The_Number_Of_Data_In_Queue(struct usart_drive* me)
{
    return Queue_Count(&me->queueHandler);
}

/**
 * @brief USART驱动对象初始化
 * 
 * @param me usart_drive对象指针
 * @param huart UART句柄
 */
void Usart_Drive_Object_Init(struct usart_drive* me, UART_HandleTypeDef *huart, USART_TypeDef* const instance)
{
    if (huart == NULL) return;
    
    me->huart = huart; // 对象句柄
    
    me->flagTxComplete = 0x01;
    me->huartInstance = instance;
    
    /* 对象方法初始化 */
    me->DMA_Sent = USART_Start_DMA_Transmit;
    me->Serial_Sent = USART_Start_Transmit;
    me->User_IDLE_Callback = USER_UART_IDLE_Callback;
    // 设置器
    me->Set_Flag_Tx_Complete = USART_Set_Flag_Tx_Complete;
    // 获取器
    me->Get_Flag_Tx_Complete = USART_Get_Flag_Tx_Complete;
    me->Get_The_Number_Of_Data_In_Queue = Usart_Get_The_Number_Of_Data_In_Queue;
    me->Get_Huart_Instance = Usart_Get_Huart_Instance;
    
    Queue_Init(&me->queueHandler, (uint8_t*)me->queueBuffer, Q_BUFFER_SIZE);
    __HAL_UART_ENABLE_IT(me->huart, UART_IT_IDLE); // 开启接收空闲中断
    USART_Start_DMA_Receive(me); // 启动DMA接收
}

