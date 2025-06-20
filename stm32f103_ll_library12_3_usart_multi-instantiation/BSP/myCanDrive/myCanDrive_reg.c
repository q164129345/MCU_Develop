#include "myCanDrive_reg.h"

volatile uint8_t txmail_free = 0;        // 空闲邮箱的数量
volatile uint32_t canSendError = 0;      // 统计发送失败次数
CAN_ESR_t gCanESR = {0,};                // 解析错误
volatile uint64_t g_RxCount = 0;         // 统计接收报文总数
volatile uint64_t g_TxCount = 0;         // 统计发送报文总数
volatile uint64_t g_HandleRxMsg = 0;     // 统计从RX Ringbuffer拿出来处理的CAN报文的数量
volatile uint32_t g_RxOverflowError = 0; // 记录接收溢出错误
volatile uint32_t g_RXRingbufferOverflow = 0; // 统计ringbuffer溢出次数

/* CAN接收ringbuffer */
volatile lwrb_t g_CanRxRBHandler; // 实例化ringbuffer
volatile CANMsg_t g_CanRxRBDataBuffer[50] = {0,}; // ringbuffer缓存（最多可以存50个CAN消息）

/* CAN发送ringbuffer */
volatile lwrb_t g_CanTxRBHandler; // 实例化TX ringbuffer
volatile CANTXMsg_t g_CanTxRBDataBuffer[50] = {0,}; // TX ringbuffer缓存（最多可以存50个将要发送的CAN消息）


/**
  * @brief  配置CAN滤波器，设置为不过滤任何消息
  * @note   此函数使用标识符屏蔽模式，32位滤波器，所有滤波器ID和屏蔽值均设为0
  * @retval None
  */
static void CAN_FilterConfig_AllMessages()
{
    /* 配置滤波器0，FIFO1 */
    CAN1->FMR |= (1UL << 0);   // 进入滤波器初始化模式
    CAN1->FM1R  &= ~(1UL << 0); // 滤波器0使用屏蔽位模式
    CAN1->sFilterRegister[0].FR1 = 0x00000000;
    CAN1->sFilterRegister[0].FR2 = 0x00000000;
    CAN1->FFA1R |= (1UL << 0);  // 分配到 FIFO1
    CAN1->FA1R  |= (1UL << 0);  // 激活滤波器0
    CAN1->FMR   &= ~(1UL << 0); // 退出滤波器初始化模式
}

/**
  * @brief  配置 CAN 滤波器：
  *         Bank2 → Mask-16bit → 0x200~0x20F 与 0x300~0x30F → FIFO1
  * @note   仅标准数据帧（IDE=0，RTR=0）
  */
static void CAN_FilterConfig_Bank2_RangeStd(void)
{
    /***** 进入过滤器初始化 *****/
    CAN1->FMR  |= CAN_FMR_FINIT;

    /* Bank2 → Mask 模式 + 16-bit */
    CAN1->FM1R &= ~(1U << 2);   // 0 = Mask Mode
    CAN1->FS1R &= ~(1U << 2);   // 0 = 16-bit Scale

    /* -------  ID+Mask 组 1  (0x200~0x20F)  ------- */
    uint16_t ID1   = 0x4000;    // 0x200 << 5
    uint16_t MASK1 = 0xFE0C;    // 如上计算
    CAN1->sFilterRegister[2].FR1  = ((uint32_t)MASK1 << 16) | ID1;

    /* -------  ID+Mask 组 2  (0x300~0x30F)  ------- */
    uint16_t ID2   = 0x6000;    // 0x300 << 5
    uint16_t MASK2 = 0xFE0C;    // 相同掩码
    CAN1->sFilterRegister[2].FR2  = ((uint32_t)MASK2 << 16) | ID2;

    /* 匹配结果送入 FIFO1 */
    CAN1->FFA1R |=  (1U << 2);  // 1 = FIFO1

    /* 激活过滤器并退出初始化 */
    CAN1->FA1R  |=  (1U << 2);  // Enable Bank2
    CAN1->FMR   &= ~CAN_FMR_FINIT;
}

/**
  * @brief  直接寄存器方式配置 Filter Bank1：
  *         List-16bit，接收标准帧 0x058 / 0x158 / 0x258 / 0x358 → FIFO1
  * @note   仅适用于 STM32F1 系列单 CAN（CAN1）控制器
  */
