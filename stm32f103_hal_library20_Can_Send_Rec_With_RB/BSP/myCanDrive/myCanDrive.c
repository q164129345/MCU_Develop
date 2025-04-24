#include "myCanDrive.h"

volatile uint8_t txmail_free = 0;             // �������������
volatile uint32_t canSendError = 0;           // ͳ�Ʒ���ʧ�ܴ���
volatile uint64_t g_RxCount = 0;              // ͳ�ƽ��յ�CAN��������
volatile uint64_t g_TxCount = 0;              // ͳ�Ʒ��͵�CAN��������
volatile uint64_t g_HandleRxMsg = 0;          // ͳ�ƴ�RX Ringbuffer�ó��������CAN���ĵ�����
volatile uint32_t g_RxOverflowError = 0;      // ͳ��RX FIFO1�������
volatile uint32_t g_RXRingbufferOverflow = 0; // ͳ��ringbuffer�������

/* CAN����ringbuffer */
volatile lwrb_t g_CanRxRBHandler; // ʵ����RX ringbuffer
volatile CANMsg_t g_CanRxRBDataBuffer[50] = {0,}; // RX ringbuffer���棨�����Դ�50��CAN��Ϣ��

/* CAN����ringbuffer */
volatile lwrb_t g_CanTxRBHandler; // ʵ����TX ringbuffer
volatile CANTXMsg_t g_CanTxRBDataBuffer[50] = {0,}; // TX ringbuffer���棨�����Դ�50����Ҫ���͵�CAN��Ϣ��

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
    g_TxCount++; // �ɹ�����һ֡CAN����
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
        g_TxCount++; // �ɹ�����һ֡CAN����
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
    /* ���������һ֡CAN��Ϣ������whileѭ������ringbufferȡ��CAN���ģ����ŷ��������� */
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
/**
 * @brief ���Է���һ֡���� CAN ���ģ��� TX RingBuffer �� CAN ���䣩
 *
 * ���ա��� peek���ɹ��� skip���� 0 ��֡���ԣ�  
 * 1. ��û�п�������� RingBuffer �����������ģ����������ء�  
 * 2. `lwrb_peek()` ��������ǰ��ı��ģ�  
 * 3. ���� @ref CAN_Send_STD_DATA_Msg_No_Serial ���ͣ�  
 * 4. ���ڷ��ͳɹ�ʱִ�� `lwrb_skip()` ����������֡��  
 *
 * @note  
 * - �ú����ȿ����жϻ�����Ҳ������ѭ���������ã����÷������б�֤
 *   �� RingBuffer ��صĲ������⣨ʾ������������ѭ����ʱ�ر�
 *   `CAN_IT_TX_MAILBOX_EMPTY`����  
 * - ����ʧ�ܽ��ۼ� @p canSendError �����������Ƴ����ģ��ȴ��´����ԡ�  
 *
 * @retval true   �ɹ��� 1 ֡����д�����䣬���Ѵ� RingBuffer �Ƴ�  
 * @retval false  �޿������� / �޴��������� / ����ʧ��
 */
static bool CAN_TxOnceFromRB(void)
{
    CANTXMsg_t msg;
    if (txmail_free == 0) {
        return false; // û����
    }
    if (lwrb_get_full((lwrb_t*)&g_CanTxRBHandler) < sizeof(CANTXMsg_t)) {
        return false; // û����
    }
    /* ��TX ringbuffer����Ϣ���˵�TX�������� */
    lwrb_peek((lwrb_t*)&g_CanTxRBHandler, 0, &msg, sizeof(CANTXMsg_t)); // ֻ����(peek)һ֡��������
    if (CAN_Send_STD_DATA_Msg_No_Serial(msg.CanId, msg.data, msg.Len) == 0) {
        lwrb_skip((lwrb_t*)&g_CanTxRBHandler, sizeof(CANTXMsg_t)); // �ɹ�����ʽ����֡�� RingBuffer ɾ��
        return true;
    }
    canSendError++; // �����Ϻ��ٵ�����
    return false;
}

/**
 * @brief     �� CAN TX ����ж��е��õĿ�����������
 *
 * �������� @ref CAN_TxOnceFromRB ֱ���޷���д�����䣨��������
 * RingBuffer û���ݣ������� ISR����ִ���κι��жϲ�����
 *
 * @note ��Ӧ�� @p USB_HP_CAN1_TX_IRQHandler �ȷ������ IRQ �ص�����á�
 */
void CAN_Get_CANMsg_From_RB_To_TXMailBox_IT(void)
{
    while (CAN_TxOnceFromRB()) { }
}

/**
 * @brief     ��ѭ�����ڵ��õ�������������
 *
 * Ϊ������ ISR ��������ͬһ TX RingBuffer��  
 * - ���뺯��ǰ��ʱ�ر� `CAN_IT_TX_MAILBOX_EMPTY` �жϣ�  
 * - ͨ��ѭ������ @ref CAN_TxOnceFromRB �����п�������������  
 * - ���������¿����жϡ�  
 *
 * @note ������ 1 ms ������ϵͳ�����ڵ���һ�Σ��Է����˸�����
 *       ISR Ƶ�ʲ�������� RingBuffer��
 */
