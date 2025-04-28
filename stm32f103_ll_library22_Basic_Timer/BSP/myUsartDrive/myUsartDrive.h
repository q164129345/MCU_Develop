/**
 * @file myUsartDrive.h
 * @brief Register definitions for USART driver module
 * 
 * This header file contains register definitions and related structures
 * for the USART (Universal Synchronous/Asynchronous Receiver Transmitter) 
 * driver module. It provides the hardware abstraction layer for USART 
 * controller registers.
 * 
 * @note This file is intended for low-level register access and control of USART peripherals.
 * 
 * @version 1.0.0
 * @date 2025-03-31
 * @author Wallace.zhang
 * 
 * @copyright
 * 
 * @license SPDX-License-Identifier: MIT
 */
#ifndef __MYUSARTDRIVE_H
#define __MYUSARTDRIVE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

#define RX_BUFFER_SIZE 1024U
#define TX_BUFFER_SIZE 1024U



void USART1_SendString_DMA(const char *data, uint16_t len);
void USART1_Reinit(void);

void USART1_TX_DMA1_Channel4_Handler(void);
void USART1_RX_DMA1_Channel5_Handler(void);
void USART1_RX_Interrupt_Handler(void);
void USART1_Module_Run(void);
void USART1_Config(void);





#ifdef __cplusplus
}
#endif

#endif /* __MYUSARTDRIVE_H */
