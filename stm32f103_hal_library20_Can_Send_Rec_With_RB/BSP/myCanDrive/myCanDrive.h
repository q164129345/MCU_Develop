/**
 * @file myCanDrive.h
 * @brief definitions for CAN driver module
 * @note 
 * 
 * @version 1.0.0
 * @date 2025-04-09
 * @author Wallace.zhang
 * 
 * @copyright
 * 
 * @license SPDX-License-Identifier: MIT
 */
#ifndef __MYCANDRIVE_H
#define __MYCANDRIVE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "can.h"
    
typedef struct {
    CAN_RxHeaderTypeDef RxHeader;
    uint8_t RxData[8];
}CANMsg_t;

typedef struct {
    uint32_t CanId;
    uint8_t  Len;
    uint8_t  data[8];
}CANTXMsg_t;



void CAN_Send_STD_DATA_Msg_Serial(uint32_t canid, uint8_t* data, uint8_t len);
uint8_t CAN_Send_STD_DATA_Msg_No_Serial(uint32_t canid, uint8_t* data, uint8_t len);
void CAN_Config(void);
void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan);
void CAN_FIFO1_Overflow_Handler(void);
void CAN_Get_CANMsg_From_RB_To_TXMailBox_IT(void);
void CAN_Get_CANMsg_From_RB_To_TXMailBox(void);
void CAN_TX_Complete_Handler(void);
bool CAN_Send_CAN_STD_Message(uint32_t canid, uint8_t* data, uint8_t len);

#ifdef __cplusplus
}
#endif

#endif /* __MYCANDRIVE_H */