void CAN_Get_CANMsg_From_RB_To_TXMailBox(void)
{
    /* ----------- ���˳��������� �� �޿������� ----------- */
    if (txmail_free == 0) {
        return; // ����ȫ��
    }
    if (lwrb_get_full((lwrb_t*)&g_CanTxRBHandler) < sizeof(CANTXMsg_t)) {
        return; // ������û����
    }
    /* �� TX-Mailbox-Empty �жϣ���ֹ���� */
    __HAL_CAN_DISABLE_IT(&hcan, CAN_IT_TX_MAILBOX_EMPTY);
    /* ����Ȼ���ŵ�����һ�η��� */
    while (CAN_TxOnceFromRB()) { }
    /* �ָ��ж� */
    __HAL_CAN_ENABLE_IT(&hcan, CAN_IT_TX_MAILBOX_EMPTY);
}

/**
 * @brief  ��һ֡��׼ CAN ����֡д�� Tx ���λ�����
 *
 * �ϲ�Ӧ�õ��ô˺������ɡ��Ŷӡ�����һ֡��׼ CAN ���ģ�  
 * �����������ж� + ��ѭ�������첽�ذѻ������еı���
 * ȡ����д�� CAN1 �� 3 ���������䣬ʵ��**����������**��
 *
 * @param[in]  canid  11 λ��׼��ʶ������Χ 0 �C 0x7FF��
 * @param[in]  data   ָ����������ݵĻ�������0 �C 8 �ֽڣ�
 * @param[in]  len    ���ݳ��ȣ�DLC����ȡֵ 0 �C 8
 *
 * @retval true   �ѳɹ��ѱ�֡д�� Tx RingBuffer  
 * @retval false  Tx RingBuffer �ռ䲻�㣬��ǰ֡������
 *
 * @note
 * - **�̰߳�ȫ**��������������ѭ���������������е��ã�  
 *   ����Ӳ�ж�ֻ������������д���� @p lwrb_write() ��ʵ��
 *   ��д������ȫ�������������ٽ�����  
 * - ����������ʱ��ʵ��ֱ�ӷ��� @c false �������Ǿ����ݣ���
 *   ��Ҫ�����������ȡ����ԣ����ȵ��� @p lwrb_skip() �������
 *   һ֡����д�룬��Ӧ�ڴ���������ȷע�͡� 
 */
bool CAN_Send_CAN_STD_Message(uint32_t canid, uint8_t* data, uint8_t len)
{
    if (lwrb_get_free((lwrb_t*)&g_CanTxRBHandler) < sizeof(CANTXMsg_t)) {
        return false;
    }
    /* ��TX Ringbuffer�����һ֡CAN���� */
    CANTXMsg_t canMsg = { .CanId = canid, .Len = len };
    memcpy(canMsg.data, data, len);
    lwrb_write((lwrb_t*)&g_CanTxRBHandler, &canMsg, sizeof(CANTXMsg_t)); 
    return true;
}

/**
  * @brief  CAN��ʼ��
  */
void CAN_Config(void)
{
    lwrb_init((lwrb_t*)&g_CanRxRBHandler, (uint8_t*)g_CanRxRBDataBuffer, sizeof(g_CanRxRBDataBuffer) + 1); // RX Ringbuffer��ʼ��
    lwrb_init((lwrb_t*)&g_CanTxRBHandler, (uint8_t*)g_CanTxRBDataBuffer, sizeof(g_CanTxRBDataBuffer) + 1); // TX Ringbuffer��ʼ��
    
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

/**
  * @brief  ������ CAN ������ɻص�
  * @note   HAL �� USB_HP_CAN1_TX_IRQHandler()��HAL_CAN_IRQHandler() �
  *         �����μ�� RQCP0/1/2 ���ֱ���� 3 �����ص���
  *         ����������ȫ���������˺������ɡ�
  */
void CAN_TxCompleteCommonCallback(CAN_HandleTypeDef *hcan)
{
    /* ͨ���Ĵ�����ʽ�����Ӹ�Ч�������·�������Ŀ������� */
    txmail_free = ((hcan->Instance->TSR & CAN_TSR_TME0) ? 1 : 0) +
                 ((hcan->Instance->TSR & CAN_TSR_TME1) ? 1 : 0) +
                 ((hcan->Instance->TSR & CAN_TSR_TME2) ? 1 : 0);
    /* ��TX ringbuffer��CAN���İᵽCAN TX���� */
    CAN_Get_CANMsg_From_RB_To_TXMailBox_IT();
}

/* ---------- �� 3 �����ص�ȫ��ӳ�䵽ͬһ��ʵ�� ---------- */
void HAL_CAN_TxMailbox0CompleteCallback(CAN_HandleTypeDef *hcan)
        __attribute__((alias("CAN_TxCompleteCommonCallback")));
void HAL_CAN_TxMailbox1CompleteCallback(CAN_HandleTypeDef *hcan)
        __attribute__((alias("CAN_TxCompleteCommonCallback")));
void HAL_CAN_TxMailbox2CompleteCallback(CAN_HandleTypeDef *hcan)
        __attribute__((alias("CAN_TxCompleteCommonCallback")));

