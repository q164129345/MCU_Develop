/**
 * @file    bsp_usart_drive.c
 * @brief   STM32F1系列 USART + DMA + RingBuffer LL底层驱动实现，支持多实例化
 * @author  Wallace.zhang
 * @version 1.0.0
 * @date    2025-01-10
 */

#include "bsp_usart_drive/bsp_usart_drive.h"

/* ============================= 私有函数声明 ============================= */

/**
 * @brief  配置DMA接收通道（循环模式）
 * @param  usart 指向USART驱动结构体的指针
 * @note   就像设置一个"自动传送带"，持续将串口数据搬运到内存
 * @retval 无
 */
static void USART_LL_DMA_RX_Configure(USART_LL_Driver_t *usart);

/**
 * @brief  使用DMA发送字符串（非阻塞方式）
 * @param  usart 指向USART驱动结构体的指针
 * @param  data  指向发送数据的缓冲区
 * @param  len   发送数据长度
 * @note   就像启动一个"快递员"，自动把数据送到目的地
 * @retval 无
 */
static void USART_LL_SendString_DMA(USART_LL_Driver_t *usart, const uint8_t *data, uint16_t len);

/**
 * @brief  将DMA接收到的数据复制到RingBuffer中（支持环形）
 * @param  usart 指向USART驱动结构体的指针
 * @note   就像将货物从"临时仓库"搬到"长期储存仓库"
 * @retval 无
 */
static void USART_LL_DMA_RX_Copy(USART_LL_Driver_t *usart);

/**
 * @brief  将数据写入接收RingBuffer
 * @param  usart 指向USART驱动结构体的指针
 * @param  data  指向要写入的数据缓冲区
 * @param  len   要写入的数据长度
 * @retval 返回写入状态（同USART_LL_Put_TxData_To_Ringbuffer）
 * @note   智能缓冲区管理：满了就挤掉老数据，腾出空间给新数据
 */
static uint8_t USART_LL_Put_RxData_Into_Ringbuffer(USART_LL_Driver_t *usart, const void* data, uint16_t len);

/**
 * @brief  处理DMA发送通道错误
 * @param  usart 指向USART驱动结构体的指针
 * @retval 1 检测到并处理了错误，0 无错误
 * @note   就像"故障检测器"，发现问题立即修复
 */
static uint8_t USART_LL_DMA_TX_Error_Handler(USART_LL_Driver_t *usart);

/**
 * @brief  处理DMA接收通道错误
 * @param  usart 指向USART驱动结构体的指针
 * @retval 1 检测到并处理了错误，0 无错误
 * @note   就像"故障检测器"，发现问题立即修复
 */
static uint8_t USART_LL_DMA_RX_Error_Handler(USART_LL_Driver_t *usart);

/**
 * @brief  处理USART硬件错误（ORE、NE、FE、PE等）
 * @param  usart 指向USART驱动结构体的指针
 * @retval 1 检测到并处理了错误，0 无错误
 * @note   清除各种串口硬件错误标志
 */
static uint8_t USART_LL_Hardware_Error_Handler(USART_LL_Driver_t *usart);

/**
 * @brief  获取TX DMA忙碌状态
 * @param  usart 指向USART驱动结构体的指针
 * @retval 0 空闲，1 忙碌
 * @note   检查"快递员"是否还在工作
 */
static uint8_t USART_LL_Get_TX_DMA_Busy(USART_LL_Driver_t *usart);

/* ============================= 公有函数实现 ============================= */

/**
 * @brief  阻塞方式发送以 NUL 结尾的字符串
 * @param  usart  指向USART驱动结构体的指针
 * @param  str    指向以'\0'结尾的字符串
 * @note   逐字节轮询发送，就像用"老式打字机"一个字一个字地敲
 */
void USART_LL_SendString_Blocking(USART_LL_Driver_t* usart, const char* str)
{
    if (!usart || !str) return;
    
    while (*str) {
        while (!LL_USART_IsActiveFlag_TXE(usart->USARTx)); // 等待发送寄存器空
        LL_USART_TransmitData8(usart->USARTx, *str++);     // 发送一个字节
    }
}

