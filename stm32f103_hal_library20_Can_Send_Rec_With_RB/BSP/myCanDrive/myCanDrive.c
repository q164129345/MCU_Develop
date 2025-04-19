#include "myCanDrive.h"

volatile uint8_t txmail_free = 0;             // �������������
volatile uint32_t canSendError = 0;           // ͳ�Ʒ���ʧ�ܴ���
volatile uint32_t g_RxCount = 0;              // ͳ�ƽ��յ�CAN��������
volatile uint32_t g_HandleRxMsg = 0;          // ͳ�ƴ�ringbuffer�ó��������CAN���ĵ�����
volatile uint32_t g_RxOverflowError = 0;      // ͳ��RX FIFO1�������
volatile uint32_t g_RXRingbufferOverflow = 0; // ͳ��ringbuffer�������

/* ringbuffer */
volatile lwrb_t g_CanRxRBHandler; // ʵ����ringbuffer
volatile CANMsg_t g_CanRxRBDataBuffer[50] = {0,}; // ringbuffer���棨�����Դ�50��CAN��Ϣ��

/**
  * @brief  ����һ����׼����֡��CAN��Ϣ�����з�ʽ��
  * 
  * �˺������ô��з�ʽ���ͱ�׼CAN����֡�������ڲ��������÷���֡����ز�����
  * Ȼ���ⷢ�������Ƿ���С�������������Ѿ����꣬��ͨ������ʱ�ȴ����ٷ������ݡ�
  * �������ʧ�ܣ������Error_Handler()���д�����
  *
  * @param  canid  ��׼CAN��ʶ��
  * @param  data   ָ��Ҫ���͵����������ָ�룬���ݳ�����len����ȷ����0~8�ֽڣ�
  * @param  len    �����ֽ�����DLC����Ӧ������8
  * 
  * @note   ���������䲻����ʱ��������ͨ����ʱ��LL_mDelay(1)���ȴ�һ��ʱ�䣬
  *         �����ʱ�����޷�������䣬������ȴ������շ���ʧ�ܡ�
  * 
  * @retval None
  */
void CAN_Send_STD_DATA_Msg_Serial(uint32_t canid, uint8_t* data, uint8_t len)
{
    uint32_t TxMailbox;
    uint32_t timeout = 100; // ���ȴ�100ms
    CAN_TxHeaderTypeDef txHeader;
    
    if (len > 8) {
        Error_Handler(); // ���ݾ������󣬿�ѡ��ضϻ���������
    }
    
    /* ����CAN����֡���� */
    txHeader.StdId = canid;       // ��׼��ʶ�����ɸ���ʵ�������޸�
    txHeader.ExtId = 0;           // ���ڱ�׼֡����չ��ʶ����Ч
    txHeader.IDE = CAN_ID_STD;    // ��׼֡
    txHeader.RTR = CAN_RTR_DATA;  // ����֡
    txHeader.DLC = len;           // ���ݳ���
    txHeader.TransmitGlobalTime = DISABLE; // ������ȫ��ʱ��
    /* ���û�з������䣬�ӳٵȴ��������ʵʱ����ϵͳ����������Ǹ��Ӻõķ����� */
    while ((HAL_CAN_GetTxMailboxesFreeLevel(&hcan) == 0) && (timeout > 0)) {
        LL_mDelay(1);
        timeout--;
    }
    /* �ж��Ƿ�ʱδ������� */
    if ((timeout == 0) && (HAL_CAN_GetTxMailboxesFreeLevel(&hcan) == 0)) {
        canSendError++; // ��¼һ�η��ͳ�ʱ����
        return;
    }
    /* ����CAN��Ϣ */
    if (HAL_CAN_AddTxMessage(&hcan, &txHeader, data, &TxMailbox) != HAL_OK) {
    /* ����ʧ�ܣ���������� */
        Error_Handler();
    }
}

