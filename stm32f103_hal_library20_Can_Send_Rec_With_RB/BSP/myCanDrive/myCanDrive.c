#include "myCanDrive.h"

extern CAN_HandleTypeDef hcan;

volatile uint8_t txmail_free = 0;             // 空闲邮箱的数量
volatile uint32_t canSendError = 0;           // 统计发送失败次数
volatile uint64_t g_RxCount = 0;              // 统计接收的CAN报文总数
volatile uint64_t g_TxCount = 0;              // 统计发送的CAN报文总数
volatile uint64_t g_HandleRxMsg = 0;          // 统计从RX Ringbuffer拿出来处理的CAN报文的数量
volatile uint32_t g_RxOverflowError = 0;      // 统计RX FIFO1溢出数量
volatile uint32_t g_RXRingbufferOverflow = 0; // 统计ringbuffer溢出次数

/* CAN接收ringbuffer */
volatile lwrb_t g_CanRxRBHandler; // 实例化RX ringbuffer
volatile CANMsg_t g_CanRxRBDataBuffer[50] = {0,}; // RX ringbuffer缓存（最多可以存50个CAN消息）

/* CAN发送ringbuffer */
volatile lwrb_t g_CanTxRBHandler; // 实例化TX ringbuffer
volatile CANTXMsg_t g_CanTxRBDataBuffer[50] = {0,}; // TX ringbuffer缓存（最多可以存50个将要发送的CAN消息）

/**
  * @brief  发送一个标准数据帧的CAN消息（串行方式）
  * 
  * 此函数采用串行方式发送标准CAN数据帧。函数内部首先配置发送帧的相关参数，
  * 然后检测发送邮箱是否空闲。如果发送邮箱已经用完，则通过短延时等待后再发送数据。
  * 如果发送失败，则调用Error_Handler()进行错误处理。
  *
  * @param  canid  标准CAN标识符
  * @param  data   指向将要发送的数据数组的指针，数据长度由len参数确定（0~8字节）
  * @param  len    数据字节数（DLC），应不超过8
  * 
  * @note   当发送邮箱不可用时，函数会通过延时（LL_mDelay(1)）等待一段时间，
  *         如果延时后仍无法获得邮箱，则继续等待或最终发送失败。
  * 
  * @retval None
  */
void CAN_Send_STD_DATA_Msg_Serial(uint32_t canid, uint8_t* data, uint8_t len)
{
    uint32_t TxMailbox;
    uint32_t timeout = 100; // 最大等待100ms
    CAN_TxHeaderTypeDef txHeader;
    
    if (len > 8) {
        Error_Handler(); // 根据具体需求，可选择截断或进入错误处理
    }
    
    /* 配置CAN发送帧参数 */
    txHeader.StdId = canid;       // 标准标识符，可根据实际需求修改
    txHeader.ExtId = 0;           // 对于标准帧，扩展标识符无效
    txHeader.IDE = CAN_ID_STD;    // 标准帧
    txHeader.RTR = CAN_RTR_DATA;  // 数据帧
    txHeader.DLC = len;           // 数据长度
    txHeader.TransmitGlobalTime = DISABLE; // 不发送全局时间
    /* 如果没有发送邮箱，延迟等待（如果有实时操作系统，发起调度是更加好的方案） */
    while ((HAL_CAN_GetTxMailboxesFreeLevel(&hcan) == 0) && (timeout > 0)) {
        LL_mDelay(1);
        timeout--;
    }
    /* 判断是否超时未获得邮箱 */
    if ((timeout == 0) && (HAL_CAN_GetTxMailboxesFreeLevel(&hcan) == 0)) {
        canSendError++; // 记录一次发送超时错误
        return;
    }
    /* 发送CAN消息 */
    if (HAL_CAN_AddTxMessage(&hcan, &txHeader, data, &TxMailbox) != HAL_OK) {
    /* 发送失败，进入错误处理 */
        Error_Handler();
    }
    g_TxCount++; // 成功发送一帧CAN报文
}

/**
  * @brief  发送一个标准数据帧的CAN消息（非串行方式）
  * 
  * 此函数采用非串行方式发送标准CAN数据帧。发送前首先检查全局变量txmail_free，
  * 判断是否有空闲的发送邮箱。如果有，则配置帧参数并发送消息，同时减少空闲邮箱计数；
  * 否则，增加canSendError计数以记录错误，并返回错误代码。
  *
  * @param  canid  标准CAN标识符
  * @param  data   指向将要发送的数据数组的指针，数据长度由len参数确定（0~8字节）
  * @param  len    数据字节数（DLC），应不超过8
  *
  * @note   函数依赖全局变量txmail_free来检测发送邮箱的空闲状态，
  *         当发送邮箱不足时，会增加全局变量canSendError计数，并返回错误。
  * 
  * @retval uint8_t  返回0表示发送成功；返回1表示发送邮箱已满，未能发送CAN消息
  */
