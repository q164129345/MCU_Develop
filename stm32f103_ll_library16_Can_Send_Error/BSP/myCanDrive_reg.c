#include "myCanDrive_reg.h"

volatile uint8_t txmail_free = 0;
volatile uint32_t canSendError = 0;
CAN_ESR_t gCanESR = {0,};

/**
  * @brief  使用直接寄存器操作的CAN初始化配置
  * @retval note
  */
void CAN_Config(void)
{
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

    /* 9. 配置过滤器 0，FIFO0 */
    CAN1->FMR |= (1UL << 0);   // 进入过滤器初始化模式
    CAN1->sFilterRegister[0].FR1 = 0x00000000;
    CAN1->sFilterRegister[0].FR2 = 0x00000000;
    CAN1->FFA1R &= ~(1UL << 0);  // 分配到 FIFO0
    CAN1->FA1R  |=  (1UL << 0);  // 激活过滤器 0
    CAN1->FMR   &= ~(1UL << 0);  // 退出过滤器初始化模式
    
    /* 10.CAN发送完成中断 */
    // 开启CAN发送完成的全局中断，并设置中断优先级
    NVIC_SetPriority(USB_HP_CAN1_TX_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),3, 0));
    NVIC_EnableIRQ(USB_HP_CAN1_TX_IRQn);
    CAN1->IER |= CAN_IER_TMEIE; // 发送邮箱空中断使能
    
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







