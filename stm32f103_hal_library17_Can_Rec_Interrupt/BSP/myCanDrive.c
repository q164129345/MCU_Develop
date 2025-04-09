#include "myCanDrive.h"
#include "can.h"

volatile uint8_t txmail_free = 0;
volatile uint32_t canSendError = 0;

/**
  * @brief  通过CAN总线发送固定序列数据（带序列号）。
  * @note   此函数配置标准CAN数据帧（ID: 0x123），发送8字节固定数据(0x01-0x08)，
  *         如果发送邮箱已满，将延迟1ms等待，发送失败时触发错误处理。
  *         适用于需要简单可靠发送固定格式数据的场景。
  * @retval None
  */
void CAN_Send_Msg_Serial(void)
{
    CAN_TxHeaderTypeDef TxHeader;
    uint32_t TxMailbox;
    uint8_t TxData[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    /* 配置CAN发送帧参数 */
    TxHeader.StdId = 0x123;          // 标准标识符，可根据实际需求修改
    TxHeader.ExtId = 0;              // 对于标准帧，扩展标识符无效
    TxHeader.IDE = CAN_ID_STD;       // 标准帧
    TxHeader.RTR = CAN_RTR_DATA;     // 数据帧
    TxHeader.DLC = 8;                // 数据长度为8字节
    TxHeader.TransmitGlobalTime = DISABLE; // 不发送全局时间
    /* 如果没有发送邮箱，延迟等待（如果有实时操作系统，发起调度是更加好的方案） */
    if (HAL_CAN_GetTxMailboxesFreeLevel(&hcan) == 0) {
        LL_mDelay(1);
    }
    /* 发送CAN消息 */
    if (HAL_CAN_AddTxMessage(&hcan, &TxHeader, TxData, &TxMailbox) != HAL_OK) {
    /* 发送失败，进入错误处理 */
        Error_Handler();
    }
}
/**
  * @brief  通过CAN总线发送固定序列数据（无序列号，带邮箱状态检查）。
  * @note   此函数在发送邮箱可用时（txmail_free > 0）配置标准CAN数据帧（ID: 0x200），
  *         发送8字节固定数据(0x01-0x08)，成功后减少可用邮箱计数，
  *         发送失败时触发错误处理，邮箱不可用时返回发送失败状态。
  * @retval uint8_t 发送状态：0表示成功，1表示邮箱不可用，发送失败
  */
uint8_t CAN_Send_Msg_No_Serial(void)
{
    if (txmail_free > 0) {
        CAN_TxHeaderTypeDef TxHeader;
        uint32_t TxMailbox;
        uint8_t TxData[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
        /* 配置CAN发送帧参数 */
        TxHeader.StdId = 0x200;          // 标准标识符，可根据实际需求修改
        TxHeader.ExtId = 0;              // 对于标准帧，扩展标识符无效
        TxHeader.IDE = CAN_ID_STD;       // 标准帧
        TxHeader.RTR = CAN_RTR_DATA;     // 数据帧
        TxHeader.DLC = 8;                // 数据长度为8字节
        TxHeader.TransmitGlobalTime = DISABLE; // 不发送全局时间
        /* 发送CAN消息 */
        if (HAL_CAN_AddTxMessage(&hcan, &TxHeader, TxData, &TxMailbox) != HAL_OK) {
        /* 发送失败，进入错误处理 */
            Error_Handler();
        }
        txmail_free--; // 用掉一个发送邮箱
        return 0;
    } else {
        return 1;
    }
}

/**
  * @brief  CAN初始化
  */
void CAN_Config(void)
{
    txmail_free = HAL_CAN_GetTxMailboxesFreeLevel(&hcan); // 获取发送邮箱的空闲数量，一般都是3个
    HAL_CAN_ActivateNotification(&hcan, CAN_IT_TX_MAILBOX_EMPTY); // 启动发送完成中断
    if (HAL_CAN_Start(&hcan) != HAL_OK) {
      Error_Handler(); // 启动失败，进入错误处理
    }
}