static void CAN_FilterConfig_Bank1_ListStd(void)
{
    /* ---------- 1. 进入过滤器初始化模式 ---------- */
    CAN1->FMR |= CAN_FMR_FINIT;                // FINIT = 1 ? “Filter Init” 模式

    /* ---------- 2. 选择 Bank1 的工作模式 ---------- */
    CAN1->FM1R |=  (1UL << 1);                 // FM1R bit1 = 1 → List  模式
    CAN1->FS1R &= ~(1UL << 1);                 // FS1R bit1 = 0 → 16-bit ×4
    CAN1->FFA1R |=  (1UL << 1);                // FFA1R bit1 = 1 → 匹配后送 FIFO1

    /* ---------- 3. 写入四个离散标准帧的 16-bit 编码 ---------- */
    /* 编码：ID << 5（放 [15:5]），IDE=0，RTR=0 */
    uint16_t id0 = 0x0B00;                     // 0x058 << 5
    uint16_t id1 = 0x2B00;                     // 0x158 << 5
    uint16_t id2 = 0x4B00;                     // 0x258 << 5
    uint16_t id3 = 0x6B00;                     // 0x358 << 5

    /* FR1 高 16 位 = ID0，低 16 位 = ID1 */
    CAN1->sFilterRegister[1].FR1 =
        ( (uint32_t)id0 << 16 ) | id1;

    /* FR2 高 16 位 = ID2，低 16 位 = ID3 */
    CAN1->sFilterRegister[1].FR2 =
        ( (uint32_t)id2 << 16 ) | id3;

    /* ---------- 4. 激活 Bank1 并退出初始化 ---------- */
    CAN1->FA1R |=  (1UL << 1);                 // FA1R bit1 = 1 → 激活
    CAN1->FMR  &= ~CAN_FMR_FINIT;              // FINIT = 0 ? 过滤器正式生效
}

/**
  * @brief  使用直接寄存器操作的CAN初始化配置
  * @retval note
  */
