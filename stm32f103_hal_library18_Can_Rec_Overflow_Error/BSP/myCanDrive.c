#include "myCanDrive.h"

volatile uint8_t txmail_free = 0;
volatile uint32_t canSendError = 0;
volatile uint32_t g_RxCount = 0;
volatile uint32_t g_RxOverflowError = 0;
CANMsg_t g_CanMsg = {0,}; // 全局变量，易于观察

/**
  * @brief  通过CAN总线发送固定序列数据（带序列号）。
  * @note   此函数配置标准CAN数据帧（ID: 0x123），发送8字节固定数据(0x01-0x08)，
  *         如果发送邮箱已满，将延迟1ms等待，发送失败时触发错误处理。
  *         适用于需要简单可靠发送固定格式数据的场景。
  * @retval None
  */
void CAN_Send_Msg_Serial(void)
{
    CAN_TxHeaderTypeDef TxHeader;
    uint32_t TxMailbox;
    uint8_t TxData[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    /* 配置CAN发送帧参数 */
    TxHeader.StdId = 0x123;          // 标准标识符，可根据实际需求修改
    TxHeader.ExtId = 0;              // 对于标准帧，扩展标识符无效
    TxHeader.IDE = CAN_ID_STD;       // 标准帧
    TxHeader.RTR = CAN_RTR_DATA;     // 数据帧
    TxHeader.DLC = 8;                // 数据长度为8字节
    TxHeader.TransmitGlobalTime = DISABLE; // 不发送全局时间
    /* 如果没有发送邮箱，延迟等待（如果有实时操作系统，发起调度是更加好的方案） */
    if (HAL_CAN_GetTxMailboxesFreeLevel(&hcan) == 0) {
        LL_mDelay(1);
    }
    /* 发送CAN消息 */
    if (HAL_CAN_AddTxMessage(&hcan, &TxHeader, TxData, &TxMailbox) != HAL_OK) {
    /* 发送失败，进入错误处理 */
        Error_Handler();
    }
}
/**
  * @brief  通过CAN总线发送固定序列数据（无序列号，带邮箱状态检查）。
  * @note   此函数在发送邮箱可用时（txmail_free > 0）配置标准CAN数据帧（ID: 0x200），
  *         发送8字节固定数据(0x01-0x08)，成功后减少可用邮箱计数，
  *         发送失败时触发错误处理，邮箱不可用时返回发送失败状态。
  * @retval uint8_t 发送状态：0表示成功，1表示邮箱不可用，发送失败
  */
uint8_t CAN_Send_Msg_No_Serial(void)
{
    if (txmail_free > 0) {
        CAN_TxHeaderTypeDef TxHeader;
        uint32_t TxMailbox;
        uint8_t TxData[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
        /* 配置CAN发送帧参数 */
        TxHeader.StdId = 0x200;          // 标准标识符，可根据实际需求修改
        TxHeader.ExtId = 0;              // 对于标准帧，扩展标识符无效
        TxHeader.IDE = CAN_ID_STD;       // 标准帧
        TxHeader.RTR = CAN_RTR_DATA;     // 数据帧
        TxHeader.DLC = 8;                // 数据长度为8字节
        TxHeader.TransmitGlobalTime = DISABLE; // 不发送全局时间
        /* 发送CAN消息 */
        if (HAL_CAN_AddTxMessage(&hcan, &TxHeader, TxData, &TxMailbox) != HAL_OK) {
        /* 发送失败，进入错误处理 */
            Error_Handler();
        }
        txmail_free--; // 用掉一个发送邮箱
        return 0;
    } else {
        return 1;
    }
}

/**
  * @brief  配置CAN滤波器，设置为不过滤任何消息
  * @note   此函数使用标识符屏蔽模式，32位滤波器，所有滤波器ID和屏蔽值均设为0
  * @retval None
  */
static void CAN_FilterConfig_AllMessages(CAN_HandleTypeDef *hcan)
{
    CAN_FilterTypeDef filterConfig;

    // 选择滤波器银行（根据实际需求，如果只有一个CAN，一般可用0号滤波器银行）
    filterConfig.FilterBank = 0;
    // 设置为标识符屏蔽位模式
    filterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
    // 设置滤波器为32位模式
    filterConfig.FilterScale = CAN_FILTERSCALE_32BIT;

    // 设置滤波器标识符均为0
    filterConfig.FilterIdHigh = 0x0000;
    filterConfig.FilterIdLow  = 0x0000;
    // 设置屏蔽值为0，即不对任何比特进行过滤
    filterConfig.FilterMaskIdHigh = 0x0000;
    filterConfig.FilterMaskIdLow  = 0x0000;
    
    // 将接收到的消息分配到FIFO0（也可以选择FIFO1，根据实际应用配置）
    filterConfig.FilterFIFOAssignment = CAN_RX_FIFO1;
    // 激活滤波器
    filterConfig.FilterActivation = ENABLE;
    
    // 配置滤波器，配置失败则进入错误处理
    if (HAL_CAN_ConfigFilter(hcan, &filterConfig) != HAL_OK)
    {
        Error_Handler();
    }
}
/**
  * @brief  CAN初始化
  */
void CAN_Config(void)
{
    txmail_free = HAL_CAN_GetTxMailboxesFreeLevel(&hcan); // 获取发送邮箱的空闲数量，一般都是3个
    HAL_CAN_ActivateNotification(&hcan, CAN_IT_TX_MAILBOX_EMPTY); // 启动发送完成中断
    //HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO1_MSG_PENDING); // 启动接收FIFO1中断，当FIFO1中有消息到达时，产生中断
    HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO1_OVERRUN); // 启动接收FIFO1溢出中断
    CAN_FilterConfig_AllMessages(&hcan); // 配置过滤器
    if (HAL_CAN_Start(&hcan) != HAL_OK) {
      Error_Handler(); // 启动失败，进入错误处理
    }
}

/**
  * @brief  CAN接收中断回调函数，处理接收到的CAN消息
  * @param  hcan: CAN句柄
  * @retval None
  */
void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    CAN_RxHeaderTypeDef RxHeader;
    uint8_t RxData[8];
    uint32_t pendingMessages = HAL_CAN_GetRxFifoFillLevel(hcan, CAN_RX_FIFO1); // 获取FIFO1一共有多少个CAN报文
    
    while(pendingMessages) {
        /* 从FIFO1中读取接收消息 */
        if(HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO1, &RxHeader, RxData) == HAL_OK) {
            // 在这里添加对接收到数据的处理代码
            // 例如：调用用户自定义的函数处理数据
            // Process_CAN_Message(&RxHeader, RxData);
            // 例如：将接收到的数据放入Ringbuffer
            
            /* 将接收到的CAN信息放入全局变量，易于观察。实际项目应该在中断将收到的CAN报文放入ringbuffer,然后在主循环处理CAN报文 */
            g_CanMsg.RxHeader = RxHeader;
            memcpy((uint8_t*)g_CanMsg.RxData,(uint8_t*)RxData,8);
            g_RxCount++; // 累加接收到的CAN报文总数
            // 示例：简单地将接收到数据存入一个全局变量或打印（如果调试时有串口或其他调试方式）
        } else {
            // 如果读取消息失败，也可以在这里做错误处理
        }
        pendingMessages = HAL_CAN_GetRxFifoFillLevel(hcan, CAN_RX_FIFO1); // 更新FIFO1里CAN报文的数量
    }
}

void CAN_FIFO1_Overflow_Handler(void)
{
    if (CAN1->RF1R & CAN_RF1R_FOVR1) { // 读取FOVR1位（FIFO1溢出中断），看看是不是被置1
        g_RxOverflowError++;
    }
}

