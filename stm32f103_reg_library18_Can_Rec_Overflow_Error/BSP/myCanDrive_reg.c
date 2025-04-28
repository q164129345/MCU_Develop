#include "myCanDrive_reg.h"

volatile uint8_t txmail_free = 0;
volatile uint32_t canSendError = 0;
CAN_ESR_t gCanESR = {0,};
volatile uint32_t g_RxCount = 0;    // ��¼���ձ�������
volatile uint32_t g_RxOverflowError = 0; // ��¼�����������
CANMsg_t g_CanMsg = {0,}; // ȫ�ֱ��������ڹ۲�
/**
  * @brief  ʹ��ֱ�ӼĴ���������CAN��ʼ������
  * @retval note
  */
void CAN_Config(void)
{
    /* 1. ʹ�� CAN1 �� GPIOA ʱ�� */
    RCC->APB1ENR |= (1UL << 25);  // ʹ�� CAN1 ʱ��
    RCC->APB2ENR |= (1UL << 2);   // ʹ�� GPIOA ʱ��

    /* 2. ���� PA11 (CAN_RX) Ϊ�������롢PA12 (CAN_TX) Ϊ����������� */
    // PA11: CRH[15:12], MODE=00, CNF=10 (��������)
    GPIOA->CRH &= ~(0xF << 12);
    GPIOA->CRH |=  (0x8 << 12);
    GPIOA->ODR |=  (1UL << 11); // ����

    // PA12: CRH[19:16], MODE=11(50MHz), CNF=10(��������)
    GPIOA->CRH &= ~(0xF << 16);
    GPIOA->CRH |=  (0xB << 16);

    /* 3. �˳� SLEEP ģʽ��������˯��״̬�� */
    if (CAN1->MSR & (1UL << 1)) { // ��� MSR.SLAK �Ƿ�Ϊ 1
        CAN1->MCR &= ~(1UL << 1);  // ��� SLEEP λ
        while (CAN1->MSR & (1UL << 1));  // �ȴ� MSR.SLAK �� 0
    }

    /* 4. �����ʼ��ģʽ */
    CAN1->MCR |= (1UL << 0);          // ������� INIT ģʽ (MCR.INRQ=1)
    while (!(CAN1->MSR & (1UL << 0))); // �ȴ� MSR.INAK �� 1

    /* 5. �ر� TTCM/ABOM/AWUM/RFLM/TXFP(��Ϊ Disable) */
    CAN1->MCR &= ~(1UL << 7);  // �ر� TTCM��ʱ�䴥��ģʽ��
    CAN1->MCR &= ~(1UL << 6);  // �ر� ABOM���Զ����߹���ģʽ��
    CAN1->MCR &= ~(1UL << 5);  // �ر� AWUM������Զ����ѣ�
    CAN1->MCR &= ~(1UL << 3);  // �ر� RFLM��FIFO ���ʱ���Ǿɱ��ģ�
    CAN1->MCR &= ~(1UL << 2);  // �ر� TXFP������ FIFO ���ȼ���

    /* 6. �ر��Զ��ش� (NART=1) */
    CAN1->MCR |= (1UL << 4);

    /*
    7. ���� BTR=0x002D0003, ������ HAL ��ͬ:
       - SJW=0  => 1Tq
       - TS2=0x02 => 2 => 3Tq
       - TS1=0x0D => 13 => 14Tq
       - BRP=0x03 => 3 => ��Ƶ=4
       - ���գ�������500K
    */
    CAN1->BTR = (0x00 << 24) |  // SILM(31) | LBKM(30) = 0
                (0x00 << 22) |  // SJW(23:22) = 0 (SJW = 1Tq)
                (0x02 << 20) |  // TS2(22:20) = 2 (TS2 = 3Tq)
                (0x0D << 16) |  // TS1(19:16) = 13 (TS1 = 14Tq)
                (0x0003);       // BRP(9:0) = 3 (Prescaler = 4)

    /* 8. �˳���ʼ��ģʽ */
    CAN1->MCR &= ~(1UL << 0);  // ��� INRQ (��������ģʽ)
    while (CAN1->MSR & (1UL << 0)); // �ȴ� MSR.INAK �� 0

    /* ������������߹رձ�־��������ʽ���BOFFλ����������ָ�Bus-off����״̬ */
    CAN1->ESR &= ~CAN_ESR_BOFF;  // ��ʽ���BOFFλ

    /* 9. �����˲���0��FIFO1 */
    CAN1->FMR |= (1UL << 0);   // �����˲�����ʼ��ģʽ
    CAN1->FM1R  &= ~(1UL << 0); // �˲���0ʹ������λģʽ
    CAN1->sFilterRegister[0].FR1 = 0x00000000;
    CAN1->sFilterRegister[0].FR2 = 0x00000000;
    CAN1->FFA1R |= (1UL << 0);  // ���䵽 FIFO1
    CAN1->FA1R  |= (1UL << 0);  // �����˲���0
    CAN1->FMR   &= ~(1UL << 0); // �˳��˲�����ʼ��ģʽ
    
    /* 10.CAN��������ж� */
    // ����CAN������ɵ�ȫ���жϣ��������ж����ȼ�
    NVIC_SetPriority(USB_HP_CAN1_TX_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),3, 0));
    NVIC_EnableIRQ(USB_HP_CAN1_TX_IRQn);
    CAN1->IER |= CAN_IER_TMEIE; // ʹ�ܷ���������ж�
    
    // ����CAN��Ϣ�Һ��жϣ������жϣ�
    NVIC_SetPriority(CAN1_RX1_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0, 0));
    NVIC_EnableIRQ(CAN1_RX1_IRQn);
    //CAN1->IER |= CAN_IER_FMPIE1; // ʹ�ܽ���FIFO1�жϣ���Ϣ�Һ��жϣ�
    CAN1->IER |= CAN_IER_FOVIE1; // ʹ�ܽ���FIFO1����ж�
    
    /* 11.��ȡһ�η������������ */
    txmail_free = ((CAN1->TSR & CAN_TSR_TME0) ? 1 : 0) +
             ((CAN1->TSR & CAN_TSR_TME1) ? 1 : 0) +
             ((CAN1->TSR & CAN_TSR_TME2) ? 1 : 0);
}

