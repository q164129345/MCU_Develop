/**
 * @file    bsp_usart_drive.h
 * @brief   STM32F1系列 USART + DMA + RingBuffer LL底层驱动接口，支持多实例化
 * @author  Wallace.zhang
 * @version 1.0.0
 * @date    2025-01-10
 * 
 * @note    基于LL库开发，支持多个USART实例（USART1、USART2、USART3等）
 *          参考bsp_usart_hal的多实例化设计，但使用LL库实现更高的性能
 */
#ifndef __BSP_USART_DRIVE_H
#define __BSP_USART_DRIVE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "lwrb/lwrb.h"

/* ============================= DMA通道函数映射宏 ============================= */
/**
 * @brief  STM32F1 LL库DMA标志检查和清除函数映射宏
 * @note   STM32F1的LL库没有通用的LL_DMA_IsActiveFlag_TC()函数，
 *         而是按通道编号提供具体函数，如LL_DMA_IsActiveFlag_TC1()等
 */

/* DMA传输完成标志检查宏 */
#define LL_DMA_IsActiveFlag_TC(DMAx, Channel) \
    ((Channel == LL_DMA_CHANNEL_1) ? LL_DMA_IsActiveFlag_TC1(DMAx) : \
     (Channel == LL_DMA_CHANNEL_2) ? LL_DMA_IsActiveFlag_TC2(DMAx) : \
     (Channel == LL_DMA_CHANNEL_3) ? LL_DMA_IsActiveFlag_TC3(DMAx) : \
     (Channel == LL_DMA_CHANNEL_4) ? LL_DMA_IsActiveFlag_TC4(DMAx) : \
     (Channel == LL_DMA_CHANNEL_5) ? LL_DMA_IsActiveFlag_TC5(DMAx) : \
     (Channel == LL_DMA_CHANNEL_6) ? LL_DMA_IsActiveFlag_TC6(DMAx) : \
     (Channel == LL_DMA_CHANNEL_7) ? LL_DMA_IsActiveFlag_TC7(DMAx) : 0)

/* DMA半传输标志检查宏 */
#define LL_DMA_IsActiveFlag_HT(DMAx, Channel) \
    ((Channel == LL_DMA_CHANNEL_1) ? LL_DMA_IsActiveFlag_HT1(DMAx) : \
     (Channel == LL_DMA_CHANNEL_2) ? LL_DMA_IsActiveFlag_HT2(DMAx) : \
     (Channel == LL_DMA_CHANNEL_3) ? LL_DMA_IsActiveFlag_HT3(DMAx) : \
     (Channel == LL_DMA_CHANNEL_4) ? LL_DMA_IsActiveFlag_HT4(DMAx) : \
     (Channel == LL_DMA_CHANNEL_5) ? LL_DMA_IsActiveFlag_HT5(DMAx) : \
     (Channel == LL_DMA_CHANNEL_6) ? LL_DMA_IsActiveFlag_HT6(DMAx) : \
     (Channel == LL_DMA_CHANNEL_7) ? LL_DMA_IsActiveFlag_HT7(DMAx) : 0)

/* DMA传输错误标志检查宏 */
#define LL_DMA_IsActiveFlag_TE(DMAx, Channel) \
    ((Channel == LL_DMA_CHANNEL_1) ? LL_DMA_IsActiveFlag_TE1(DMAx) : \
     (Channel == LL_DMA_CHANNEL_2) ? LL_DMA_IsActiveFlag_TE2(DMAx) : \
     (Channel == LL_DMA_CHANNEL_3) ? LL_DMA_IsActiveFlag_TE3(DMAx) : \
     (Channel == LL_DMA_CHANNEL_4) ? LL_DMA_IsActiveFlag_TE4(DMAx) : \
     (Channel == LL_DMA_CHANNEL_5) ? LL_DMA_IsActiveFlag_TE5(DMAx) : \
     (Channel == LL_DMA_CHANNEL_6) ? LL_DMA_IsActiveFlag_TE6(DMAx) : \
     (Channel == LL_DMA_CHANNEL_7) ? LL_DMA_IsActiveFlag_TE7(DMAx) : 0)

/* DMA传输完成标志清除宏 */
#define LL_DMA_ClearFlag_TC(DMAx, Channel) \
    do { \
        if (Channel == LL_DMA_CHANNEL_1) LL_DMA_ClearFlag_TC1(DMAx); \
        else if (Channel == LL_DMA_CHANNEL_2) LL_DMA_ClearFlag_TC2(DMAx); \
        else if (Channel == LL_DMA_CHANNEL_3) LL_DMA_ClearFlag_TC3(DMAx); \
        else if (Channel == LL_DMA_CHANNEL_4) LL_DMA_ClearFlag_TC4(DMAx); \
        else if (Channel == LL_DMA_CHANNEL_5) LL_DMA_ClearFlag_TC5(DMAx); \
        else if (Channel == LL_DMA_CHANNEL_6) LL_DMA_ClearFlag_TC6(DMAx); \
        else if (Channel == LL_DMA_CHANNEL_7) LL_DMA_ClearFlag_TC7(DMAx); \
    } while(0)