/**
  * @brief  ����һ����׼����֡��CAN��Ϣ���Ǵ��з�ʽ��
  * 
  * �˺������÷Ǵ��з�ʽ���ͱ�׼CAN����֡������ǰ���ȼ��ȫ�ֱ���txmail_free��
  * �ж��Ƿ��п��еķ������䡣����У�������֡������������Ϣ��ͬʱ���ٿ������������
  * ��������canSendError�����Լ�¼���󣬲����ش�����롣
  *
  * @param  canid  ��׼CAN��ʶ��
  * @param  data   ָ��Ҫ���͵����������ָ�룬���ݳ�����len����ȷ����0~8�ֽڣ�
  * @param  len    �����ֽ�����DLC����Ӧ������8
  *
  * @note   ��������ȫ�ֱ���txmail_free����ⷢ������Ŀ���״̬��
  *         ���������䲻��ʱ��������ȫ�ֱ���canSendError�����������ش���
  * 
  * @retval uint8_t  ����0��ʾ���ͳɹ�������1��ʾ��������������δ�ܷ���CAN��Ϣ
  */
uint8_t CAN_Send_STD_DATA_Msg_No_Serial(uint32_t canid, uint8_t* data, uint8_t len)
{
    if (len > 8) {
        Error_Handler(); // ���ݾ������󣬿�ѡ��ضϻ���������
    }
    
    if (txmail_free > 0) {
        uint32_t TxMailbox;
        CAN_TxHeaderTypeDef txHeader;
        /* ����CAN����֡���� */
        txHeader.StdId = canid;      // ��׼��ʶ�����ɸ���ʵ�������޸�
        txHeader.ExtId = 0;          // ���ڱ�׼֡����չ��ʶ����Ч
        txHeader.IDE = CAN_ID_STD;   // ��׼֡
        txHeader.RTR = CAN_RTR_DATA; // ����֡
        txHeader.DLC = len;          // ���ݳ���
        txHeader.TransmitGlobalTime = DISABLE; // ������ȫ��ʱ��
        /* ����CAN��Ϣ */
        if (HAL_CAN_AddTxMessage(&hcan, &txHeader, data, &TxMailbox) != HAL_OK) {
        /* ����ʧ�ܣ���������� */
            Error_Handler();
        }
        txmail_free--; // �õ�һ����������
        return 0;
    } else {
        canSendError++; // �����������ˣ����
        return 1;
    }
}

/**
  * @brief  ����CAN�˲���������Ϊ�������κ���Ϣ
  * @note   �˺���ʹ�ñ�ʶ������ģʽ��32λ�˲����������˲���ID������ֵ����Ϊ0
  * @retval None
  */
static void CAN_FilterConfig_AllMessages(CAN_HandleTypeDef *hcan)
{
    CAN_FilterTypeDef filterConfig;

    // ѡ���˲������У�����ʵ���������ֻ��һ��CAN��һ�����0���˲������У�
    filterConfig.FilterBank = 0;
    // ����Ϊ��ʶ������λģʽ
    filterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
    // �����˲���Ϊ32λģʽ
    filterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
    // �����˲�����ʶ����Ϊ0
    filterConfig.FilterIdHigh = 0x0000;
    filterConfig.FilterIdLow  = 0x0000;
    // ��������ֵΪ0���������κα��ؽ��й���
    filterConfig.FilterMaskIdHigh = 0x0000;
    filterConfig.FilterMaskIdLow  = 0x0000;
    // �����յ�����Ϣ���䵽FIFO0��Ҳ����ѡ��FIFO1������ʵ��Ӧ�����ã�
    filterConfig.FilterFIFOAssignment = CAN_RX_FIFO1;
    // �����˲���
    filterConfig.FilterActivation = ENABLE;
    // �����˲���������ʧ������������
    if (HAL_CAN_ConfigFilter(hcan, &filterConfig) != HAL_OK) {
        Error_Handler();
    }
}

/**
 * @brief  �ӻ��λ������ж�ȡ���������й���� CAN ��Ϣ
 * @details ��������ѯȫ�ֻ��λ����� g_CanRxRBHandler��ֻҪ���п�������
 *          ���ڻ����һ�������� CANMsg_t ��Ϣ���ȣ��Ͳ��ϣ�
 *            1. ��һ����Ϣ��ȡ�����ر��� canMsg
 *            2. ȫ�ּ����� g_HandleRxMsg ��������¼�Ѵ������Ϣ��
 *            3. ���� canMsg.RxHeader.StdId �� switch-case �зַ��������ҵ�����֧
 * @note   - �û����� switch-case ����䲻ͬ StdId ��Ӧ�Ĵ����߼�  
 *         - ���޸�����Ϣ�ɶ�����������  
 * @retval None
 */