/**
  * @brief  ��ȡ�������������(�Ĵ�����ʽ)
  * @retval �������������(0, 1, 2)�����޿��������򷵻� 0xFF
  */
__STATIC_INLINE uint8_t CAN_Get_Free_Mailbox(void)
{
    uint8_t mailbox;
    for (mailbox = 0; mailbox < 3; mailbox++) {
        // ��������Ӧ�� TIR �Ĵ����� TXRQ λ�Ƿ�Ϊ 0
        // TXRQ λ����Ϊ (1UL << 0) ����Ϊ 0 ��ʾ������û�д��ڷ���������
        if ((CAN1->sTxMailBox[mailbox].TIR & (1UL << 0)) == 0) {
            return mailbox;
        }
    }
    return 0xFF; // û�п�������
}

/**
  * @brief  ֱ�Ӳ����Ĵ���ʵ��CAN��Ϣ����(��׼����֡)�������������ֱ��CAN��Ϣ���ɹ����ͳ�ȥ
  * @param  stdId: ��׼ID(11λ)
  * @param  data: ����ָ��
  * @param  DLC: ���ݳ���(0~8)
  * @retval 0=���ͳɹ�; 1=�޿�������; 2=��ʱ/ʧ��
  */
uint8_t CAN_SendMessage_Blocking(uint32_t stdId, uint8_t *data, uint8_t DLC)
{
    uint8_t mailbox;
    uint32_t timeout = 100; // ���ȴ�100ms
    if (DLC > 8) {
        Error_Handler(); // ���ݾ������󣬿�ѡ��ضϻ���������
    }
    mailbox = CAN_Get_Free_Mailbox(); // Ѱ�ҿ�������
    if(mailbox == 0xFF)
        return 1; // �޿�������
    /* ��ո����� */
    CAN1->sTxMailBox[mailbox].TIR  = 0; // ����֡����׼֡��ʶ��
    CAN1->sTxMailBox[mailbox].TDTR = 0;
    CAN1->sTxMailBox[mailbox].TDLR = 0;
    CAN1->sTxMailBox[mailbox].TDHR = 0;
    /* ��׼IDд��TIR��[31:21]����׼֡, RTR=0, IDE=0 */
    CAN1->sTxMailBox[mailbox].TIR |= (stdId << 21); // ���ñ�׼֡ID
    /* ����DLC */
    CAN1->sTxMailBox[mailbox].TDTR = (DLC & 0x0F);  // ����CAN���ĵĳ��ȡ�ʹ��&�����Ŀ���Ǳ�ֻ֤�б���DLC�ĵ���λд��TDTR�Ĵ�����������浽����λ��
    /* ������� */
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
    CAN1->sTxMailBox[mailbox].TIR |= CAN_TI0R_TXRQ; // ����������
    /* ��ѯ�ȴ�TXRQ�����ʱ */
    while((CAN1->sTxMailBox[mailbox].TIR & CAN_TI0R_TXRQ) && (timeout > 0)) {
        LL_mDelay(1);
        timeout--;
    }
    /* �������������ͳ�ʱʧ�� */
    if ((CAN1->sTxMailBox[mailbox].TIR & CAN_TI0R_TXRQ) && (timeout == 0)) {
        canSendError++;
        return 2;
    }
    
    return 0; // ���ͳɹ�
}