/* DMA半传输标志清除宏 */
#define LL_DMA_ClearFlag_HT(DMAx, Channel) \
    do { \
        if (Channel == LL_DMA_CHANNEL_1) LL_DMA_ClearFlag_HT1(DMAx); \
        else if (Channel == LL_DMA_CHANNEL_2) LL_DMA_ClearFlag_HT2(DMAx); \
        else if (Channel == LL_DMA_CHANNEL_3) LL_DMA_ClearFlag_HT3(DMAx); \
        else if (Channel == LL_DMA_CHANNEL_4) LL_DMA_ClearFlag_HT4(DMAx); \
        else if (Channel == LL_DMA_CHANNEL_5) LL_DMA_ClearFlag_HT5(DMAx); \
        else if (Channel == LL_DMA_CHANNEL_6) LL_DMA_ClearFlag_HT6(DMAx); \
        else if (Channel == LL_DMA_CHANNEL_7) LL_DMA_ClearFlag_HT7(DMAx); \
    } while(0)

/* DMA传输错误标志清除宏 */
#define LL_DMA_ClearFlag_TE(DMAx, Channel) \
    do { \
        if (Channel == LL_DMA_CHANNEL_1) LL_DMA_ClearFlag_TE1(DMAx); \
        else if (Channel == LL_DMA_CHANNEL_2) LL_DMA_ClearFlag_TE2(DMAx); \
        else if (Channel == LL_DMA_CHANNEL_3) LL_DMA_ClearFlag_TE3(DMAx); \
        else if (Channel == LL_DMA_CHANNEL_4) LL_DMA_ClearFlag_TE4(DMAx); \
        else if (Channel == LL_DMA_CHANNEL_5) LL_DMA_ClearFlag_TE5(DMAx); \
        else if (Channel == LL_DMA_CHANNEL_6) LL_DMA_ClearFlag_TE6(DMAx); \
        else if (Channel == LL_DMA_CHANNEL_7) LL_DMA_ClearFlag_TE7(DMAx); \
    } while(0)

/**
 * @brief USART驱动实例结构体（基于LL库 + DMA + RingBuffer + 统计）
 * 
 * 这个结构体就像一个"工具箱"，每个USART都有自己的一套工具：
 * - 就像每个邮递员都有自己的邮包（缓冲区）
 * - 自己的计数器（统计信息）
 * - 自己的工作状态（忙碌标志）
 */
typedef struct
{
    /* USART硬件实例 */
    USART_TypeDef       *USARTx;            /**< USART寄存器基地址（如USART1、USART2等） */
    DMA_TypeDef         *DMAx;              /**< DMA控制器基地址（如DMA1、DMA2等） */
    uint32_t            dmaTxChannel;       /**< DMA发送通道（如LL_DMA_CHANNEL_4） */
    uint32_t            dmaRxChannel;       /**< DMA接收通道（如LL_DMA_CHANNEL_5） */
    
    /* 状态和统计信息 */
    volatile uint8_t    txDMABusy;          /**< DMA发送忙碌标志：1=正在发送，0=空闲 */
    volatile uint64_t   rxMsgCount;         /**< 统计接收字节总数 */
    volatile uint64_t   txMsgCount;         /**< 统计发送字节总数 */
    volatile uint16_t   dmaRxLastPos;       /**< DMA接收缓冲区上次读取的位置 */
    
    /* 错误统计 */
    volatile uint32_t   errorDMATX;         /**< DMA发送错误统计 */
    volatile uint32_t   errorDMARX;         /**< DMA接收错误统计 */
    volatile uint32_t   errorRX;            /**< 串口接收错误统计 */

    /* RX相关缓冲区 */
    uint8_t             *rxDMABuffer;       /**< DMA接收缓冲区数组 */
    uint8_t             *rxRBBuffer;        /**< 接收RingBuffer缓冲区数组 */
    uint16_t            rxBufSize;          /**< DMA缓冲区/RX Ringbuffer缓冲区大小 */
    lwrb_t              rxRB;               /**< 接收RingBuffer句柄 */

    /* TX相关缓冲区 */
    uint8_t             *txDMABuffer;       /**< DMA发送缓冲区数组 */
    uint8_t             *txRBBuffer;        /**< 发送RingBuffer缓冲区数组 */
    uint16_t            txBufSize;          /**< DMA缓冲区/TX Ringbuffer缓冲区大小 */
    lwrb_t              txRB;               /**< 发送RingBuffer句柄 */
    
} USART_LL_Driver_t;

/* ================================ 核心API ================================ */

/**
 * @brief  阻塞方式发送以 NUL 结尾的字符串（调试用，不走DMA）
 * @param  usart  指向USART驱动结构体的指针
 * @param  str    指向以'\0'结尾的字符串
 * @note   通过LL库API逐字节发送，底层轮询TXE标志位（USART_SR.TXE）。
 * @retval 无
 */
void USART_LL_SendString_Blocking(USART_LL_Driver_t* usart, const char* str);

