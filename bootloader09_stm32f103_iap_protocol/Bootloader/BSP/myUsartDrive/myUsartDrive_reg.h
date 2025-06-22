/**
 * @file myUsartDrive_reg.h
 * @brief Register-level access and basic driver interfaces for USART1 module
 * 
 * This header file contains low-level register access definitions, basic configuration 
 * parameters, and function prototypes for USART1 driver, hardware abstraction layer for USART1 controller registers.
 * 
 * @note This file provides register-level access and minimal driver functions 
 *       for USART1 peripheral control.
 * 
 * @version 1.0.0
 * @date 2025-04-29
 * @author Wallace.zhang
 * 
 * @copyright
 * (C) 2025 Wallace.zhang. All rights reserved.
 * 
 * @license SPDX-License-Identifier: MIT
 */
#ifndef __MYUSARTDRIVE_REG_H
#define __MYUSARTDRIVE_REG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/** 
 * @brief USART RX buffer size definition (unit: bytes)
 */
#define RX_BUFFER_SIZE 1024U

/** 
 * @brief USART TX buffer size definition (unit: bytes)
 */
#define TX_BUFFER_SIZE 2048U

/**
  * @brief  阻塞方式发送以 NUL 结尾的字符串
  */
void USART1_SendString_Blocking(const char* str);

/**
 * @brief Send a string over USART1 using DMA
 * @param data Pointer to the data to send
 * @param len  Number of bytes to send
 */
void USART1_SendString_DMA(const uint8_t *data, uint16_t len);

/**
  * @brief   将数据写入 USART1 发送 ringbuffer 中
  */
uint8_t USART1_Put_TxData_To_Ringbuffer(const void* data, uint16_t len);

/**
 * @brief Reinitialize USART1 RX DMA reception
 */
void USART1_Reinit(void);

/**
 * @brief DMA1 Channel4 interrupt handler for USART1 TX
 */
//void USART1_TX_DMA1_Channel4_Interrupt_Handler(void);

/**
 * @brief DMA1 Channel5 interrupt handler for USART1 RX
 */
//void USART1_RX_DMA1_Channel5_Interrupt_Handler(void);

/**
 * @brief USART1 global interrupt handler (RX idle detection, error handling)
 */
//void USART1_RX_Interrupt_Handler(void);

/**
 * @brief USART1 module main task (should be called in main loop)
 */
void USART1_Module_Run(void);

/**
 * @brief Configure USART1 and related DMA channels
 */
void USART1_Configure(void);

#ifdef __cplusplus
}
#endif

#endif /* __MYUSARTDRIVE_REG_H */
