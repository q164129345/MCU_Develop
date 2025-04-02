#include "myCanDrive_reg.h"

volatile uint8_t txmail_free = 0;
CAN_ESR_t gCanESR = {0,};

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
    //CAN1->ESR &= ~CAN_ESR_BOFF;  // ��ʽ���BOFFλ

    /* 9. ���ù����� 0��FIFO0 */
    CAN1->FMR |= (1UL << 0);   // �����������ʼ��ģʽ
    CAN1->sFilterRegister[0].FR1 = 0x00000000;
    CAN1->sFilterRegister[0].FR2 = 0x00000000;
    CAN1->FFA1R &= ~(1UL << 0);  // ���䵽 FIFO0
    CAN1->FA1R  |=  (1UL << 0);  // ��������� 0
    CAN1->FMR   &= ~(1UL << 0);  // �˳���������ʼ��ģʽ
    
    /* 10.CAN��������ж� */
    // ����CAN������ɵ�ȫ���жϣ��������ж����ȼ�
    NVIC_SetPriority(USB_HP_CAN1_TX_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),3, 0));
    NVIC_EnableIRQ(USB_HP_CAN1_TX_IRQn);
    CAN1->IER |= CAN_IER_TMEIE; // ����������ж�ʹ��
    
    /* 11.��ȡһ�η������������ */
    txmail_free = ((CAN1->TSR & CAN_TSR_TME0) ? 1 : 0) +
             ((CAN1->TSR & CAN_TSR_TME1) ? 1 : 0) +
             ((CAN1->TSR & CAN_TSR_TME2) ? 1 : 0);
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
    uint32_t timeout = 0xFFFF;
    /* Ѱ�ҿ������� */
    for(mailbox = 0; mailbox < 3; mailbox++) {
      if((CAN1->sTxMailBox[mailbox].TIR & (1UL << 0)) == 0)
         break;
    }
    if(mailbox >= 3)
        return 1; // �޿�������
    /* ��ո����� */
    CAN1->sTxMailBox[mailbox].TIR  = 0;
    CAN1->sTxMailBox[mailbox].TDTR = 0;
    CAN1->sTxMailBox[mailbox].TDLR = 0;
    CAN1->sTxMailBox[mailbox].TDHR = 0;
    /* ��׼IDд��TIR��[31:21]����׼֡, RTR=0, IDE=0 */
    CAN1->sTxMailBox[mailbox].TIR |= (stdId << 21);
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
    /* ���������� */
    CAN1->sTxMailBox[mailbox].TIR |= CAN_TI0R_TXRQ;
    /* ��ѯ�ȴ�TXRQ�����ʱ */
    while((CAN1->sTxMailBox[mailbox].TIR & CAN_TI0R_TXRQ) && --timeout);
    if(timeout == 0) {
        // ����ʧ��(��ACK��λ����), ����2
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
    if (txmail_free > 0) {
        /* ��ո����䲢����ID��DLC������ */
        CAN1->sTxMailBox[mailbox].TIR  = 0;
        CAN1->sTxMailBox[mailbox].TDTR = (DLC & 0x0F);
        CAN1->sTxMailBox[mailbox].TDLR = 0;
        CAN1->sTxMailBox[mailbox].TDHR = 0;
        CAN1->sTxMailBox[mailbox].TIR |= (stdId << 21);

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
uint8_t CAN_Check_Error(CAN_ESR_t* can_esr) {
    /* ��ȡCAN����״̬�Ĵ�����ESR�� */
    uint32_t esr = CAN1->ESR;

    /* �������д����ֶ� */
    can_esr->lec   = (esr & CAN_ESR_LEC)   >> CAN_ESR_LEC_Pos;   // ��������루Last Error Code��
    can_esr->tec   = (esr & CAN_ESR_TEC)   >> CAN_ESR_TEC_Pos;   // ���ʹ����������Transmit Error Counter��
    can_esr->rec   = (esr & CAN_ESR_REC)   >> CAN_ESR_REC_Pos;   // ���մ����������Receive Error Counter��
    can_esr->epvf  = (esr & CAN_ESR_EPVF)  >> CAN_ESR_EPVF_Pos;  // ���󱻶���־��Error Passive Flag��
    can_esr->ewgf  = (esr & CAN_ESR_EWGF)  >> CAN_ESR_EWGF_Pos;  // ���󾯸��־��Error Warning Flag��
    can_esr->boff  = (esr & CAN_ESR_BOFF)  >> CAN_ESR_BOFF_Pos;  // ���߹رձ�־��Bus-Off Flag��

    /* ���LEC�������ֶΣ����Ӧλд0����� */
    //CAN1->ESR &= ~CAN_ESR_LEC;

    /* ���������س̶ȷּ����أ�BOFF > EPVF > LEC > EWGF��*/
    if (can_esr->boff == 0x01)  return 3;    // ���߹ر�״̬�������أ���ҪӲ����λ��
    if (can_esr->epvf == 0x01)  return 2;    // ���󱻶�״̬��TEC/REC > 127��
    if (can_esr->lec == 0x01)   return 1;    // Э�����λ���/��ʽ/ACK�ȴ���
    if (can_esr->ewgf == 0x01)  return 4;    // ����״̬��TEC/REC > 96��
    return 0;                        // �޴���
}

void CAN_BusOff_Recover(void)
{
    CAN1->MCR |= CAN_MCR_SLEEP;  // ����˯��ģʽ��ֹͣ�շ���
    CAN_Config();                // ���³�ʼ��һ��CAN
}







