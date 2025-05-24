/**
 * @file    bsp_usart_hal.c
 * @brief   STM32F1系列 USART + DMA + RingBuffer HAL库底层驱动实现（多实例、可变缓冲区）
 * @author  Wallace.zhang
 * @version 2.0.0
 * @date    2025-05-23
 */

#include "bsp_usart_hal.h"

/**
  * @brief   阻塞方式发送以NUL结尾字符串（调试用，非DMA）
  * @param   usart  指向USART驱动结构体的指针
  * @param   str    指向以'\0'结尾的字符串
  * @note    通过HAL库API逐字节发送，底层会轮询TXE位（USART_SR.TXE）。
  * @retval  无
  */
void USART_SendString_Blocking(USART_Driver_t* usart, const char* str)
{
    if (!usart || !str) return;
    HAL_UART_Transmit(usart->huart, (uint8_t*)str, strlen(str), 1000);
}

/**
  * @brief  配置并启动USART的DMA接收（环形模式）
  * @param  usart 指向USART驱动结构体的指针
  * @note   必须保证huart、hdma_rx已通过CubeMX正确初始化
  *         - 调用本函数会停止原有DMA，然后重新配置DMA并启动环形接收
  *         - 使能USART的IDLE中断，实现突发/不定长帧高效处理
  * @retval 无
  */
static void USART_Received_DMA_Configure(USART_Driver_t *usart)
{
    if (!usart) return;
    HAL_DMA_Abort(usart->hdma_rx); //! 先关闭
    HAL_UART_Receive_DMA(usart->huart, usart->rxDMABuffer, usart->rxBufSize); //! 启动环形DMA
    __HAL_UART_ENABLE_IT(usart->huart, UART_IT_IDLE); //! 使能IDLE中断
    usart->dmaRxLastPos = 0;
}

/**
  * @brief  启动DMA方式串口发送
  * @param  usart 指向USART驱动结构体的指针
  * @param  data  指向待发送的数据缓冲区
  * @param  len   待发送的数据字节数
  * @note   发送前需确保txDMABusy为0，否则应等待前一帧发送完成
  *         启动后DMA自动填充USART_DR寄存器，实现高效异步发送
  * @retval 无
  */
static void USART_SendString_DMA(USART_Driver_t *usart, uint8_t *data, uint16_t len)
{
    if (!usart || !data || len == 0 || len > usart->txBufSize) return;
    while (usart->txDMABusy); // 等待DMA空闲
    usart->txDMABusy = 1;
    HAL_UART_Transmit_DMA(usart->huart, data, len);
}

/**
 * @brief  写入数据到接收RingBuffer
 * @param  usart 指向USART驱动结构体的指针
 * @param  data  指向要写入的数据缓冲区
 * @param  len   要写入的数据长度（单位：字节）
 * @retval 0  数据成功写入，无数据丢弃
 * @retval 1  ringbuffer剩余空间不足，丢弃部分旧数据以容纳新数据
 * @retval 2  数据长度超过ringbuffer总容量，仅保留新数据尾部（全部旧数据被清空）
 * @retval 3  输入数据指针为空
 * @note
 * - 本函数通过lwrb库操作ringbuffer。
 * - 当len > ringbuffer容量时，强行截断，仅保留最新usart->rxRBBuffer字节。
 * - 若空间不足，自动调用lwrb_skip()丢弃部分旧数据。
 */
static uint8_t Put_Data_Into_Ringbuffer(USART_Driver_t *usart, const void *data, uint16_t len)
{
    //! 检查输入指针是否合法
    if (!usart || !data) return 3;
    
    lwrb_t *rb = &usart->rxRB;
    uint16_t rb_size = usart->rxBufSize;
    //! 获取当前RingBuffer剩余空间
    lwrb_sz_t free_space = lwrb_get_free(rb);
    
    //! 分三种情况处理：长度小于、等于、大于RingBuffer容量
    uint8_t ret = 0;
    if (len < rb_size) {
        //! 数据小于RingBuffer容量
        if (len <= free_space) {
            //! 空间充足，直接写入
            lwrb_write(rb, data, len);
        } else {
            //! 空间不足，需丢弃部分旧数据
            lwrb_sz_t used = lwrb_get_full(rb);
            lwrb_sz_t skip_len = len - free_space;
            if (skip_len > used) {
                skip_len = used;
            }
            lwrb_skip(rb, skip_len); //! 跳过（丢弃）旧数据
            lwrb_write(rb, data, len);
            ret = 1;
        }
    } else if (len == rb_size) { //! 数据刚好等于RingBuffer容量
        if (free_space < rb_size) {
            lwrb_reset(rb); //! 空间不足，重置RingBuffer
            ret = 1;
        }
        lwrb_write(rb, data, len);
    } else { //! 数据超过RingBuffer容量，仅保留最后rb_size字节
        const uint8_t *byte_ptr = (const uint8_t *)data;
        data = (const void *)(byte_ptr + (len - rb_size));
        lwrb_reset(rb);
        lwrb_write(rb, data, rb_size);
        ret = 2;
    }
    return ret;
}

