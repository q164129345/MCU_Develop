#include "myCanDrive_reg.h"

volatile uint8_t txmail_free = 0;        // 空闲邮箱的数量
volatile uint32_t canSendError = 0;      // 统计发送失败次数
CAN_ESR_t gCanESR = {0,};                // 解析错误
volatile uint32_t g_RxCount = 0;         // 记录接收报文总数
volatile uint32_t g_RxOverflowError = 0; // 记录接收溢出错误
volatile uint32_t g_RXRingbufferOverflow = 0; // 统计ringbuffer溢出次数

/* ringbuffer */
volatile lwrb_t g_CanRxRBHandler; // 实例化ringbuffer
volatile CANMsg_t g_CanRxRBDataBuffer[50] = {0,}; // ringbuffer缓存（最多可以存50个CAN消息）

/**
  * @brief  使用直接寄存器操作的CAN初始化配置
  * @retval note
  */
void CAN_Config(void)
{
    /* ringbuffer */
    lwrb_init((lwrb_t*)&g_CanRxRBHandler, (uint8_t*)g_CanRxRBDataBuffer, sizeof(g_CanRxRBDataBuffer)); // ringbuffer初始化
    
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

    /* 9. 配置滤波器0，FIFO1 */
    CAN1->FMR |= (1UL << 0);   // 进入滤波器初始化模式
    CAN1->FM1R  &= ~(1UL << 0); // 滤波器0使用屏蔽位模式
    CAN1->sFilterRegister[0].FR1 = 0x00000000;
    CAN1->sFilterRegister[0].FR2 = 0x00000000;
    CAN1->FFA1R |= (1UL << 0);  // 分配到 FIFO1
    CAN1->FA1R  |= (1UL << 0);  // 激活滤波器0
    CAN1->FMR   &= ~(1UL << 0); // 退出滤波器初始化模式
    
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
  * @brief  从ringbuffer中取出所有的CANMsg_t消息并依次发送到CAN总线上
  * @return 0：所有消息发送成功  
  *         1：发送过程中出现发送邮箱不足或发送错误  
  *         2：没有一条完整的CAN消息可供发送
  */
uint8_t CAN_Send_CANMsg_FromRingBuffer(void)
{
    uint8_t ret = 2;  // 默认返回2表示没有消息可发
    while(lwrb_get_full((lwrb_t*)&g_CanRxRBHandler) >= sizeof(CANMsg_t)) {
        ret = 0; // 已有消息可发送
        if(txmail_free == 0) {
            ret = 1;  // 没有可用的发送邮箱
            break;
        }
        CANMsg_t canMsg;
        lwrb_read((lwrb_t*)&g_CanRxRBHandler, &canMsg, sizeof(CANMsg_t)); // 从ringbuffer中读取一条CAN消息
        if (CAN_SendMessage_NonBlocking(canMsg.RxHeader.StdId, canMsg.RxData, canMsg.RxHeader.DLC)) { // 使用非串行方式发送消息
            ret = 1; // 如果发送失败（例如发送邮箱不足或其他错误），记录错误后退出
            break;
        }
    }
    return ret;
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