uint8_t CAN_Send_STD_DATA_Msg_No_Serial(uint32_t canid, uint8_t* data, uint8_t len)
{
    if (len > 8) {
        Error_Handler(); // 根据具体需求，可选择截断或进入错误处理
    }
    
    if (txmail_free > 0) {
        uint32_t TxMailbox;
        CAN_TxHeaderTypeDef txHeader;
        /* 配置CAN发送帧参数 */
        txHeader.StdId = canid;      // 标准标识符，可根据实际需求修改
        txHeader.ExtId = 0;          // 对于标准帧，扩展标识符无效
        txHeader.IDE = CAN_ID_STD;   // 标准帧
        txHeader.RTR = CAN_RTR_DATA; // 数据帧
        txHeader.DLC = len;          // 数据长度
        txHeader.TransmitGlobalTime = DISABLE; // 不发送全局时间
        /* 发送CAN消息 */
        if (HAL_CAN_AddTxMessage(&hcan, &txHeader, data, &TxMailbox) != HAL_OK) {
        /* 发送失败，进入错误处理 */
            Error_Handler();
        }
        txmail_free--; // 用掉一个发送邮箱
        g_TxCount++; // 成功发送一帧CAN报文
        return 0;
    } else {
        canSendError++; // 发送邮箱满了，溢出
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
    if (HAL_CAN_ConfigFilter(hcan, &filterConfig) != HAL_OK) {
        Error_Handler();
    }
}

/**
 * @brief  从环形缓冲区中读取并处理所有挂起的 CAN 消息
 * @details 本函数轮询全局环形缓冲区 g_CanRxRBHandler，只要其中可用数据
 *          大于或等于一个完整的 CANMsg_t 消息长度，就不断：
 *            1. 将一条消息读取到本地变量 canMsg
 *            2. 全局计数器 g_HandleRxMsg 自增，记录已处理的消息数
 *            3. 根据 canMsg.RxHeader.StdId 在 switch-case 中分发到具体的业务处理分支
 * @note   - 用户需在 switch-case 中填充不同 StdId 对应的处理逻辑  
 *         - 若无更多消息可读，函数返回  
 * @retval None
 */
void CAN_Data_Process(void)
{
    /* 如果至少有一帧CAN消息，进入while循环，从ringbuffer取出CAN报文，接着分析，处理。 */
    while(lwrb_get_full((lwrb_t*)&g_CanRxRBHandler) >= sizeof(CANMsg_t)) {
        CANMsg_t canMsg;
        lwrb_read((lwrb_t*)&g_CanRxRBHandler, &canMsg, sizeof(CANMsg_t)); // 从ringbuffer中读取一条CAN消息
        g_HandleRxMsg++; // 有一条CAN消息从ringbuffer拿出来处理
        /* 在这里，根据CANID处理业务代码.等等 */
//        switch(canMsg.RxHeader.StdId) {
//            case 0x200:
//                // TODO: 处理 ID = 0x200 的消息
//                break;
//            case 0x201:
//                // TODO: 处理 ID = 0x201 的消息
//                break;
//            case 0x202:
//            case 0x203:
//                break;
//            default:
//                break;
//        }
    }
}
/**
 * @brief 尝试发送一帧待发 CAN 报文（从 TX RingBuffer → CAN 邮箱）
 *
 * 按照「先 peek、成功后 skip」的 0 丢帧策略：  
 * 1. 若没有空闲邮箱或 RingBuffer 中无完整报文，则立即返回。  
 * 2. `lwrb_peek()` 仅复制最前面的报文；  
 * 3. 调用 @ref CAN_Send_STD_DATA_Msg_No_Serial 发送；  
 * 4. 仅在发送成功时执行 `lwrb_skip()` 真正弹出这帧。  
 *
 * @note  
 * - 该函数既可在中断环境，也可在主循环环境调用；调用方需自行保证
 *   与 RingBuffer 相关的并发互斥（示例代码已在主循环临时关闭
 *   `CAN_IT_TX_MAILBOX_EMPTY`）。  
 * - 发送失败仅累加 @p canSendError 计数，不会移除报文，等待下次重试。  
 *
 * @retval true   成功将 1 帧报文写入邮箱，并已从 RingBuffer 移除  
 * @retval false  无可用邮箱 / 无待发送数据 / 发送失败
 */
static bool CAN_TxOnceFromRB(void)
{
    CANTXMsg_t msg;
    if (txmail_free == 0) {
        return false; // 没邮箱
    }
    if (lwrb_get_full((lwrb_t*)&g_CanTxRBHandler) < sizeof(CANTXMsg_t)) {
        return false; // 没数据
    }
    /* 将TX ringbuffer的消息搬运到TX发送邮箱 */
    lwrb_peek((lwrb_t*)&g_CanTxRBHandler, 0, &msg, sizeof(CANTXMsg_t)); // 只窥视(peek)一帧，不弹出
    if (CAN_Send_STD_DATA_Msg_No_Serial(msg.CanId, msg.data, msg.Len) == 0) {
        lwrb_skip((lwrb_t*)&g_CanTxRBHandler, sizeof(CANTXMsg_t)); // 成功：正式把这帧从 RingBuffer 删掉
        return true;
    }
    canSendError++; // 理论上很少到这里
    return false;
}

/**
 * @brief     主循环周期调用的批量补发函数
 *
 * 为避免与 ISR 并发访问同一 TX RingBuffer：  
 * - 进入函数前暂时关闭 `CAN_IT_TX_MAILBOX_EMPTY` 中断；  
 * - 通过循环调用 @ref CAN_TxOnceFromRB 将所有空闲邮箱填满；  
 * - 结束后重新开启中断。  
 *
 * @note 建议在 1 ms 或更快的系统节拍内调用一次，以防极端负载下
 *       ISR 频率不足以清空 RingBuffer。
 */
void CAN_Get_CANMsg_From_RB_To_TXMailBox(void)
{
    // 在主循环也要更新发送邮箱的数量
    txmail_free = ((hcan.Instance->TSR & CAN_TSR_TME0) ? 1 : 0) +
             ((hcan.Instance->TSR & CAN_TSR_TME1) ? 1 : 0) +
             ((hcan.Instance->TSR & CAN_TSR_TME2) ? 1 : 0);
    
    /* ----------- 早退出：无数据 或 无空闲邮箱 ----------- */
    if (txmail_free == 0) {
        return; // 邮箱全满
    }
    if (lwrb_get_full((lwrb_t*)&g_CanTxRBHandler) < sizeof(CANTXMsg_t)) {
        return; // 缓冲区没数据
    }
    
    
    
    /* 关 TX-Mailbox-Empty 中断，防止并发 */
    __HAL_CAN_DISABLE_IT(&hcan, CAN_IT_TX_MAILBOX_EMPTY);
    /* 把仍然空着的邮箱一次发完 */
    while (CAN_TxOnceFromRB()) { }
    /* 恢复中断 */
    __HAL_CAN_ENABLE_IT(&hcan, CAN_IT_TX_MAILBOX_EMPTY);
}

/**
 * @brief  将一帧标准 CAN 数据帧写入 Tx 环形缓冲区
 *
 * 上层应用调用此函数即可“排队”发送一帧标准 CAN 报文；  
 * 发送驱动（中断 + 主循环）会异步地把缓冲区中的报文
 * 取出并写入 CAN1 的 3 个发送邮箱，实现**非阻塞发送**。
 *
 * @param[in]  canid  11 位标准标识符（范围 0 – 0x7FF）
 * @param[in]  data   指向待发送数据的缓冲区（0 – 8 字节）
 * @param[in]  len    数据长度（DLC），取值 0 – 8
 *
 * @retval true   已成功把本帧写入 Tx RingBuffer  
 * @retval false  Tx RingBuffer 空间不足，当前帧被丢弃
 *
 * @note
 * - **线程安全**：本函数可在主循环或任务上下文中调用；  
 *   由于硬中断只读缓冲区而不写，且 @p lwrb_write() 已实现
 *   读写并发安全，因此无需额外临界区。  
 * - 当缓冲区满时本实现直接返回 @c false 而不覆盖旧数据；若
 *   需要“新数据优先”策略，可先调用 @p lwrb_skip() 弹出最旧
 *   一帧后再写入，并应在代码中作明确注释。 
 */
bool CAN_Send_CAN_STD_Message(uint32_t canid, uint8_t* data, uint8_t len)
{
    if (lwrb_get_free((lwrb_t*)&g_CanTxRBHandler) < sizeof(CANTXMsg_t)) {
        return false;
    }
    /* 往TX Ringbuffer里放入一帧CAN报文 */
    CANTXMsg_t canMsg = { .CanId = canid, .Len = len };
    memcpy(canMsg.data, data, len);
    lwrb_write((lwrb_t*)&g_CanTxRBHandler, &canMsg, sizeof(CANTXMsg_t)); 
    return true;
}

/**
  * @brief  CAN初始化
  */
void CAN_Config(void)
{
    lwrb_init((lwrb_t*)&g_CanRxRBHandler, (uint8_t*)g_CanRxRBDataBuffer, sizeof(g_CanRxRBDataBuffer)); // RX Ringbuffer初始化
    lwrb_init((lwrb_t*)&g_CanTxRBHandler, (uint8_t*)g_CanTxRBDataBuffer, sizeof(g_CanTxRBDataBuffer)); // TX Ringbuffer初始化
    
    txmail_free = HAL_CAN_GetTxMailboxesFreeLevel(&hcan); // 获取发送邮箱的空闲数量，一般都是3个
    HAL_CAN_ActivateNotification(&hcan, CAN_IT_TX_MAILBOX_EMPTY); // 启动发送完成中断
    HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO1_MSG_PENDING); // 启动接收FIFO1中断，当FIFO1中有消息到达时，产生中断
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
    CANMsg_t canMsg = {0,};
    uint32_t pendingMessages = HAL_CAN_GetRxFifoFillLevel(hcan, CAN_RX_FIFO1); // 获取FIFO1一共有多少个CAN报文
    
    while(pendingMessages) {
        /* 从FIFO1中读取接收消息 */
        if(HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO1, &canMsg.RxHeader, canMsg.RxData) == HAL_OK) {
            g_RxCount++; // 累加接收到的CAN报文总数
            if(lwrb_get_free((lwrb_t*)&g_CanRxRBHandler) < sizeof(CANMsg_t)) { // 判断ringbuffer是否被挤满
                g_RXRingbufferOverflow++;    // 累加ringbuffer溢出全局计数器
                lwrb_reset((lwrb_t*)&g_CanRxRBHandler); // 清空ringbuffer
            }
            lwrb_write((lwrb_t*)&g_CanRxRBHandler, &canMsg, sizeof(CANMsg_t)); // 将CAN报文放入ringbuffer
        } else {
            // 如果读取消息失败，也可以在这里做错误处理
        }
        pendingMessages = HAL_CAN_GetRxFifoFillLevel(hcan, CAN_RX_FIFO1); // 更新FIFO1里CAN报文的数量
    }
}