/**
 * @brief  用户数据写入发送 RingBuffer
 * @param  usart  指向USART驱动结构体的指针
 * @param  data   指向要写入的数据缓冲区
 * @param  len    要写入的数据长度（字节）
 * @retval 0-3   写入状态码
 * @note   智能缓冲区：就像一个"智能邮箱"，满了会自动清理旧邮件
 */
uint8_t USART_LL_Put_TxData_To_Ringbuffer(USART_LL_Driver_t *usart, const void* data, uint16_t len)
{
    if (!usart || !data) return 3; // 输入参数无效
    
    lwrb_t *rb = &usart->txRB;
    uint16_t capacity = usart->txBufSize;
    lwrb_sz_t freeSpace = lwrb_get_free(rb);
    uint8_t ret = 0;
    
    // 情况1：数据长度小于ringbuffer容量
    if (len < capacity) {
        if (len <= freeSpace) {
            lwrb_write(rb, data, len); // 剩余空间够，直接写入
        } else {
            // 空间不足，挤掉一些旧数据
            lwrb_sz_t used = lwrb_get_full(rb);
            lwrb_sz_t skip_len = len - freeSpace;
            if (skip_len > used) skip_len = used;
            lwrb_skip(rb, skip_len);
            lwrb_write(rb, data, len);
            ret = 1;
        }
    } else if (len == capacity) { // 情况2：数据长度等于ringbuffer容量
        if (freeSpace < capacity) { // 清空ringbuffer
            lwrb_reset(rb);
            ret = 1;
        }
        lwrb_write(rb, data, len);
    } else { // 情况3：数据长度大于ringbuffer容量，截断保留 capacity 字节
        const uint8_t *ptr = (const uint8_t*)data + (len - capacity);
        lwrb_reset(rb);
        lwrb_write(rb, ptr, capacity);
        ret = 2;
    }
    return ret;
}

/**
 * @brief  USART模块主运行函数
 * @param  usart 指向USART驱动结构体的指针
 * @note   就像一个"邮局管理员"，定期检查是否有邮件要发送
 */
void USART_LL_Module_Run(USART_LL_Driver_t *usart)
{
    if (!usart) return;
    
    /* 检查发送队列，如果有数据且DMA空闲，就启动发送 */
    uint16_t available = lwrb_get_full(&usart->txRB);
    if (available && USART_LL_Get_TX_DMA_Busy(usart) == 0) {
        uint16_t len = (available > usart->txBufSize) ? usart->txBufSize : available;
        lwrb_read(&usart->txRB, usart->txDMABuffer, len); // 从RingBuffer读取到DMA缓冲区
        usart->txMsgCount += len; // 统计发送数据
        USART_LL_SendString_DMA(usart, usart->txDMABuffer, len); // 启动DMA发送
    }
}

/**
 * @brief  获取接收RingBuffer中的可读字节数
 */
uint32_t USART_LL_Get_Available_RxData_Length(USART_LL_Driver_t *usart)
{
    if (!usart) return 0;
    return lwrb_get_full(&usart->rxRB);
}

/**
 * @brief  从接收RingBuffer中读取一个字节
 */
uint8_t USART_LL_Read_A_Byte_Data(USART_LL_Driver_t *usart, uint8_t* data)
{
    if (!usart || !data) return 0;
    return lwrb_read(&usart->rxRB, data, 1);
}

/* ========================= 中断处理函数 ========================= */

/**
 * @brief  USART发送DMA中断处理函数
 * @param  usart 指向USART驱动结构体的指针
 * @note   DMA发送完成后调用，就像"快递员完成投递后报告"
 */
void USART_LL_DMA_TX_Interrupt_Handler(USART_LL_Driver_t *usart)
{
    if (!usart) return;
    
    if (USART_LL_DMA_TX_Error_Handler(usart)) { // 处理错误
        // 错误已被处理
    } else if (LL_DMA_IsActiveFlag_TC(usart->DMAx, usart->dmaTxChannel)) { // 传输完成
        // 清除传输完成标志
        LL_DMA_ClearFlag_TC(usart->DMAx, usart->dmaTxChannel);
        // 关闭DMA通道，确保下次传输前已经完全停止
        LL_DMA_DisableChannel(usart->DMAx, usart->dmaTxChannel);
        // 关闭USART的DMAT位（关闭DMA发送请求）
        LL_USART_DisableDMAReq_TX(usart->USARTx);
        // 清除DMA忙碌状态
        usart->txDMABusy = 0;
    }
}