/**
  * @brief  ֱ�Ӳ����Ĵ���ʵ��CAN��Ϣ����(��׼����֡)�����벻������
  * @param  stdId: ��׼ID(11λ)
  * @param  data: ����ָ��
  * @param  DLC: ���ݳ���(0~8)
  * @retval 0=���ͳɹ�; 1=�޿�������,û�з�������
  */
uint8_t CAN_SendMessage_NonBlocking(uint32_t stdId, uint8_t *data, uint8_t DLC)
{
    uint8_t mailbox;
    if (DLC > 8) {
        Error_Handler(); // ���ݾ������󣬿�ѡ��ضϻ���������
    }
    
    if (txmail_free > 0) {
        mailbox = CAN_Get_Free_Mailbox(); // ��Ȼ�п������䣬���������ĸ�����
        /* ��ո����䲢����ID��DLC������ */
        CAN1->sTxMailBox[mailbox].TIR  = 0; // ����֡����׼֡��ʶ��
        CAN1->sTxMailBox[mailbox].TDTR = (DLC & 0x0F);
        CAN1->sTxMailBox[mailbox].TDLR = 0;
        CAN1->sTxMailBox[mailbox].TDHR = 0;
        CAN1->sTxMailBox[mailbox].TIR |= (stdId << 21); // ���ñ�׼֡ID

        /* ������� */
        for(uint8_t i = 0; i < DLC && i < 8; i++) {
            if(i < 4)
                CAN1->sTxMailBox[mailbox].TDLR |= ((uint32_t)data[i]) << (8 * i);
            else
                CAN1->sTxMailBox[mailbox].TDHR |= ((uint32_t)data[i]) << (8 * (i-4));
        }

        /* ����������ֱ�ӷ��� */
        CAN1->sTxMailBox[mailbox].TIR |= CAN_TI0R_TXRQ;
        txmail_free--;
        return 0; // �ѷ��������󣬲��ȴ����
    } else {
        return 1; // ����ʧ�ܣ���Ϊ������������
    }
}

/**
 * @brief  ��Ⲣ����CAN1����������״̬
 * @param[out] can_esr  ���ڴ洢������ϸ��Ϣ��CAN_ESR_t�ṹ��ָ��
 * @retval uint8_t       ����״̬�룺
 *                       - 0: �޴���
 *                       - 1: Э�����LEC�ֶΣ�
 *                       - 2: ���󱻶�״̬��EPVF = 1��
 *                       - 3: ���߹ر�״̬��BOFF = 1��
 *                       - 4: ����״̬��EWGF = 1��
 * @note   ���������ڶ�ȡ���Զ����LEC�������ֶ�
 */
uint8_t CAN_Check_Error(void) {
    /* ��ȡCAN����״̬�Ĵ�����ESR�� */
    uint32_t esr = CAN1->ESR;
    uint8_t result = 0;

    /* �������д����ֶ� */
    gCanESR.lec   = (esr & CAN_ESR_LEC)   >> CAN_ESR_LEC_Pos;   // ��������루Last Error Code��
    gCanESR.tec   = (esr & CAN_ESR_TEC)   >> CAN_ESR_TEC_Pos;   // ���ʹ����������Transmit Error Counter��
    gCanESR.rec   = (esr & CAN_ESR_REC)   >> CAN_ESR_REC_Pos;   // ���մ����������Receive Error Counter��
    gCanESR.epvf  = (esr & CAN_ESR_EPVF)  >> CAN_ESR_EPVF_Pos;  // ���󱻶���־��Error Passive Flag��
    gCanESR.ewgf  = (esr & CAN_ESR_EWGF)  >> CAN_ESR_EWGF_Pos;  // ���󾯸��־��Error Warning Flag��
    gCanESR.boff  = (esr & CAN_ESR_BOFF)  >> CAN_ESR_BOFF_Pos;  // ���߹رձ�־��Bus-Off Flag��

    /* ���LEC�������ֶΣ����Ӧλд0����� */
    CAN1->ESR &= ~CAN_ESR_LEC;

    /* ���������س̶ȷּ����أ�BOFF > EPVF > LEC > EWGF��*/
    if (gCanESR.boff == 0x01) {
        return 3;
    } else if (gCanESR.epvf == 0x01) {
        return 2; // ���󱻶�״̬��TEC/REC > 127��
    } else if (gCanESR.lec == 0x01) {
        return 1; // Э�����λ���/��ʽ/ACK�ȴ���
    } else if (gCanESR.ewgf == 0x01) {
        return 4; // ����״̬��TEC/REC > 96��
    } else {
        return 0; // �޴���
    }
}