void CAN_Config(void)
{
    /* ringbuffer */
    lwrb_init((lwrb_t*)&g_CanRxRBHandler, (uint8_t*)g_CanRxRBDataBuffer, sizeof(g_CanRxRBDataBuffer) + 1); // RX ringbuffer初始化
    lwrb_init((lwrb_t*)&g_CanTxRBHandler, (uint8_t*)g_CanTxRBDataBuffer, sizeof(g_CanTxRBDataBuffer) + 1); // TX Ringbuffer初始化
    
    /* 1. 使能 CAN1 和 GPIOA 时钟 */
    RCC->APB1ENR |= (1UL << 25);  // 使能 CAN1 时钟
    RCC->APB2ENR |= (1UL << 2);   // 使能 GPIOA 时钟

    /* 2. 配置 PA11 (CAN_RX) 为上拉输入、PA12 (CAN_TX) 为复用推挽输出 */
    // PA11: CRH[15:12], MODE=00, CNF=10 (上拉输入)
    GPIOA->CRH &= ~(0xF << 12);
    GPIOA->CRH |=  (0x8 << 12);
    GPIOA->ODR |=  (1UL << 11); // 上拉

    // PA12: CRH[19:16], MODE=11(50MHz), CNF=10(复用推挽)
    GPIOA->CRH &= ~(0xF << 16);
    GPIOA->CRH |=  (0xB << 16);

    /* 3. 退出 SLEEP 模式（若处于睡眠状态） */
    if (CAN1->MSR & (1UL << 1)) { // 检查 MSR.SLAK 是否为 1
        CAN1->MCR &= ~(1UL << 1);  // 清除 SLEEP 位
        while (CAN1->MSR & (1UL << 1));  // 等待 MSR.SLAK 变 0
    }

    /* 4. 进入初始化模式 */
    CAN1->MCR |= (1UL << 0);          // 请求进入 INIT 模式 (MCR.INRQ=1)
    while (!(CAN1->MSR & (1UL << 0))); // 等待 MSR.INAK 变 1

    /* 5. 关闭 TTCM/ABOM/AWUM/RFLM/TXFP(均为 Disable) */
    CAN1->MCR &= ~(1UL << 7);  // 关闭 TTCM（时间触发模式）
    CAN1->MCR &= ~(1UL << 6);  // 关闭 ABOM（自动离线管理模式）
    CAN1->MCR &= ~(1UL << 5);  // 关闭 AWUM（软件自动唤醒）
    CAN1->MCR &= ~(1UL << 3);  // 关闭 RFLM（FIFO 溢出时覆盖旧报文）
    CAN1->MCR &= ~(1UL << 2);  // 关闭 TXFP（发送 FIFO 优先级）

    /* 6. 关闭自动重传 (NART=1) */
    CAN1->MCR |= (1UL << 4);

    /*
    7. 设置 BTR=0x002D0003, 保持与 HAL 相同:
       - SJW=0  => 1Tq
       - TS2=0x02 => 2 => 3Tq
       - TS1=0x0D => 13 => 14Tq
       - BRP=0x03 => 3 => 分频=4
       - 最终，波特率500K
    */
    CAN1->BTR = (0x00 << 24) |  // SILM(31) | LBKM(30) = 0
                (0x00 << 22) |  // SJW(23:22) = 0 (SJW = 1Tq)
                (0x02 << 20) |  // TS2(22:20) = 2 (TS2 = 3Tq)
                (0x0D << 16) |  // TS1(19:16) = 13 (TS1 = 14Tq)
                (0x0003);       // BRP(9:0) = 3 (Prescaler = 4)

    /* 8. 退出初始化模式 */
    CAN1->MCR &= ~(1UL << 0);  // 清除 INRQ (进入正常模式)
    while (CAN1->MSR & (1UL << 0)); // 等待 MSR.INAK 变 0

    /* 新增：清除总线关闭标志（加了显式清除BOFF位后，这个函数恢复Bus-off错误状态 */
    CAN1->ESR &= ~CAN_ESR_BOFF;  // 显式清除BOFF位

    /* 9. 配置滤波器，FIFO1 */
    //CAN_FilterConfig_AllMessages(); // 接收所有CANID到FIFO1
    CAN_FilterConfig_Bank2_RangeStd(); // 仅接收CANID:0x200~0x20F与0x300~0x30F到FIFO1
    CAN_FilterConfig_Bank1_ListStd();  // 仅接收4个CANID(0x0058、0x0158、0x0258、0x0358)
    
    /* 10.CAN发送完成中断 */
    // 开启CAN发送完成的全局中断，并设置中断优先级
    NVIC_SetPriority(USB_HP_CAN1_TX_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),3, 0));
    NVIC_EnableIRQ(USB_HP_CAN1_TX_IRQn);
    CAN1->IER |= CAN_IER_TMEIE; // 使能发送邮箱空中断
    
    // 开启CAN消息挂号中断（接收中断）
    NVIC_SetPriority(CAN1_RX1_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0, 0));
    NVIC_EnableIRQ(CAN1_RX1_IRQn);
    CAN1->IER |= CAN_IER_FMPIE1; // 使能接收FIFO1中断（消息挂号中断）
    CAN1->IER |= CAN_IER_FOVIE1; // 使能接收FIFO1溢出中断
    
    /* 11.获取一次发送邮箱的数量 */
    txmail_free = ((CAN1->TSR & CAN_TSR_TME0) ? 1 : 0) +
             ((CAN1->TSR & CAN_TSR_TME1) ? 1 : 0) +
             ((CAN1->TSR & CAN_TSR_TME2) ? 1 : 0);
}

/**
  * @brief  获取空闲邮箱的索引(寄存器方式)
  * @retval 空闲邮箱的索引(0, 1, 2)，若无空闲邮箱则返回 0xFF
  */
__STATIC_INLINE uint8_t CAN_Get_Free_Mailbox(void)
{
    uint8_t mailbox;
    for (mailbox = 0; mailbox < 3; mailbox++) {
        // 检查邮箱对应的 TIR 寄存器的 TXRQ 位是否为 0
        // TXRQ 位定义为 (1UL << 0) ，若为 0 表示该邮箱没有处于发送请求中
        if ((CAN1->sTxMailBox[mailbox].TIR & (1UL << 0)) == 0) {
            return mailbox;
        }
    }
    return 0xFF; // 没有空闲邮箱
}