/**
 * @brief  USART接收DMA中断处理函数
 * @param  usart 指向USART驱动结构体的指针
 * @note   处理DMA接收的半传输和全传输中断
 */
void USART_LL_DMA_RX_Interrupt_Handler(USART_LL_Driver_t *usart)
{
    if (!usart) return;
    
    if (USART_LL_DMA_RX_Error_Handler(usart)) { // 处理错误失败
        // 错误已被处理
    } else if (LL_DMA_IsActiveFlag_HT(usart->DMAx, usart->dmaRxChannel)) { // 半传输中断
        // 清除半传输标志
        LL_DMA_ClearFlag_HT(usart->DMAx, usart->dmaRxChannel);
        USART_LL_DMA_RX_Copy(usart); // 将DMA接收数据放到ringbuffer
    } else if (LL_DMA_IsActiveFlag_TC(usart->DMAx, usart->dmaRxChannel)) { // 全传输中断
        // 清除传输完成标志
        LL_DMA_ClearFlag_TC(usart->DMAx, usart->dmaRxChannel);
        USART_LL_DMA_RX_Copy(usart); // 将DMA接收数据放到ringbuffer
    }
}

/**
 * @brief  USART全局中断处理函数
 * @param  usart 指向USART驱动结构体的指针
 * @note   处理IDLE中断和各种错误中断
 */
void USART_LL_RX_Interrupt_Handler(USART_LL_Driver_t *usart)
{
    if (!usart) return;
    
    if (USART_LL_Hardware_Error_Handler(usart)) { // 处理硬件错误
        usart->errorRX++; // 有错误，记录计数
    } else if (LL_USART_IsActiveFlag_IDLE(usart->USARTx)) {  // 检查 USART 是否空闲中断
        /* 清除IDLE标志：必须先读SR，再读DR */
        volatile uint32_t tmp = usart->USARTx->SR;
        tmp = usart->USARTx->DR;
        (void)tmp;
        USART_LL_DMA_RX_Copy(usart); // 将接收数据放到ringbuffer
    }
}

/* ========================== 初始化和配置 ========================== */

/**
 * @brief  初始化USART驱动实例
 * @param  usart         指向USART驱动结构体的指针
 * @param  USARTx        USART寄存器基地址
 * @param  DMAx          DMA控制器基地址
 * @param  dmaTxChannel  DMA发送通道
 * @param  dmaRxChannel  DMA接收通道
 * @param  rxDMABuffer   DMA接收缓冲区指针
 * @param  rxRBBuffer    接收RingBuffer缓冲区指针
 * @param  rxBufSize     接收缓冲区大小
 * @param  txDMABuffer   DMA发送缓冲区指针
 * @param  txRBBuffer    发送RingBuffer缓冲区指针
 * @param  txBufSize     发送缓冲区大小
 * @note   就像"搭建一个完整的邮政系统"，配置所有必要组件
 */
