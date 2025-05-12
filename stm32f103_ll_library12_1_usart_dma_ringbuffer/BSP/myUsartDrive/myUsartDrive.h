/**
 * @file myUsartDrive.h
 * @brief Low-layer driver interface definitions for USART1 module
 * 
 * This header file contains function declarations, buffer definitions, and 
 * low-level control interfaces for USART1 using STM32 LL (Low-Layer) library.
 * 
 * @note This file is intended for low-level register access and control of USART1 
 *       peripherals, based on the STM32 LL library.
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
#ifndef __MYUSARTDRIVE_H
#define __MYUSARTDRIVE_H

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
#define TX_BUFFER_SIZE 1024U

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
 * @brief Reinitialize USART1 receiving DMA
 */
void USART1_Reinit(void);

/**
 * @brief DMA1_Channel4 interrupt handler for USART1 TX
 */
void USART1_TX_DMA1_Channel4_Interrupt_Handler(void);

/**
 * @brief DMA1_Channel5 interrupt handler for USART1 RX
 */
void USART1_RX_DMA1_Channel5_Interrupt_Handler(void);

/**
 * @brief USART1 global interrupt handler (RX idle detection, error handling)
 */
void USART1_RX_Interrupt_Handler(void);

/**
 * @brief USART1 main module run function (should be called in main loop)
 */
void USART1_Module_Run(void);

/**
 * @brief Configure USART1 peripheral and DMA channels
 */
void USART1_Config(void);

#ifdef __cplusplus
}
#endif

#endif /* __MYUSARTDRIVE_H */