/**
 * @brief  从DMA环形缓冲区搬运新收到的数据到RingBuffer（支持环绕）
 * @param  usart 指向USART驱动结构体的指针
 * @note   支持IDLE、DMA HT/TC等多中断共同调用
 *         - 本函数计算DMA环形缓冲区的新数据，并搬运到RingBuffer
 *         - 支持一次性或分段搬运（缓冲区环绕时自动分两段处理）
 * @retval 无
 */
static void USART_DMA_RX_Copy(USART_Driver_t *usart)
{
    uint16_t bufsize  = usart->rxBufSize;
    uint16_t curr_pos = bufsize - __HAL_DMA_GET_COUNTER(usart->hdma_rx);
    uint16_t last_pos = usart->dmaRxLastPos;

    if (curr_pos != last_pos) {
        if (curr_pos > last_pos) {
        //! 普通情况，未环绕
            Put_Data_Into_Ringbuffer(usart, usart->rxDMABuffer + last_pos, curr_pos - last_pos);
            usart->rxMsgCount += (curr_pos - last_pos);
        } else {
        //! 环绕，分两段处理
            Put_Data_Into_Ringbuffer(usart, usart->rxDMABuffer + last_pos, bufsize - last_pos);
            Put_Data_Into_Ringbuffer(usart, usart->rxDMABuffer, curr_pos);
            usart->rxMsgCount += (bufsize - last_pos) + curr_pos;
        }
        usart->dmaRxLastPos = curr_pos;
    }
}

/**
  * @brief  DMA接收搬运环形Buffer（推荐在IDLE、DMA中断等调用）
  * @param  usart 指向USART驱动结构体的指针
  * @retval 无
  */
void USART_DMA_RX_Interrupt_Handler(USART_Driver_t *usart)
{
    if (!usart) return;
    USART_DMA_RX_Copy(usart);
}

/**
  * @brief  串口IDLE中断处理（需在USARTx_IRQHandler中调用）
  * @param  usart 指向USART驱动结构体的指针
  * @note   检查并清除IDLE标志，及时触发DMA搬运
  * @retval 无
  */
void USART_RX_IDLE_Interrupt_Handler(USART_Driver_t *usart)
{
    if (!usart) return;
    if (__HAL_UART_GET_FLAG(usart->huart, UART_FLAG_IDLE)) {
        __HAL_UART_CLEAR_IDLEFLAG(usart->huart);
        USART_DMA_RX_Copy(usart);
    }
}

/**
  * @brief  DMA发送完成回调（由用户在HAL库TxCpltCallback中调用）
  * @param  usart 指向USART驱动结构体的指针
  * @note   一定要调用，否则无法再次DMA发送
  * @retval 无
  */
void USART_DMA_TX_Interrupt_Handler(USART_Driver_t *usart)
{
    if (!usart) return;
    usart->txDMABusy = 0;
}

/**
  * @brief  将数据写入指定USART驱动的发送 RingBuffer 中
  * @param  usart  指向USART驱动结构体的指针
  * @param  data   指向要写入的数据缓冲区
  * @param  len    要写入的数据长度（字节）
  * @retval  0  数据成功写入，无数据丢弃
  * @retval  1  ringbuffer 空间不足，丢弃部分旧数据以容纳新数据
  * @retval  2  数据长度超过 ringbuffer 总容量，仅保留最新 TX_BUFFER_SIZE 字节
  * @retval  3  输入数据指针为空
  * @note
  * - 使用 lwrb 库操作发送 RingBuffer（usart->txRB）。
  * - 若 len > ringbuffer 容量，会自动截断，仅保留最新的数据。
  * - 若空间不足，将调用 lwrb_skip() 丢弃部分旧数据。
  */
uint8_t USART_Put_TxData_To_Ringbuffer(USART_Driver_t *usart, const void* data, uint16_t len)
{
    if (!usart || !data) return 3; //! 检查输入数据指针有效性
    
    lwrb_t *rb = &usart->txRB;
    uint16_t capacity = usart->txBufSize;
    lwrb_sz_t freeSpace = lwrb_get_free(rb);
    uint8_t ret = 0;
    
    //! 情况1：数据长度小于ringbuffer容量
    if (len < capacity) {
        if (len <= freeSpace) {
            lwrb_write(rb, data, len); //! 剩余空间充足，直接写入
        } else {
            //! 空间不足，需丢弃部分旧数据
            lwrb_sz_t used = lwrb_get_full(rb);
            lwrb_sz_t skip_len = len - freeSpace;
            if (skip_len > used) skip_len = used;
            lwrb_skip(rb, skip_len);
            lwrb_write(rb, data, len);
            ret = 1;
        }
    } else if (len == capacity) { //! 情况2：数据长度等于ringbuffer容量
        if (freeSpace < capacity) { //! 如果ringbuffer已有数据
            lwrb_reset(rb);
            ret = 1;
        }
        lwrb_write(rb, data, len);
    } else { //! 情况3：数据长度大于ringbuffer容量，仅保留最后 capacity 字节
        const uint8_t *ptr = (const uint8_t*)data + (len - capacity);
        lwrb_reset(rb);
        lwrb_write(rb, ptr, capacity);
        ret = 2;
    }
    return ret;
}