void USART_LL_Config(USART_LL_Driver_t *usart,
                     USART_TypeDef *USARTx, DMA_TypeDef *DMAx,
                     uint32_t dmaTxChannel, uint32_t dmaRxChannel,
                     uint8_t *rxDMABuffer, uint8_t *rxRBBuffer, uint16_t rxBufSize,
                     uint8_t *txDMABuffer, uint8_t *txRBBuffer, uint16_t txBufSize)
{
    if (!usart) return;
    
    /* 硬件实例配置 */
    usart->USARTx = USARTx;
    usart->DMAx = DMAx;
    usart->dmaTxChannel = dmaTxChannel;
    usart->dmaRxChannel = dmaRxChannel;
    
    /* RX缓冲区配置 */
    usart->rxDMABuffer = rxDMABuffer;
    usart->rxRBBuffer = rxRBBuffer;
    usart->rxBufSize = rxBufSize;
    lwrb_init(&usart->rxRB, usart->rxRBBuffer, usart->rxBufSize);

    /* TX缓冲区配置 */
    usart->txDMABuffer = txDMABuffer;
    usart->txRBBuffer = txRBBuffer;
    usart->txBufSize = txBufSize;
    lwrb_init(&usart->txRB, usart->txRBBuffer, usart->txBufSize);

    /* 状态初始化 */
    usart->txDMABusy = 0;
    usart->dmaRxLastPos = 0;
    usart->rxMsgCount = 0;
    usart->txMsgCount = 0;
    usart->errorDMATX = 0;
    usart->errorDMARX = 0;
    usart->errorRX = 0;
    
    /* 配置USART和DMA */
    LL_USART_EnableDMAReq_RX(usart->USARTx); // 使能USART_RX的DMA请求
    LL_USART_EnableIT_IDLE(usart->USARTx);   // 开启USART空闲中断
  
    LL_DMA_EnableIT_HT(usart->DMAx, usart->dmaRxChannel); // 使能DMA接收半传输中断
    LL_DMA_EnableIT_TC(usart->DMAx, usart->dmaRxChannel); // 使能DMA接收传输完成中断
  
    LL_DMA_EnableIT_TC(usart->DMAx, usart->dmaTxChannel); // 使能DMA发送传输完成中断
    
    USART_LL_DMA_RX_Configure(usart); // 配置DMA接收
}

/**
 * @brief  DMA错误恢复处理
 */
void USART_LL_DMA_Error_Recover(USART_LL_Driver_t *usart, uint8_t dir)
{
    if (!usart) return;

    if (dir == 0) { // RX错误
        usart->errorDMARX++; // DMA接收错误计数 */
        LL_DMA_DisableChannel(usart->DMAx, usart->dmaRxChannel);
        while(LL_DMA_IsEnabledChannel(usart->DMAx, usart->dmaRxChannel)); // 等待完全关闭
        // 重新配置并启动DMA接收
        USART_LL_DMA_RX_Configure(usart);
    } else { // TX错误
        usart->errorDMATX++; // DMA发送错误计数 */
        LL_DMA_DisableChannel(usart->DMAx, usart->dmaTxChannel);
        LL_USART_DisableDMAReq_TX(usart->USARTx);
        usart->txDMABusy = 0; // 清除忙碌标志
        // 一般等待主循环重新发送
    }
}

/* ============================= 私有函数实现 ============================= */

/**
 * @brief  配置DMA接收通道
 * @param  usart 指向USART驱动结构体的指针
 * @note   设置DMA循环模式，从串口持续接收数据到缓冲区
 */
static void USART_LL_DMA_RX_Configure(USART_LL_Driver_t *usart) 
{
    if (!usart) return;
    
    /* 配置DMA接收：从USART_DR到内存缓冲区 */
    LL_DMA_SetMemoryAddress(usart->DMAx, usart->dmaRxChannel, (uint32_t)usart->rxDMABuffer);
    LL_DMA_SetPeriphAddress(usart->DMAx, usart->dmaRxChannel, LL_USART_DMA_GetRegAddr(usart->USARTx));
    LL_DMA_SetDataLength(usart->DMAx, usart->dmaRxChannel, usart->rxBufSize);
    LL_DMA_EnableChannel(usart->DMAx, usart->dmaRxChannel);
}

/**
 * @brief  使用DMA发送字符串
 * @param  usart 指向USART驱动结构体的指针
 * @param  data  指向发送数据的缓冲区
 * @param  len   发送数据长度
 * @note   启动DMA非阻塞发送
 */