/**
  * @brief  直接操作寄存器实现CAN消息发送(标准数据帧)，代码会阻塞，直到CAN消息被成功发送出去
  * @param  stdId: 标准ID(11位)
  * @param  data: 数据指针
  * @param  DLC: 数据长度(0~8)
  * @retval 0=发送成功; 1=无空闲邮箱; 2=超时/失败
  */
uint8_t CAN_SendMessage_Blocking(uint32_t stdId, uint8_t *data, uint8_t DLC)
{
    uint8_t mailbox;
    uint32_t timeout = 100; // 最大等待100ms
    if (DLC > 8) {
        Error_Handler(); // 根据具体需求，可选择截断或进入错误处理
    }
    mailbox = CAN_Get_Free_Mailbox(); // 寻找空闲邮箱
    if(mailbox == 0xFF)
        return 1; // 无空闲邮箱
    /* 清空该邮箱 */
    CAN1->sTxMailBox[mailbox].TIR  = 0; // 数据帧、标准帧标识符
    CAN1->sTxMailBox[mailbox].TDTR = 0;
    CAN1->sTxMailBox[mailbox].TDLR = 0;
    CAN1->sTxMailBox[mailbox].TDHR = 0;
    /* 标准ID写入TIR的[31:21]、标准帧, RTR=0, IDE=0 */
    CAN1->sTxMailBox[mailbox].TIR |= (stdId << 21); // 设置标准帧ID
    /* 配置DLC */
    CAN1->sTxMailBox[mailbox].TDTR = (DLC & 0x0F);  // 设置CAN报文的长度。使用&运算的目的是保证只有变量DLC的低四位写入TDTR寄存器，不会干涉到其他位。
    /* 填充数据 */
    if(DLC <= 4) {
        for(uint8_t i = 0; i < DLC; i++) {
          CAN1->sTxMailBox[mailbox].TDLR |= ((uint32_t)data[i]) << (8 * i);
        }
    } else {
        for(uint8_t i = 0; i < 4; i++) {
          CAN1->sTxMailBox[mailbox].TDLR |= ((uint32_t)data[i]) << (8 * i);
        }
        for(uint8_t i = 4; i < DLC; i++) {
          CAN1->sTxMailBox[mailbox].TDHR |= ((uint32_t)data[i]) << (8 * (i-4));
        }
    }
    CAN1->sTxMailBox[mailbox].TIR |= CAN_TI0R_TXRQ; // 发起发送请求
    /* 轮询等待TXRQ清零或超时 */
    while((CAN1->sTxMailBox[mailbox].TIR & CAN_TI0R_TXRQ) && (timeout > 0)) {
        LL_mDelay(1);
        timeout--;
    }
    /* 阻塞结束，发送超时失败 */
    if ((CAN1->sTxMailBox[mailbox].TIR & CAN_TI0R_TXRQ) && (timeout == 0)) {
        canSendError++;
        return 2;
    }
    
    return 0; // 发送成功
}

/**
  * @brief  直接操作寄存器实现CAN消息发送(标准数据帧)，代码不会阻塞
  * @param  stdId: 标准ID(11位)
  * @param  data: 数据指针
  * @param  DLC: 数据长度(0~8)
  * @retval 0=发送成功; 1=无空闲邮箱,没有发送数据
  */
uint8_t CAN_SendMessage_NonBlocking(uint32_t stdId, uint8_t *data, uint8_t DLC)
{
    uint8_t mailbox;
    if (DLC > 8) {
        Error_Handler(); // 根据具体需求，可选择截断或进入错误处理
    }
    
    if (txmail_free > 0) {
        mailbox = CAN_Get_Free_Mailbox(); // 既然有空闲邮箱，看看到底哪个空闲
        /* 清空该邮箱并配置ID、DLC和数据 */
        CAN1->sTxMailBox[mailbox].TIR  = 0; // 数据帧、标准帧标识符
        CAN1->sTxMailBox[mailbox].TDTR = (DLC & 0x0F);
        CAN1->sTxMailBox[mailbox].TDLR = 0;
        CAN1->sTxMailBox[mailbox].TDHR = 0;
        CAN1->sTxMailBox[mailbox].TIR |= (stdId << 21); // 设置标准帧ID

        /* 填充数据 */
        for(uint8_t i = 0; i < DLC && i < 8; i++) {
            if(i < 4)
                CAN1->sTxMailBox[mailbox].TDLR |= ((uint32_t)data[i]) << (8 * i);
            else
                CAN1->sTxMailBox[mailbox].TDHR |= ((uint32_t)data[i]) << (8 * (i-4));
        }

        /* 发起发送请求并直接返回 */
        CAN1->sTxMailBox[mailbox].TIR |= CAN_TI0R_TXRQ;
        txmail_free--;
        return 0; // 已发起发送请求，不等待完成
    } else {
        return 1; // 发送失败，因为发送邮箱满了
    }
}

