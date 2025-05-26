/**
 * @file myCanDrive_reg.h
 * @brief Register definitions for CAN driver module
 * 
 * This header file contains register definitions and related structures
 * for the CAN (Controller Area Network) driver module. It provides the
 * hardware abstraction layer for CAN controller registers.
 * 
 * @note 
 * 
 * @version 1.0.0
 * @date 2025-03-31
 * @author Wallace.zhang
 * 
 * @copyright
 * 
 * @license SPDX-License-Identifier: MIT
 */
#ifndef __MYCANDRIVE_REG_H
#define __MYCANDRIVE_REG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/**
 * @brief CAN控制器错误状态寄存器结构体 (CAN Error Status Register)
 * 
 * 用于描述CAN控制器的错误状态信息，通常对应硬件寄存器的映射。
 */
typedef struct {
    volatile uint8_t rec;    ///< Receive Error Counter (接收错误计数器), 范围: 0-255
    volatile uint8_t tec;    ///< Transmit Error Counter (发送错误计数器), 范围: 0-255
    volatile uint8_t lec;    ///< Last Error Code (最后一次错误代码), 常见值:
                    ///< 0 = No error
                    ///< 1 = Stuff error
                    ///< 2 = Form error
                    ///< 3 = Acknowledgment error
                    ///< 4 = Bit recessive error
                    ///< 5 = Bit dominant error
                    ///< 6 = CRC error
                    ///< 7 = Custom (由厂商定义)

    volatile uint8_t boff;   ///< Bus-Off Status (总线关闭状态), 布尔值:
                    ///< 0 = Node is in active state (节点处于活动状态)
                    ///< 1 = Node is in bus-off state (节点因错误过多被总线关闭)

    volatile uint8_t epvf;   ///< Error Passive or Warning Flag (错误被动/警告标志), 布尔值:
                    ///< 0 = Error active (错误主动状态)
                    ///< 1 = Error passive or warning (错误被动或警告状态)

    volatile uint8_t ewgf;   ///< Error Warning Flag (错误警告标志), 布尔值:
                    ///< 0 = Error count below warning threshold (错误计数低于警告阈值)
                    ///< 1 = Error count reached warning threshold (错误计数达到警告阈值)
} CAN_ESR_t;

// 从HAL库学习过来的结构体
typedef struct
{
  uint32_t StdId;    /*!< Specifies the standard identifier.
                          This parameter must be a number between Min_Data = 0 and Max_Data = 0x7FF. */

  uint32_t ExtId;    /*!< Specifies the extended identifier.
                          This parameter must be a number between Min_Data = 0 and Max_Data = 0x1FFFFFFF. */

  uint32_t IDE;      /*!< Specifies the type of identifier for the message that will be transmitted.
                          This parameter can be a value of @ref CAN_identifier_type */

  uint32_t RTR;      /*!< Specifies the type of frame for the message that will be transmitted.
                          This parameter can be a value of @ref CAN_remote_transmission_request */

  uint32_t DLC;      /*!< Specifies the length of the frame that will be transmitted.
                          This parameter must be a number between Min_Data = 0 and Max_Data = 8. */

  uint32_t Timestamp; /*!< Specifies the timestamp counter value captured on start of frame reception.
                          @note: Time Triggered Communication Mode must be enabled.
                          This parameter must be a number between Min_Data = 0 and Max_Data = 0xFFFF. */

  uint32_t FilterMatchIndex; /*!< Specifies the index of matching acceptance filter element.
                          This parameter must be a number between Min_Data = 0 and Max_Data = 0xFF. */

} CAN_RxHeaderTypeDef;

typedef struct {
    CAN_RxHeaderTypeDef RxHeader;
    uint8_t RxData[8];
}CANMsg_t;

typedef struct {
    uint32_t CanId;
    uint8_t  Len;
    uint8_t  data[8];
}CANTXMsg_t;


void CAN_Config(void);
uint8_t CAN_SendMessage_Blocking(uint32_t stdId, uint8_t *data, uint8_t DLC);
uint8_t CAN_SendMessage_NonBlocking(uint32_t stdId, uint8_t *data, uint8_t DLC);
uint8_t CAN_Check_Error(void);
void CAN_BusOff_Recover(void);
bool CAN_Send_CAN_STD_Message(uint32_t canid, uint8_t* data, uint8_t len);
void CAN_Get_CANMsg_From_RB_To_TXMailBox(void);
void CAN_Get_CANMsg_From_RB_To_TXMailBox_IT(void);
void CAN_Data_Process(void);


#ifdef __cplusplus
}
#endif

#endif /* __MYCANDRIVE_REG_H */

