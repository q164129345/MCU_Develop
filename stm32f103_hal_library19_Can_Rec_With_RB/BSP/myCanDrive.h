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
    volatile uint8_t RxData[8];
}CANMsg_t;


void CAN_Send_Msg_Serial(void);
uint8_t CAN_Send_Msg_No_Serial(void);
void CAN_Config(void);
void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan);
void CAN_FIFO1_Overflow_Handler(void);

#ifdef __cplusplus
}
#endif

#endif /* __MYCANDRIVE_H */