/**
 * @brief  检测并解析CAN1控制器错误状态
 * @param[out] can_esr  用于存储错误详细信息的CAN_ESR_t结构体指针
 * @retval uint8_t       错误状态码：
 *                       - 0: 无错误
 *                       - 1: 协议错误（LEC字段）
 *                       - 2: 错误被动状态（EPVF = 1）
 *                       - 3: 总线关闭状态（BOFF = 1）
 *                       - 4: 警告状态（EWGF = 1）
 * @note   本函数会在读取后自动清除LEC错误码字段
 */
uint8_t CAN_Check_Error(void) {
    /* 读取CAN错误状态寄存器（ESR） */
    uint32_t esr = CAN1->ESR;
    uint8_t result = 0;

    /* 解析所有错误字段 */
    gCanESR.lec   = (esr & CAN_ESR_LEC)   >> CAN_ESR_LEC_Pos;   // 最后错误代码（Last Error Code）
    gCanESR.tec   = (esr & CAN_ESR_TEC)   >> CAN_ESR_TEC_Pos;   // 发送错误计数器（Transmit Error Counter）
    gCanESR.rec   = (esr & CAN_ESR_REC)   >> CAN_ESR_REC_Pos;   // 接收错误计数器（Receive Error Counter）
    gCanESR.epvf  = (esr & CAN_ESR_EPVF)  >> CAN_ESR_EPVF_Pos;  // 错误被动标志（Error Passive Flag）
    gCanESR.ewgf  = (esr & CAN_ESR_EWGF)  >> CAN_ESR_EWGF_Pos;  // 错误警告标志（Error Warning Flag）
    gCanESR.boff  = (esr & CAN_ESR_BOFF)  >> CAN_ESR_BOFF_Pos;  // 总线关闭标志（Bus-Off Flag）

    /* 清除LEC错误码字段（向对应位写0清除） */
    CAN1->ESR &= ~CAN_ESR_LEC;

    /* 按错误严重程度分级返回（BOFF > EPVF > LEC > EWGF）*/
    if (gCanESR.boff == 0x01) {
        return 3;
    } else if (gCanESR.epvf == 0x01) {
        return 2; // 错误被动状态（TEC/REC > 127）
    } else if (gCanESR.lec == 0x01) {
        return 1; // 协议错误（位填充/格式/ACK等错误）
    } else if (gCanESR.ewgf == 0x01) {
        return 4; // 警告状态（TEC/REC > 96）
    } else {
        return 0; // 无错误
    }
}

/**
 * @brief CAN总线Bus-Off恢复函数
 *
 * 该函数用于在CAN总线进入Bus-Off状态后，恢复CAN控制器的正常工作状态。
 * 它首先通过设置CAN1->MCR寄存器中的CAN_MCR_SLEEP位，使CAN进入睡眠模式，停止数据发送和接收，
 * 随后调用CAN_Config函数对CAN进行重新初始化，以便使其重新恢复正常运行。
 *
 * @note 
 */
void CAN_BusOff_Recover(void)
{
    CAN1->MCR |= CAN_MCR_SLEEP;  // 进入睡眠模式（停止收发）
    CAN_Config();                // 重新初始化一次CAN
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
    if (CAN_SendMessage_NonBlocking(msg.CanId, msg.data, msg.Len) == 0) {
        lwrb_skip((lwrb_t*)&g_CanTxRBHandler, sizeof(CANTXMsg_t)); // 成功：正式把这帧从 RingBuffer 删掉
        return true;
    }
    canSendError++; // 理论上很少到这里
    return false;
}