/**
 * @brief   初始化USART驱动，配置DMA、RingBuffer与中断
 * @param   usart         指向USART驱动结构体的指针
 * @param   rxDMABuffer   DMA接收缓冲区指针
 * @param   rxRBBuffer    接收RingBuffer缓冲区指针
 * @param   rxBufSize     接收缓冲区大小
 * @param   txDMABuffer   DMA发送缓冲区指针
 * @param   txRBBuffer    发送RingBuffer缓冲区指针
 * @param   txBufSize     发送缓冲区大小
 * @retval  无
 * @note    需先通过CubeMX完成串口、DMA相关硬件配置和句柄赋值
 */
void USART_Config(USART_Driver_t *usart,
                  uint8_t *rxDMABuffer, uint8_t *rxRBBuffer, uint16_t rxBufSize,
                  uint8_t *txDMABuffer, uint8_t *txRBBuffer, uint16_t txBufSize)
{
    if (!usart) return;
    usart->rxDMABuffer = rxDMABuffer;
    usart->rxRBBuffer  = rxRBBuffer;
    usart->rxBufSize   = rxBufSize;
    lwrb_init(&usart->rxRB, usart->rxRBBuffer, usart->rxBufSize);

    usart->txDMABuffer = txDMABuffer;
    usart->txRBBuffer  = txRBBuffer;
    usart->txBufSize   = txBufSize;
    lwrb_init(&usart->txRB, usart->txRBBuffer, usart->txBufSize);

    USART_Received_DMA_Configure(usart); // 初始化DMA RX

    usart->txDMABusy = 0;
    usart->dmaRxLastPos = 0;
    usart->rxMsgCount = 0;
    usart->txMsgCount = 0;
    usart->errorDMATX = 0;
    usart->errorDMARX = 0;
    usart->errorRX = 0;
}

/**
  * @brief  USART模块主循环调度函数（DMA + RingBuffer高效收发）
  * @param  usart 指向USART驱动结构体的指针
  * @note   建议主循环定时（如1ms）调用
  *         - 检查发送RingBuffer是否有待发送数据，且DMA当前空闲
  *         - 若条件满足，从发送RingBuffer读取一段数据到DMA发送缓冲区，并通过DMA启动异步发送
  *         - 自动维护已发送数据统计
  * @retval 无
  */
void USART_Module_Run(USART_Driver_t *usart)
{
    if (!usart) return;
    uint16_t available = lwrb_get_full(&usart->txRB);
    if (available && usart->txDMABusy == 0) {
        uint16_t len = (available > usart->txBufSize) ? usart->txBufSize : available;
        lwrb_read(&usart->txRB, usart->txDMABuffer, len);
        usart->txMsgCount += len;
        USART_SendString_DMA(usart, usart->txDMABuffer, len);
    }
}

/**
  * @brief  获取USART接收RingBuffer中的可读字节数
  * @param  usart 指向USART驱动结构体的指针
  * @retval uint32_t 可读取的数据字节数
  * @note   通常在主循环或数据解析前调用，用于判断是否需要读取数据。
  */
uint32_t USART_Get_The_Existing_Amount_Of_Data(USART_Driver_t *usart)
{
    if (!usart) return 0;
    return lwrb_get_full(&usart->rxRB);
}

/**
  * @brief  从USART接收RingBuffer中读取一个字节数据
  * @param  usart 指向USART驱动结构体的指针
  * @param  data  指向存放读取结果的缓冲区指针
  * @retval 1  读取成功，有新数据存入 *data
  * @retval 0  读取失败（无数据或data为NULL）
  * @note   本函数不会阻塞，无数据时直接返回0。
  */
uint8_t USART_Take_A_Piece_Of_Data(USART_Driver_t *usart, uint8_t* data)
{
    if (!usart || !data) return 0;
    return lwrb_read(&usart->rxRB, data, 1);
}

/**
  * @brief  DMA传输错误后的自动恢复操作（含错误统计）
  * @param  usart 指向USART驱动结构体
  * @param  dir   方向：0=RX, 1=TX
  * @note   检测到DMA传输错误（TE）时调用，自动进行统计并恢复
  *         RX方向会自动重启DMA，TX方向建议等待主循环调度新发送
  * @retval 无
  */
void USART_DMA_Error_Recover(USART_Driver_t *usart, uint8_t dir)
{
    if (!usart) return;

    if (dir == 0) { //! RX方向
        usart->errorDMARX++; //! DMA接收错误计数 */
        HAL_DMA_Abort(usart->hdma_rx);
        HAL_UART_Receive_DMA(usart->huart, usart->rxDMABuffer, usart->rxBufSize);
        //! 可以加入极端情况下的USART复位等
    } else { //! TX方向
        usart->errorDMATX++; //! DMA发送错误计数 */
        HAL_DMA_Abort(usart->hdma_tx);
        //! 一般等待主循环触发新的DMA发送
    }
    //! 可插入报警、日志
}