/**
 * @brief CAN����Bus-Off�ָ�����
 *
 * �ú���������CAN���߽���Bus-Off״̬�󣬻ָ�CAN����������������״̬��
 * ������ͨ������CAN1->MCR�Ĵ����е�CAN_MCR_SLEEPλ��ʹCAN����˯��ģʽ��ֹͣ���ݷ��ͺͽ��գ�
 * ������CAN_Config������CAN�������³�ʼ�����Ա�ʹ�����»ָ��������С�
 *
 * @note 
 */
void CAN_BusOff_Recover(void)
{
    CAN1->MCR |= CAN_MCR_SLEEP;  // ����˯��ģʽ��ֹͣ�շ���
    CAN_Config();                // ���³�ʼ��һ��CAN
}

/**
 * @brief ��FIFO1�л�ȡCAN��������
 *
 * �ú���ֱ��ͨ���Ĵ�����ʽ��ȡCAN1��FIFO1�ڵı������ݣ���������ͷ�����ݣ����ͷ�FIFO1�еı��ġ�
 * ������ı���ͷ��Ϣ�洢��rxHeaderָ��Ľṹ���У����ݲ��ִ洢��rxData��ָ�������С�

 * @param[in,out] rxHeader ָ��CAN_RxHeaderTypeDef�ṹ���ָ�룬���ڴ洢������ı���ͷ��Ϣ������ID��IDE��RTR��DLC���˲���ƥ��������ʱ�������
 * @param[out] rxData ָ�����ڴ洢�������ݵĻ�����ָ�룬���ɴ洢8�ֽ����ݡ�
 *
 * @note �ú���������յ��ı���λ��FIFO1��
 */
__STATIC_INLINE void CAN_Get_Message_From_FIFO1(CAN_RxHeaderTypeDef* rxHeader, uint8_t* rxData)
{
    // 1. ��FIFO1�ж�ȡ���ձ�����ؼĴ�������
    uint32_t rx_rir   = CAN1->sFIFOMailBox[1].RIR;   // ���ձ�ʶ���Ĵ���
    uint32_t rx_rdtr  = CAN1->sFIFOMailBox[1].RDTR;  // �������ݳ��Ⱥ�ʱ����Ĵ���
    uint32_t rx_rdlr  = CAN1->sFIFOMailBox[1].RDLR;  // �������ݵ�32λ�Ĵ���
    uint32_t rx_rdhr  = CAN1->sFIFOMailBox[1].RDHR;  // �������ݸ�32λ�Ĵ���

    // 2. ��������ͷ
    // RDTR�Ĵ�����
    //     - [3:0]  ��ʾ�����ֽ�����DLC��
    //     - [7:4]  ����������ʵ�������ͬ����������ֻ����DLC��ʱ�����
    //     - [15:8] ��ʾ�˲���ƥ������(FilterMatchIndex)
    //     - [31:16] ��ʾ����ʱ���
    rxHeader->DLC               = rx_rdtr & 0x0F;
    rxHeader->FilterMatchIndex  = (rx_rdtr >> 8) & 0xFF;
    rxHeader->Timestamp         = (rx_rdtr >> 16) & 0xFFFF;

    // 3. �ж�֡���Ͳ���ȡID��Զ�̴�������RTR����־
    // IDEλ��һ����RIR�ĵ�2λ����0��ʾ��׼֡��1��ʾ��չ֡
    if ((rx_rir & (1UL << 2)) == 0) {  // ��׼֡
        rxHeader->IDE   = 0;
        // ��׼IDλ��RIR��[31:21]����11λ
        rxHeader->StdId = (rx_rir >> 21) & 0x7FF;
        rxHeader->ExtId = 0;
    } else {  // ��չ֡
        rxHeader->IDE   = 1;
        // ��չIDλ��RIR��[31:3]����29λ
        rxHeader->ExtId = (rx_rir >> 3) & 0x1FFFFFFF;
        rxHeader->StdId = 0;
    }
    // RTRλ��RIR�ĵ�1λ����1��ʾԶ��֡��0��ʾ����֡
    rxHeader->RTR = (rx_rir & (1UL << 1)) ? 1 : 0;

    // 4. ��ȡ���ݲ��֣�����DLC��RDLR��RDHR����ȡ�����ֽڣ����8�ֽڣ�
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

    // 5. �ͷ�FIFO1�еı��ģ�д1��CAN1->RF1R�е�RFOM1λ���ͷŸñ��ģ�ʹFIFO1ָ��ǰ��
    CAN1->RF1R |= CAN_RF1R_RFOM1;
}