void CAN_Data_Process(void)
{
    while(lwrb_get_full((lwrb_t*)&g_CanRxRBHandler) >= sizeof(CANMsg_t)) {
        CANMsg_t canMsg;
        lwrb_read((lwrb_t*)&g_CanRxRBHandler, &canMsg, sizeof(CANMsg_t)); // ��ringbuffer�ж�ȡһ��CAN��Ϣ
        g_HandleRxMsg++; // ��һ��CAN��Ϣ��ringbuffer�ó�������
        /* ���������CANID����ҵ�����.�ȵ� */
//        switch(canMsg.RxHeader.StdId) {
//            case 0x200:
//                // TODO: ���� ID = 0x200 ����Ϣ
//                break;
//            case 0x201:
//                // TODO: ���� ID = 0x201 ����Ϣ
//                break;
//            case 0x202:
//            case 0x203:
//                break;
//            default:
//                break;
//        }
    }
}

void CAN_Send_CANMsg_To_CANBUS(void)
{
    
    
    
    
    
}



/**
  * @brief  CAN��ʼ��
  */
void CAN_Config(void)
{
    lwrb_init((lwrb_t*)&g_CanRxRBHandler, (uint8_t*)g_CanRxRBDataBuffer, sizeof(g_CanRxRBDataBuffer) + 1); // ringbuffer��ʼ��
    
    txmail_free = HAL_CAN_GetTxMailboxesFreeLevel(&hcan); // ��ȡ��������Ŀ���������һ�㶼��3��
    HAL_CAN_ActivateNotification(&hcan, CAN_IT_TX_MAILBOX_EMPTY); // ������������ж�
    HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO1_MSG_PENDING); // ��������FIFO1�жϣ���FIFO1������Ϣ����ʱ�������ж�
    HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO1_OVERRUN); // ��������FIFO1����ж�
    CAN_FilterConfig_AllMessages(&hcan); // ���ù�����
    if (HAL_CAN_Start(&hcan) != HAL_OK) {
      Error_Handler(); // ����ʧ�ܣ����������
    }
}

/**
  * @brief  CAN�����жϻص�������������յ���CAN��Ϣ
  * @param  hcan: CAN���
  * @retval None
  */
void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    CANMsg_t canMsg = {0,};
    uint32_t pendingMessages = HAL_CAN_GetRxFifoFillLevel(hcan, CAN_RX_FIFO1); // ��ȡFIFO1һ���ж��ٸ�CAN����
    
    while(pendingMessages) {
        /* ��FIFO1�ж�ȡ������Ϣ */
        if(HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO1, &canMsg.RxHeader, canMsg.RxData) == HAL_OK) {
            g_RxCount++; // �ۼӽ��յ���CAN��������
            if(lwrb_get_free((lwrb_t*)&g_CanRxRBHandler) < sizeof(CANMsg_t)) { // �ж�ringbuffer�Ƿ񱻼���
                g_RXRingbufferOverflow++;    // �ۼ�ringbuffer���ȫ�ּ�����
                lwrb_reset((lwrb_t*)&g_CanRxRBHandler); // ���ringbuffer
            }
            lwrb_write((lwrb_t*)&g_CanRxRBHandler, &canMsg, sizeof(CANMsg_t)); // ��CAN���ķ���ringbuffer
        } else {
            // �����ȡ��Ϣʧ�ܣ�Ҳ������������������
        }
        pendingMessages = HAL_CAN_GetRxFifoFillLevel(hcan, CAN_RX_FIFO1); // ����FIFO1��CAN���ĵ�����
    }
}

/**
  * @brief  CAN����жϴ���������ȫ���ж�CAN1_RX1_IRQHandler()�����
  * @param  Note
  * @retval None
  */
void CAN_FIFO1_Overflow_Handler(void)
{
    if (CAN1->RF1R & CAN_RF1R_FOVR1) { // ��ȡFOVR1λ��FIFO1����жϣ��������ǲ��Ǳ���1
        g_RxOverflowError++;
    }
}