/**
 * @brief  用户数据写入指定USART实例的发送 RingBuffer 中
 * @param  usart  指向USART驱动结构体的指针
 * @param  data   指向要写入的数据缓冲区
 * @param  len    要写入的数据长度（字节）
 * @retval  0  数据成功写入，不丢数据
 * @retval  1  ringbuffer 空间不足，丢弃了旧数据后成功写入
 * @retval  2  数据长度超过 ringbuffer 容量，截断后保留最新数据
 * @retval  3  输入参数指针为空
 * @note
 * - 使用 lwrb 库管理的 RingBuffer（usart->txRB）。
 * - 当 len > ringbuffer 容量时自动截断，仅保存最新的数据。
 * - 当空间不足，会调用 lwrb_skip() 丢弃旧数据。
 */
uint8_t USART_LL_Put_TxData_To_Ringbuffer(USART_LL_Driver_t *usart, const void* data, uint16_t len);

/**
 * @brief  USART模块定时运行函数（数据处理主循环，1ms内调用）
 * @param  usart 指向USART驱动结构体的指针
 * @note   在主循环中定时调用（1ms间隔推荐）
 *         - 检查发送RingBuffer是否有待发送数据，通过DMA异步发送
 *         - 自动维护已发送数据统计
 * @retval 无
 */
void USART_LL_Module_Run(USART_LL_Driver_t *usart);

/**
 * @brief  获取USART接收RingBuffer中的可读字节数
 * @param  usart 指向USART驱动结构体的指针
 * @retval uint32_t 可读取的缓冲字节数
 * @note   通过主循环调用，在数据处理前先判断是否需要读取数据。
 */
uint32_t USART_LL_Get_Available_RxData_Length(USART_LL_Driver_t *usart);

/**
 * @brief  从USART接收RingBuffer中读取一个字节数据
 * @param  usart 指向USART驱动结构体的指针
 * @param  data  指向存放读取数据的缓冲区指针
 * @retval 1  读取成功，数据存放在 *data
 * @retval 0  读取失败（无数据或data为NULL）
 * @note   如果缓冲区为空，则直接返回0。
 */
uint8_t USART_LL_Read_A_Byte_Data(USART_LL_Driver_t *usart, uint8_t* data);

/* ========================= 中断处理函数 ========================= */

/**
 * @brief  USART发送DMA中断处理函数
 * @param  usart 指向USART驱动结构体的指针
 * @note   在对应的DMA中断服务程序中调用
 */
void USART_LL_DMA_TX_Interrupt_Handler(USART_LL_Driver_t *usart);

/**
 * @brief  USART接收DMA中断处理函数
 * @param  usart 指向USART驱动结构体的指针
 * @note   在对应的DMA中断服务程序中调用，处理HT/TC中断
 */
void USART_LL_DMA_RX_Interrupt_Handler(USART_LL_Driver_t *usart);

/**
 * @brief  USART全局中断处理函数（支持DMA+RingBuffer）
 * @param  usart 指向USART驱动结构体的指针
 * @note   在USARTx_IRQHandler中调用，处理IDLE中断和错误中断
 */
void USART_LL_RX_Interrupt_Handler(USART_LL_Driver_t *usart);

/* ========================== 初始化和配置 ========================== */

/**
 * @brief  初始化USART驱动实例（包括DMA、RingBuffer、中断）
 * @param  usart         指向USART驱动结构体的指针
 * @param  USARTx        USART寄存器基地址（如USART1、USART2等）
 * @param  DMAx          DMA控制器基地址（如DMA1、DMA2等）
 * @param  dmaTxChannel  DMA发送通道（如LL_DMA_CHANNEL_4）
 * @param  dmaRxChannel  DMA接收通道（如LL_DMA_CHANNEL_5）
 * @param  rxDMABuffer   DMA接收缓冲区指针
 * @param  rxRBBuffer    接收RingBuffer缓冲区指针
 * @param  rxBufSize     接收缓冲区大小
 * @param  txDMABuffer   DMA发送缓冲区指针
 * @param  txRBBuffer    发送RingBuffer缓冲区指针
 * @param  txBufSize     发送缓冲区大小
 * @retval 无
 * @note   必须通过CubeMX完成串口、DMA硬件配置后调用此函数
 */
void USART_LL_Config(USART_LL_Driver_t *usart,
                     USART_TypeDef *USARTx, DMA_TypeDef *DMAx,
                     uint32_t dmaTxChannel, uint32_t dmaRxChannel,
                     uint8_t *rxDMABuffer, uint8_t *rxRBBuffer, uint16_t rxBufSize,
                     uint8_t *txDMABuffer, uint8_t *txRBBuffer, uint16_t txBufSize);

/**
 * @brief  DMA错误恢复处理（自动重新初始化并更新统计）
 * @param  usart 指向USART驱动结构体
 * @param  dir   方向：0=RX, 1=TX
 * @note   检测到DMA传输错误（TE）时调用，自动清除统计并恢复
 *         RX错误：自动重启DMA；TX错误：等待主循环重新发送
 * @retval 无
 */
void USART_LL_DMA_Error_Recover(USART_LL_Driver_t *usart, uint8_t dir);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_USART_DRIVE_H */