static void USART_LL_SendString_DMA(USART_LL_Driver_t *usart, const uint8_t *data, uint16_t len)
{
    if (!usart || !data || len == 0 || len > usart->txBufSize) return;
    
    // 等待上一个DMA传输完成
    while(usart->txDMABusy);
    usart->txDMABusy = 1; // 设置DMA正在发送

    // 如果DMA通道x正在使用，先关闭以便重新配置
    if (LL_DMA_IsEnabledChannel(usart->DMAx, usart->dmaTxChannel)) {
        LL_DMA_DisableChannel(usart->DMAx, usart->dmaTxChannel);
        while(LL_DMA_IsEnabledChannel(usart->DMAx, usart->dmaTxChannel));
    }
    
    // 配置DMA通道：从内存到USART_DR
    LL_DMA_SetMemoryAddress(usart->DMAx, usart->dmaTxChannel, (uint32_t)usart->txDMABuffer);
    LL_DMA_SetPeriphAddress(usart->DMAx, usart->dmaTxChannel, LL_USART_DMA_GetRegAddr(usart->USARTx));
    LL_DMA_SetDataLength(usart->DMAx, usart->dmaTxChannel, len);
    
    // 启用USART的DMA发送请求
    LL_USART_EnableDMAReq_TX(usart->USARTx); // 启用USART的DMA发送请求
    // 启用DMA通道，开始DMA传输
    LL_DMA_EnableChannel(usart->DMAx, usart->dmaTxChannel);
}

/**
 * @brief  将DMA接收位置数据复制到RingBuffer中
 * @param  usart 指向USART驱动结构体的指针
 * @note   处理DMA环形接收缓冲区的数据搬移
 */
static void USART_LL_DMA_RX_Copy(USART_LL_Driver_t *usart)
{
    if (!usart) return;
    
    uint16_t bufsize = usart->rxBufSize;
    uint16_t curr_pos = bufsize - LL_DMA_GetDataLength(usart->DMAx, usart->dmaRxChannel); // 计算当前写指针位置
    uint16_t last_pos = usart->dmaRxLastPos; // 上一次读指针位置
    
    if (curr_pos != last_pos) {
        if (curr_pos > last_pos) {
            // 正常情况，未环绕
            USART_LL_Put_RxData_Into_Ringbuffer(usart, usart->rxDMABuffer + last_pos, curr_pos - last_pos);
            usart->rxMsgCount += (curr_pos - last_pos);
        } else {
            // 环绕，分两段处理
            USART_LL_Put_RxData_Into_Ringbuffer(usart, usart->rxDMABuffer + last_pos, bufsize - last_pos);
            USART_LL_Put_RxData_Into_Ringbuffer(usart, usart->rxDMABuffer, curr_pos);
            usart->rxMsgCount += (bufsize - last_pos) + curr_pos;
        }
    }
    usart->dmaRxLastPos = curr_pos; // 更新读指针位置
}

/**
 * @brief  将数据写入接收RingBuffer
 * @param  usart 指向USART驱动结构体的指针
 * @param  data  指向要写入的数据缓冲区
 * @param  len   要写入的数据长度
 * @retval 返回写入状态
 * @note   与发送RingBuffer的写入逻辑相同
 */
static uint8_t USART_LL_Put_RxData_Into_Ringbuffer(USART_LL_Driver_t *usart, const void* data, uint16_t len)
{
    uint8_t ret = 0;
    if (!usart || !data) return 3;
    
    lwrb_t *rb = &usart->rxRB;
    uint16_t rb_size = usart->rxBufSize;
    lwrb_sz_t freeSpace = lwrb_get_free(rb); // ringbuffer剩余空间
    
    if (len < rb_size) { // 数据长度小于ringbuffer容量
        if (len <= freeSpace) { // 足够的剩余空间
            lwrb_write(rb, data, len); // 将数据放入ringbuffer
        } else { // 没有足够的空间，需要丢弃旧数据
            lwrb_sz_t used = lwrb_get_full(rb); // 使用了多少空间
            lwrb_sz_t skip_len = len - freeSpace;
            if (skip_len > used) { // 跳过的数据长度不能超过已使用的长度（例如，缓存58bytes，想接收59bytes，只能丢弃58bytes)
                skip_len = used;
            }
            lwrb_skip(rb, skip_len); // 为了接收新数据，丢弃旧数据
            lwrb_write(rb, data, len); // 将数据放入ringbuffer
            ret = 1;
        }
    } else if (len == rb_size) { // 数据长度等于ringbuffer容量
        if (freeSpace < rb_size) {
            lwrb_reset(rb); // 清空ringbuffer
            ret = 1;
        }
        lwrb_write(rb, data, len); // 将数据放入ringbuffer
    } else { // 数据长度大于ringbuffer容量，数据太大，仅保存最后RX_BUFFER_SIZE字节
        const uint8_t* byte_ptr = (const uint8_t*)data;
        data = (const void*)(byte_ptr + (len - rb_size)); // 指针偏移
        lwrb_reset(rb);
        lwrb_write(rb, data, rb_size);
        ret = 2;
    }
    
    return ret;
}