/**
 * @brief     在 CAN TX 完成中断中调用的快速续发函数
 *
 * 连续调用 @ref CAN_TxOnceFromRB 直到无法再写入邮箱（邮箱满或
 * RingBuffer 没数据）。因处于 ISR，不执行任何关中断操作。
 *
 * @note 仅应在 @p USB_HP_CAN1_TX_IRQHandler 等发送完成 IRQ 回调里调用。
 */
void CAN_Get_CANMsg_From_RB_To_TXMailBox_IT(void)
{
    while (CAN_TxOnceFromRB()) { }
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
    /* ----------- 早退出：无数据 或 无空闲邮箱 ----------- */
    if (txmail_free == 0) {
        return; // 邮箱全满
    }
    if (lwrb_get_full((lwrb_t*)&g_CanTxRBHandler) < sizeof(CANTXMsg_t)) {
        return; // 缓冲区没数据
    }
    /* 关 TX-Mailbox-Empty 中断，防止并发 */
    CAN1->IER &= ~CAN_IER_TMEIE;
    /* 把仍然空着的邮箱一次发完 */
    while (CAN_TxOnceFromRB()) { }
    /* 恢复中断 */
    CAN1->IER |= CAN_IER_TMEIE;
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
 * @brief 从FIFO1中获取CAN报文数据
 *
 * 该函数直接通过寄存器方式读取CAN1的FIFO1内的报文数据，解析报文头和数据，并释放FIFO1中的报文。
 * 解析后的报文头信息存储在rxHeader指向的结构体中，数据部分存储在rxData所指的数组中。

 * @param[in,out] rxHeader 指向CAN_RxHeaderTypeDef结构体的指针，用于存储解析后的报文头信息（包括ID、IDE、RTR、DLC、滤波器匹配索引和时间戳）。
 * @param[out] rxData 指向用于存储报文数据的缓冲区指针，最多可存储8字节数据。
 *
 * @note 该函数假设接收到的报文位于FIFO1。
 */
__STATIC_INLINE void CAN_Get_Message_From_FIFO1(CAN_RxHeaderTypeDef* rxHeader, uint8_t* rxData)
{
    // 1. 从FIFO1中读取接收报文相关寄存器内容
    uint32_t rx_rir   = CAN1->sFIFOMailBox[1].RIR;   // 接收标识符寄存器
    uint32_t rx_rdtr  = CAN1->sFIFOMailBox[1].RDTR;  // 接收数据长度和时间戳寄存器
    uint32_t rx_rdlr  = CAN1->sFIFOMailBox[1].RDLR;  // 接收数据低32位寄存器
    uint32_t rx_rdhr  = CAN1->sFIFOMailBox[1].RDHR;  // 接收数据高32位寄存器

    // 2. 解析报文头
    // RDTR寄存器：
    //     - [3:0]  表示数据字节数（DLC）
    //     - [7:4]  保留（根据实际情况不同，这里我们只关心DLC和时间戳）
    //     - [15:8] 表示滤波器匹配索引(FilterMatchIndex)
    //     - [31:16] 表示接收时间戳
    rxHeader->DLC               = rx_rdtr & 0x0F;
    rxHeader->FilterMatchIndex  = (rx_rdtr >> 8) & 0xFF;
    rxHeader->Timestamp         = (rx_rdtr >> 16) & 0xFFFF;

    // 3. 判断帧类型并提取ID和远程传输请求（RTR）标志
    // IDE位（一般在RIR的第2位）：0表示标准帧，1表示扩展帧
    if ((rx_rir & (1UL << 2)) == 0) {  // 标准帧
        rxHeader->IDE   = 0;
        // 标准ID位于RIR的[31:21]，共11位
        rxHeader->StdId = (rx_rir >> 21) & 0x7FF;
        rxHeader->ExtId = 0;
    } else {  // 扩展帧
        rxHeader->IDE   = 1;
        // 扩展ID位于RIR的[31:3]，共29位
        rxHeader->ExtId = (rx_rir >> 3) & 0x1FFFFFFF;
        rxHeader->StdId = 0;
    }
    // RTR位（RIR的第1位）：1表示远程帧，0表示数据帧
    rxHeader->RTR = (rx_rir & (1UL << 1)) ? 1 : 0;

    // 4. 读取数据部分，根据DLC从RDLR和RDHR中提取数据字节（最多8字节）
    uint8_t dlc = rxHeader->DLC;
    if (dlc > 8)
        dlc = 8;
    for (uint8_t i = 0; i < dlc; i++) {
        if (i < 4) {
            rxData[i] = (uint8_t)((rx_rdlr >> (8 * i)) & 0xFF);
        } else {
            rxData[i] = (uint8_t)((rx_rdhr >> (8 * (i - 4))) & 0xFF);
        }
    }

    // 5. 释放FIFO1中的报文：写1到CAN1->RF1R中的RFOM1位，释放该报文，使FIFO1指针前移
    CAN1->RF1R |= CAN_RF1R_RFOM1;
}

/**
  * @brief  CAN接收FIFO1溢出中断处理
  * @note   寄存器方式实现
  * @retval None
  */
__STATIC_INLINE void CAN_FIFO1_Overflow_Handler(void)
{
    g_RxOverflowError++;          // 溢出计数自增
    CAN1->RF1R |= CAN_RF1R_FOVR1; // 清除FIFO1溢出标志（写1清除）
}

/**
  * @brief  CAN接收FIFO1挂号中断处理
  * @note   寄存器方式实现
  * @retval None
  */
__STATIC_INLINE void CAN_FIFO1_Message_Pending_Handler(void)
{
    CANMsg_t canMsg = {0,};
    while (CAN1->RF1R & CAN_RF1R_FMP1) { // 当FIFO1中还有待处理的报文时，循环读取
        g_RxCount++; // 更新全局接收报文计数器
        CAN_Get_Message_From_FIFO1(&canMsg.RxHeader, (uint8_t*)canMsg.RxData); // 从FIFO1中获取CAN报文的详细内容
        if(lwrb_get_free((lwrb_t*)&g_CanRxRBHandler) < sizeof(CANMsg_t)) { // 判断ringbuffer是否被挤满
            g_RXRingbufferOverflow++;    // 累加ringbuffer溢出全局计数器
            lwrb_reset((lwrb_t*)&g_CanRxRBHandler); // 清空ringbuffer
        }
        lwrb_write((lwrb_t*)&g_CanRxRBHandler, &canMsg, sizeof(CANMsg_t)); // 将CAN报文放入ringbuffer
    }
}

/**
  * @brief  外设CAN的发送完成全局中断函数
  * @note   寄存器方式实现
  * @retval None
  */
void USB_HP_CAN1_TX_IRQHandler(void)
{
    // 检查发送邮箱空中断标志（一定要清除标志位）
    if(CAN1->TSR & CAN_TSR_TME0) { // 邮箱0空
        // 你的处理代码...
        CAN1->TSR |= CAN_TSR_TME0; // 清除标志（通过写1清除）
    }
    if(CAN1->TSR & CAN_TSR_TME1) { // 邮箱1空
        // 你的处理代码...
        CAN1->TSR |= CAN_TSR_TME1;
    }
    if(CAN1->TSR & CAN_TSR_TME2) { // 邮箱2空
        // 你的处理代码...
        CAN1->TSR |= CAN_TSR_TME2;
    }
    // 保持临界区（代码更健壮）
    __disable_irq();
    // 通过寄存器方式，更新发送邮箱的空闲数量
    txmail_free = ((CAN1->TSR & CAN_TSR_TME0) ? 1 : 0) +
                 ((CAN1->TSR & CAN_TSR_TME1) ? 1 : 0) +
                 ((CAN1->TSR & CAN_TSR_TME2) ? 1 : 0);
    // 将TX ringbuffer的CAN报文搬到CAN TX邮箱
    CAN_Get_CANMsg_From_RB_To_TXMailBox_IT();
    __enable_irq();
}

/**
  * @brief  外设CAN的RX_FIFO1全局中断函数
  * @note   寄存器方式实现
  * @retval None
  */
void CAN1_RX1_IRQHandler(void)
{
    /* 首先检查FIFO1是否发生溢出 */
    if (CAN1->RF1R & CAN_RF1R_FOVR1) { // FIFO1接收溢出中断
        CAN_FIFO1_Overflow_Handler(); // 接收FIFO1溢出中断处理
    } else if (CAN1->RF1R & CAN_RF1R_FMP1) { // FIFO1挂号中断
        CAN_FIFO1_Message_Pending_Handler(); // 接收FIFO1挂号中断处理
    } else {
        // 其他错误
    }
}