/**
  * @brief  CAN����FIFO1����жϴ���
  * @note   �Ĵ�����ʽʵ��
  * @retval None
  */
__STATIC_INLINE void CAN_FIFO1_Overflow_Handler(void)
{
    g_RxOverflowError++;          // �����������
    CAN1->RF1R |= CAN_RF1R_FOVR1; // ���FIFO1�����־��д1�����
}

/**
  * @brief  CAN����FIFO1�Һ��жϴ���
  * @note   �Ĵ�����ʽʵ��
  * @retval None
  */
__STATIC_INLINE void CAN_FIFO1_Message_Pending_Handler(void)
{
    while (CAN1->RF1R & CAN_RF1R_FMP1) { // ��FIFO1�л��д�����ı���ʱ��ѭ����ȡ
        CAN_Get_Message_From_FIFO1(&g_CanMsg.RxHeader, (uint8_t*)g_CanMsg.RxData); // ��FIFO1�л�ȡCAN���ĵ���ϸ����
        /* ��������ӶԽ��յ����ݵĴ������
         * ��������û��Զ���ĺ����������ݣ�
         * Process_CAN_Message(&g_CanMsg.RxHeader, g_CanMsg.RxData);
         * ���߽����յ������ݷ��뻷�λ�����ringbuffer��
         */
        g_RxCount++; // ����ȫ�ֽ��ձ��ļ�����
    }
}

/**
  * @brief  ����CAN�ķ������ȫ���жϺ���
  * @note   �Ĵ�����ʽʵ��
  * @retval None
  */
void USB_HP_CAN1_TX_IRQHandler(void)
{
    // ��鷢��������жϱ�־��һ��Ҫ�����־λ��
    if(CAN1->TSR & CAN_TSR_TME0) { // ����0��
        // ��Ĵ������...
        CAN1->TSR |= CAN_TSR_TME0; // �����־��ͨ��д1�����
    }
    if(CAN1->TSR & CAN_TSR_TME1) { // ����1��
        // ��Ĵ������...
        CAN1->TSR |= CAN_TSR_TME1;
    }
    if(CAN1->TSR & CAN_TSR_TME2) { // ����2��
        // ��Ĵ������...
        CAN1->TSR |= CAN_TSR_TME2;
    }
    // �����ٽ������������׳��
    __disable_irq();
    // ͨ���Ĵ�����ʽ�����·�������Ŀ�������
    txmail_free = ((CAN1->TSR & CAN_TSR_TME0) ? 1 : 0) +
                 ((CAN1->TSR & CAN_TSR_TME1) ? 1 : 0) +
                 ((CAN1->TSR & CAN_TSR_TME2) ? 1 : 0);
    __enable_irq();

}

/**
  * @brief  ����CAN��RX_FIFO1ȫ���жϺ���
  * @note   �Ĵ�����ʽʵ��
  * @retval None
  */
void CAN1_RX1_IRQHandler(void)
{
    /* ���ȼ��FIFO1�Ƿ������ */
    if (CAN1->RF1R & CAN_RF1R_FOVR1) { // FIFO1��������ж�
        CAN_FIFO1_Overflow_Handler(); // ����FIFO1����жϴ���
    } else if (CAN1->RF1R & CAN_RF1R_FMP1) { // FIFO1�Һ��ж�
        CAN_FIFO1_Message_Pending_Handler(); // ����FIFO1�Һ��жϴ���
    } else {
        // ��������
    }
}


