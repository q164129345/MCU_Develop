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
 * @brief CAN����������״̬�Ĵ����ṹ�� (CAN Error Status Register)
 * 
 * ��������CAN�������Ĵ���״̬��Ϣ��ͨ����ӦӲ���Ĵ�����ӳ�䡣
 */
typedef struct {
    volatile uint8_t rec;    ///< Receive Error Counter (���մ��������), ��Χ: 0-255
    volatile uint8_t tec;    ///< Transmit Error Counter (���ʹ��������), ��Χ: 0-255
    volatile uint8_t lec;    ///< Last Error Code (���һ�δ������), ����ֵ:
                    ///< 0 = No error
                    ///< 1 = Stuff error
                    ///< 2 = Form error
                    ///< 3 = Acknowledgment error
                    ///< 4 = Bit recessive error
                    ///< 5 = Bit dominant error
                    ///< 6 = CRC error
                    ///< 7 = Custom (�ɳ��̶���)

    volatile uint8_t boff;   ///< Bus-Off Status (���߹ر�״̬), ����ֵ:
                    ///< 0 = Node is in active state (�ڵ㴦�ڻ״̬)
                    ///< 1 = Node is in bus-off state (�ڵ��������౻���߹ر�)

    volatile uint8_t epvf;   ///< Error Passive or Warning Flag (���󱻶�/�����־), ����ֵ:
                    ///< 0 = Error active (��������״̬)
                    ///< 1 = Error passive or warning (���󱻶��򾯸�״̬)

    volatile uint8_t ewgf;   ///< Error Warning Flag (���󾯸��־), ����ֵ:
                    ///< 0 = Error count below warning threshold (����������ھ�����ֵ)
                    ///< 1 = Error count reached warning threshold (��������ﵽ������ֵ)
} CAN_ESR_t;


void CAN_Config(void);
uint8_t CAN_SendMessage_Blocking(uint32_t stdId, uint8_t *data, uint8_t DLC);
uint8_t CAN_SendMessage_NonBlocking(uint32_t stdId, uint8_t *data, uint8_t DLC);
uint8_t CAN_Check_Error(CAN_ESR_t* can_esr);
void CAN_BusOff_Recover(void);

#ifdef __cplusplus
}
#endif

#endif /* __MYCANDRIVE_REG_H */