/**
 * @brief  处理DMA发送通道错误
 * @param  usart 指向USART驱动结构体的指针
 * @retval 1 检测到并处理了错误，0 无错误
 */
static uint8_t USART_LL_DMA_TX_Error_Handler(USART_LL_Driver_t *usart) 
{
    if (!usart) return 0;
    
    // 检查通道是否有传输错误（TE）
    if (LL_DMA_IsActiveFlag_TE(usart->DMAx, usart->dmaTxChannel)) {
        // 清除传输错误标志
        LL_DMA_ClearFlag_TE(usart->DMAx, usart->dmaTxChannel);
        // 关闭DMA通道，停止当前传输
        LL_DMA_DisableChannel(usart->DMAx, usart->dmaTxChannel);
        // 关闭USART的DMA发送请求（DMAT位）
        LL_USART_DisableDMAReq_TX(usart->USARTx);
        // 清除发送标志变量
        usart->txDMABusy = 0;
        usart->errorDMATX++;
        return 1;
    } else {
        return 0;
    }
}

/**
 * @brief  处理DMA接收通道错误
 * @param  usart 指向USART驱动结构体的指针
 * @retval 1 检测到并处理了错误，0 无错误
 */
static uint8_t USART_LL_DMA_RX_Error_Handler(USART_LL_Driver_t *usart) 
{
    if (!usart) return 0;
    
    // 检查通道是否有传输错误（TE）
    if (LL_DMA_IsActiveFlag_TE(usart->DMAx, usart->dmaRxChannel)) {
        // 清除传输错误标志
        LL_DMA_ClearFlag_TE(usart->DMAx, usart->dmaRxChannel);
        // 关闭DMA通道，停止当前传输
        LL_DMA_DisableChannel(usart->DMAx, usart->dmaRxChannel);
        // 重新配置传输长度，恢复到初始状态
        LL_DMA_SetDataLength(usart->DMAx, usart->dmaRxChannel, usart->rxBufSize);
        // 重新使能DMA通道，恢复接收
        LL_DMA_EnableChannel(usart->DMAx, usart->dmaRxChannel);
        usart->errorDMARX++;
        return 1;
    } else {
        return 0;
    }
}

/**
 * @brief  处理USART硬件错误标志
 * @param  usart 指向USART驱动结构体的指针
 * @retval 1 检测到并处理了错误，0 无错误
 */
static uint8_t USART_LL_Hardware_Error_Handler(USART_LL_Driver_t *usart) 
{
    if (!usart) return 0;
    
    // 检查是否有USART错误标志（ORE、NE、FE、PE）
    if (LL_USART_IsActiveFlag_ORE(usart->USARTx) ||
        LL_USART_IsActiveFlag_NE(usart->USARTx)  ||
        LL_USART_IsActiveFlag_FE(usart->USARTx)  ||
        LL_USART_IsActiveFlag_PE(usart->USARTx))
    {
        // 通过读SR、DR来清除错误标志
        volatile uint32_t tmp = usart->USARTx->SR;
        tmp = usart->USARTx->DR;
        (void)tmp;
        return 1;
    } else {
        return 0;
    }
}

/**
 * @brief  获取TX DMA忙碌状态
 * @param  usart 指向USART驱动结构体的指针
 * @retval 0 空闲，1 忙碌
 */
static uint8_t USART_LL_Get_TX_DMA_Busy(USART_LL_Driver_t *usart)
{
    if (!usart) return 1; // 参数无效时认为忙碌
    return usart->txDMABusy;
}