/**
  * @brief  CAN溢出中断处理函数，在全局中断CAN1_RX1_IRQHandler()里调用
  * @param  Note
  * @retval None
  */
void CAN_FIFO1_Overflow_Handler(void)
{
    if (CAN1->RF1R & CAN_RF1R_FOVR1) { // 读取FOVR1位（FIFO1溢出中断），看看是不是被置1
        g_RxOverflowError++;
    }
}

/**
  * @brief  公共的 CAN 发送完成回调
  * @note   HAL 在 USB_HP_CAN1_TX_IRQHandler()→HAL_CAN_IRQHandler() 里，
  *         会依次检测 RQCP0/1/2 并分别调用 3 个弱回调。
  *         我们让它们全部别名到此函数即可。
  */
void CAN_TxCompleteCommonCallback(CAN_HandleTypeDef *hcan)
{
    /* 通过寄存器方式（更加高效），更新发送邮箱的空闲数量 */
    txmail_free = ((hcan->Instance->TSR & CAN_TSR_TME0) ? 1 : 0) +
                 ((hcan->Instance->TSR & CAN_TSR_TME1) ? 1 : 0) +
                 ((hcan->Instance->TSR & CAN_TSR_TME2) ? 1 : 0);
    /* 将TX ringbuffer的一条CAN报文搬到CAN TX邮箱 */
    CAN_TxOnceFromRB();
}

/* ---------- 把 3 个弱回调全部映射到同一个实现 ---------- */
void HAL_CAN_TxMailbox0CompleteCallback(CAN_HandleTypeDef *hcan)
        __attribute__((alias("CAN_TxCompleteCommonCallback")));
void HAL_CAN_TxMailbox1CompleteCallback(CAN_HandleTypeDef *hcan)
        __attribute__((alias("CAN_TxCompleteCommonCallback")));
void HAL_CAN_TxMailbox2CompleteCallback(CAN_HandleTypeDef *hcan)
        __attribute__((alias("CAN_TxCompleteCommonCallback")));

