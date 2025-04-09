#include "myCanDrive.h"
#include "can.h"

volatile uint8_t txmail_free = 0;
volatile uint32_t canSendError = 0;

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
  * @brief  CAN��ʼ��
  */
void CAN_Config(void)
{
    txmail_free = HAL_CAN_GetTxMailboxesFreeLevel(&hcan); // ��ȡ��������Ŀ���������һ�㶼��3��
    HAL_CAN_ActivateNotification(&hcan, CAN_IT_TX_MAILBOX_EMPTY); // ������������ж�
    if (HAL_CAN_Start(&hcan) != HAL_OK) {
      Error_Handler(); // ����ʧ�ܣ����������
    }
}




