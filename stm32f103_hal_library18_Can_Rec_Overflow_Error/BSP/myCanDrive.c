#include "myCanDrive.h"

volatile uint8_t txmail_free = 0;
volatile uint32_t canSendError = 0;
volatile uint32_t g_RxCount = 0;
volatile uint32_t g_RxOverflowError = 0;
CANMsg_t g_CanMsg = {0,}; // ȫ�ֱ��������ڹ۲�

/**
  * @brief  ͨ��CAN���߷��͹̶��������ݣ������кţ���
  * @note   �˺������ñ�׼CAN����֡��ID: 0x123��������8�ֽڹ̶�����(0x01-0x08)��
  *         ��������������������ӳ�1ms�ȴ�������ʧ��ʱ����������
  *         ��������Ҫ�򵥿ɿ����͹̶���ʽ���ݵĳ�����
  * @retval None
  */
void CAN_Send_Msg_Serial(void)
{
    CAN_TxHeaderTypeDef TxHeader;
    uint32_t TxMailbox;
    uint8_t TxData[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    /* ����CAN����֡���� */
    TxHeader.StdId = 0x123;          // ��׼��ʶ�����ɸ���ʵ�������޸�
    TxHeader.ExtId = 0;              // ���ڱ�׼֡����չ��ʶ����Ч
    TxHeader.IDE = CAN_ID_STD;       // ��׼֡
    TxHeader.RTR = CAN_RTR_DATA;     // ����֡
    TxHeader.DLC = 8;                // ���ݳ���Ϊ8�ֽ�
    TxHeader.TransmitGlobalTime = DISABLE; // ������ȫ��ʱ��
    /* ���û�з������䣬�ӳٵȴ��������ʵʱ����ϵͳ����������Ǹ��Ӻõķ����� */
    if (HAL_CAN_GetTxMailboxesFreeLevel(&hcan) == 0) {
        LL_mDelay(1);
    }
    /* ����CAN��Ϣ */
    if (HAL_CAN_AddTxMessage(&hcan, &TxHeader, TxData, &TxMailbox) != HAL_OK) {
    /* ����ʧ�ܣ���������� */
        Error_Handler();
    }
}
/**
  * @brief  ͨ��CAN���߷��͹̶��������ݣ������кţ�������״̬��飩��
  * @note   �˺����ڷ����������ʱ��txmail_free > 0�����ñ�׼CAN����֡��ID: 0x200����
  *         ����8�ֽڹ̶�����(0x01-0x08)���ɹ�����ٿ������������
  *         ����ʧ��ʱ�������������䲻����ʱ���ط���ʧ��״̬��
  * @retval uint8_t ����״̬��0��ʾ�ɹ���1��ʾ���䲻���ã�����ʧ��
  */
uint8_t CAN_Send_Msg_No_Serial(void)
{
    if (txmail_free > 0) {
        CAN_TxHeaderTypeDef TxHeader;
        uint32_t TxMailbox;
        uint8_t TxData[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
        /* ����CAN����֡���� */
        TxHeader.StdId = 0x200;          // ��׼��ʶ�����ɸ���ʵ�������޸�
        TxHeader.ExtId = 0;              // ���ڱ�׼֡����չ��ʶ����Ч
        TxHeader.IDE = CAN_ID_STD;       // ��׼֡
        TxHeader.RTR = CAN_RTR_DATA;     // ����֡
        TxHeader.DLC = 8;                // ���ݳ���Ϊ8�ֽ�
        TxHeader.TransmitGlobalTime = DISABLE; // ������ȫ��ʱ��
        /* ����CAN��Ϣ */
        if (HAL_CAN_AddTxMessage(&hcan, &TxHeader, TxData, &TxMailbox) != HAL_OK) {
        /* ����ʧ�ܣ���������� */
            Error_Handler();
        }
        txmail_free--; // �õ�һ����������
        return 0;
    } else {
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
    if (HAL_CAN_ConfigFilter(hcan, &filterConfig) != HAL_OK)
    {
        Error_Handler();
    }
}
/**
  * @brief  CAN��ʼ��
  */
void CAN_Config(void)
{
    txmail_free = HAL_CAN_GetTxMailboxesFreeLevel(&hcan); // ��ȡ��������Ŀ���������һ�㶼��3��
    HAL_CAN_ActivateNotification(&hcan, CAN_IT_TX_MAILBOX_EMPTY); // ������������ж�
    //HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO1_MSG_PENDING); // ��������FIFO1�жϣ���FIFO1������Ϣ����ʱ�������ж�
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
    CAN_RxHeaderTypeDef RxHeader;
    uint8_t RxData[8];
    uint32_t pendingMessages = HAL_CAN_GetRxFifoFillLevel(hcan, CAN_RX_FIFO1); // ��ȡFIFO1һ���ж��ٸ�CAN����
    
    while(pendingMessages) {
        /* ��FIFO1�ж�ȡ������Ϣ */
        if(HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO1, &RxHeader, RxData) == HAL_OK) {
            // ��������ӶԽ��յ����ݵĴ������
            // ���磺�����û��Զ���ĺ�����������
            // Process_CAN_Message(&RxHeader, RxData);
            // ���磺�����յ������ݷ���Ringbuffer
            
            /* �����յ���CAN��Ϣ����ȫ�ֱ��������ڹ۲졣ʵ����ĿӦ�����жϽ��յ���CAN���ķ���ringbuffer,Ȼ������ѭ������CAN���� */
            g_CanMsg.RxHeader = RxHeader;
            memcpy((uint8_t*)g_CanMsg.RxData,(uint8_t*)RxData,8);
            g_RxCount++; // �ۼӽ��յ���CAN��������
            // ʾ�����򵥵ؽ����յ����ݴ���һ��ȫ�ֱ������ӡ���������ʱ�д��ڻ��������Է�ʽ��
        } else {
            // �����ȡ��Ϣʧ�ܣ�Ҳ������������������
        }
        pendingMessages = HAL_CAN_GetRxFifoFillLevel(hcan, CAN_RX_FIFO1); // ����FIFO1��CAN���ĵ�����
    }
}

void CAN_FIFO1_Overflow_Handler(void)
{
    if (CAN1->RF1R & CAN_RF1R_FOVR1) { // ��ȡFOVR1λ��FIFO1����жϣ��������ǲ��Ǳ���1
        g_RxOverflowError++;
    }
}

