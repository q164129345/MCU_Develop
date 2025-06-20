#include "myUsartDrive/myUsartDrive.h"
#include "usart.h"

volatile uint8_t rx_buffer[RX_BUFFER_SIZE]; // 接收DMA专用缓存区
uint64_t g_Usart1_RXCount = 0;              // 统计接收的字节数
volatile uint16_t g_DMARxLastPos = 0;       // 记录上一次读取消息的地址位置

uint8_t tx_buffer[TX_BUFFER_SIZE];          // 发送DMA专用缓存区
volatile uint8_t tx_dma_busy = 0;           // DMA发送标志位（1：正在发送，0：空闲）
uint64_t g_Usart1_TXCount = 0;              // 统计发送的字节数


volatile uint16_t usart1Error = 0;
volatile uint16_t dma1Channel4Error = 0;
volatile uint16_t dma1Channel5Error = 0;

/* usart1接收ringbuffer */
volatile lwrb_t g_Usart1RxRBHandler; // 实例化ringbuffer
volatile uint8_t g_Usart1RxRB[RX_BUFFER_SIZE] = {0,}; // usart1接收ringbuffer缓存区

volatile lwrb_t g_Usart1TxRBHandler; // 实例化ringbuffer
volatile uint8_t g_Usart1TxRB[TX_BUFFER_SIZE] = {0,}; // usart1发送ringbuffer缓存区

/**
  * @brief   获取 USART1 TX DMA 传输忙状态
  * @note    通过读取全局标志 tx_dma_busy 判断当前 USART1 的 DMA 发送通道是否正在工作
  * @retval  0 表示 DMA 发送空闲，可发起新的传输
  * @retval  1 表示 DMA 发送忙，请等待当前传输完成
  */
__STATIC_INLINE uint8_t USART1_Get_TX_DMA_Busy(void)
{
    return tx_dma_busy;
}

/**
  * @brief  阻塞方式发送以 NUL 结尾的字符串
  * @param  str 指向以 '\0' 结尾的待发送字符串缓冲区
  * @note   本函数通过轮询 TXE 标志位（USART_SR.TXE，位7）来判断发送数据寄存器是否空：
  *         - 当 TXE = 1 时，表示 DR 寄存器已空，可写入下一个字节  
  *         - 通过向 DR 寄存器写入数据（LL_USART_TransmitData8）触发发送  
  *         - 重复上述过程，直到遇到字符串结束符 '\0'  
  * @retval None
  */
void USART1_SendString_Blocking(const char* str)
{
    while (*str) {
        while (!LL_USART_IsActiveFlag_TXE(USART1));
        LL_USART_TransmitData8(USART1, *str++);
    }
}

/**
  * @brief  配置DMA1通道5用于USART1_RX接收。
  * @note   此函数基于LL库实现。它将DMA1通道5的内存地址配置为rx_buffer，
  *         外设地址配置为USART1数据寄存器的地址，并设置传输数据长度为RX_BUFFER_SIZE，
  *         最后使能DMA1通道5以启动接收数据。
  * @retval None
  */
static void DMA1_Channel5_Configure(void) {
    /* 配置DMA1通道5用于USART1_RX接收 */
    LL_DMA_SetMemoryAddress(DMA1, LL_DMA_CHANNEL_5, (uint32_t)rx_buffer);
    LL_DMA_SetPeriphAddress(DMA1, LL_DMA_CHANNEL_5, (uint32_t)&USART1->DR);
    LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_5, RX_BUFFER_SIZE);
    LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_5);
}

/**
  * @brief  使用DMA发送字符串，采用USART1_TX对应的DMA1通道4
  * @param  data: 待发送数据指针（必须指向独立发送缓冲区）
  * @param  len:  待发送数据长度
  * @retval None
  */
static void USART1_SendString_DMA(const uint8_t *data, uint16_t len)
{
    if (len == 0 || len > TX_BUFFER_SIZE) { // 不能让DMA发送0个字节，会导致没办法进入发送完成中断，然后卡死这个函数。
        return;
    }
    // 等待上一次DMA传输完成
    while(tx_dma_busy);
    tx_dma_busy = 1; // 标记DMA正在发送

    // 如果DMA通道4正在使能，则先禁用以便重新配置
    if (LL_DMA_IsEnabledChannel(DMA1, LL_DMA_CHANNEL_4)) {
        LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_4);
        while(LL_DMA_IsEnabledChannel(DMA1, LL_DMA_CHANNEL_4));
    }
    // 配置DMA通道4：内存地址、外设地址及数据传输长度
    LL_DMA_SetMemoryAddress(DMA1, LL_DMA_CHANNEL_4, (uint32_t)tx_buffer);
    LL_DMA_SetPeriphAddress(DMA1, LL_DMA_CHANNEL_4, (uint32_t)&USART1->DR);
    LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_4, len);
    // 开启USART1的DMA发送请求（CR3中DMAT置1，通常为第7位）
    LL_USART_EnableDMAReq_TX(USART1); // 开启USART1的DMA发送请求
    // 启动DMA通道4，开始DMA传输
    LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_4);
}

/**
  * @brief  当检测到USART1异常时，重新初始化USART1及其相关DMA通道。
  * @note   此函数首先停止DMA1通道4（USART1_TX）和通道5（USART1_RX），确保DMA传输终止，
  *         然后禁用USART1及其DMA接收请求，并通过读SR和DR清除挂起的错误标志，
  *         经过短暂延时后调用MX_USART1_UART_Init()重新配置USART1、GPIO和DMA，
  *         MX_USART1_UART_Init()内部已完成USART1中断的配置，无需额外使能。
  * @retval None
  */
void USART1_Reinit(void)
{
    /* 禁用DMA，如果USART1启用了DMA的话 */
    LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_4); // 禁用DMA1通道4
    while(LL_DMA_IsEnabledChannel(DMA1, LL_DMA_CHANNEL_4)) { // 等待通道完全关闭（可以添加超时机制以避免死循环）
        // 空循环等待
    }
    LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_5); // 禁用DMA1通道5
    while(LL_DMA_IsEnabledChannel(DMA1, LL_DMA_CHANNEL_5)) {  // 等待通道完全关闭（可以添加超时机制以避免死循环）
        // 空循环等待
    }
    
    /* USART1 */
    NVIC_DisableIRQ(USART1_IRQn); // 1. 禁用USART1全局中断，避免重启过程中产生新的中断干扰
    LL_USART_DisableDMAReq_RX(USART1); // 2. 禁用USART1的DMA接收请求（如果使用DMA接收）
    LL_USART_Disable(USART1); // 3. 禁用USART1
    
    // 4. 读SR和DR以清除挂起的错误标志（例如IDLE、ORE、NE、FE、PE）
    volatile uint32_t tmp = USART1->SR;
    tmp = USART1->DR;
    (void)tmp;
    
    for (volatile uint32_t i = 0; i < 1000; i++); // 5. 可选：短暂延时，确保USART完全关闭
    tx_dma_busy = 0; // 复位发送标志
    MX_USART1_UART_Init(); // 重新初始化USART1
    DMA1_Channel5_Configure(); // 重新初始化DMA1通道5
}

/**
  * @brief  检查DMA1通道4传输错误，并处理错误状态。
  * @note   此函数基于LL库实现。主要用于检测USART1_TX传输过程中DMA1通道4是否发生传输错误（TE）。
  *         如果检测到错误，则清除错误标志、禁用DMA1通道4，并关闭USART1的DMA发送请求，
  *         从而终止当前传输。返回1表示错误已处理；否则返回0。
  * @retval uint8_t 错误状态：1表示检测到并处理了错误，0表示无错误。
  */
__STATIC_INLINE uint8_t DMA1_Channel4_Error_Handler(void) {
    // 检查通道4是否发生传输错误（TE）
    if (LL_DMA_IsActiveFlag_TE4(DMA1)) {
        // 清除传输错误标志
        LL_DMA_ClearFlag_TE4(DMA1);
        // 禁用DMA通道4，停止当前传输
        LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_4);
        // 清除USART1的DMA发送请求（DMAT位）
        LL_USART_DisableDMAReq_TX(USART1);
        // 清除发送标志！！
        tx_dma_busy = 0;
        return 1;
    } else {
        return 0;
    }
}

/**
 * @brief  USART1发送DMA1_Channel4中断处理函数
 * 
 * 处理USART1通过DMA1通道4发送过程中产生的中断，包括：
 * - DMA传输错误
 * - 传输完成（TC，Transfer Complete）
 * 
 * @note
 * - 使用LL库操作DMA和USART寄存器位。
 * - 发送完成后关闭DMA发送通道，并清除USART的DMAT请求位。
 * - 通过标志变量tx_dma_busy指示发送完成，允许下一次发送。
 * 
 * @details
 * 1. 检查DMA传输错误并统计错误次数。
 * 2. 检查发送完成中断（TC）：
 *    - 清除TC中断标志。
 *    - 禁用DMA1_Channel4，防止误触发。
 *    - 禁止USART1的DMA发送请求（DMAT位清零）。
 *    - 清除发送忙标志(tx_dma_busy = 0)。
 * 
 * @warning
 * - 中断处理过程中，必须及时清除TC中断标志，否则中断会持续触发。
 * - 禁用DMA发送请求前，确保DMA传输已经完成。
 */
void USART1_TX_DMA1_Channel4_Interrupt_Handler(void)
{
    if (DMA1_Channel4_Error_Handler()) { // 监控传输错误
        dma1Channel4Error++;
    } else if(LL_DMA_IsActiveFlag_TC4(DMA1)) {// 检查传输完成标志（TC）是否置位（LL库提供TC4接口）
        // 清除DMA传输完成标志
        LL_DMA_ClearFlag_TC4(DMA1);
        // 禁用DMA通道4，确保下次传输前重新配置
        LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_4);
        // 清除USART1中DMAT位，关闭DMA发送请求
        LL_USART_DisableDMAReq_TX(USART1);
        // 标记DMA发送完成
        tx_dma_busy = 0;
    }
}

/**
  * @brief  检查DMA1通道5传输错误，并恢复DMA接收。
  * @note   此函数基于LL库实现。主要用于检测USART1_RX传输过程中DMA1通道5是否发生传输错误（TE）。
  *         如果检测到错误，则清除错误标志、禁用DMA1通道5、重置传输长度为RX_BUFFER_SIZE，
  *         并重新使能DMA通道5以恢复数据接收。返回1表示错误已处理；否则返回0。
  * @retval uint8_t 错误状态：1表示检测到并处理了错误，0表示无错误。
  */
__STATIC_INLINE uint8_t DMA1_Channel5_Error_Hanlder(void) {
    // 检查通道5是否发生传输错误（TE）
    if (LL_DMA_IsActiveFlag_TE5(DMA1)) {
        // 清除传输错误标志
        LL_DMA_ClearFlag_TE5(DMA1);
        // 禁用DMA通道5，停止当前传输
        LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_5);
        // 重新设置传输长度，恢复到初始状态
        LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_5, RX_BUFFER_SIZE);
        // 重新使能DMA通道5，恢复接收
        LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_5);
        return 1;
    } else {
        return 0;
    }
}

/**
  * @brief  将数据写入USART1接收ringbuffer中。

  * @param[in] data 指向要写入的数据缓冲区
  * @param[in] len  要写入的数据长度（单位：字节）
  * 
  * @retval 0 数据成功写入，无数据丢弃
  * @retval 1 ringbuffer剩余空间不足，旧数据被丢弃以容纳新数据
  * @retval 2 数据长度超过ringbuffer总容量，全部旧数据被清空，且新数据也有丢失
  * @retval 3 空数据指针
  * @note 
  * - 使用lwrb库操作ringbuffer。
  * - 当len > RX_BUFFER_SIZE时，为防止写入越界，会强行截断为RX_BUFFER_SIZE大小。
  * - 若ringbuffer空间不足，将调用 `lwrb_skip()` 跳过旧数据，优先保留最新数据。
  */
static uint8_t USART1_Put_Data_Into_Ringbuffer(const void* data, uint16_t len)
{
    uint8_t ret = 0;
    if (data == NULL) return ret = 3;
    
    lwrb_sz_t freeSpace = lwrb_get_free((lwrb_t*)&g_Usart1RxRBHandler); // ringbuffer剩余空间
    
    if (len < RX_BUFFER_SIZE) { // 新数据长度小于ringbuffer的总容量
        if (len <= freeSpace) { // 足够的剩余空间
            lwrb_write((lwrb_t*)&g_Usart1RxRBHandler, data, len); // 将数据放入ringbuffer
        } else { // 没有足够的空间，所以要跳过部分旧数据
            lwrb_sz_t used = lwrb_get_full((lwrb_t*)&g_Usart1RxRBHandler); // 使用了多少空间
            lwrb_sz_t skip_len = len - freeSpace;
            if (skip_len > used) { // 关键！跳过的数据长度不能超过已有数据总长，避免越界（比如，有58bytes，不能跳过59bytes，最多只能跳过58bytes)
                skip_len = used;
            }
            lwrb_skip((lwrb_t*)&g_Usart1RxRBHandler, skip_len); // 为了接收新的数据，丢弃部分旧的数据
            lwrb_write((lwrb_t*)&g_Usart1RxRBHandler, data, len); // 将数据放入ringbuffer
            ret = 1;
        }
    } else if (len == RX_BUFFER_SIZE) { // 新数据长度等于ringbuffer的总容量
        if (freeSpace < RX_BUFFER_SIZE) {
            lwrb_reset((lwrb_t*)&g_Usart1RxRBHandler); // 重置ringbuffer
            ret = 1;
        }
        lwrb_write((lwrb_t*)&g_Usart1RxRBHandler, data, len); // 将数据放入ringbuffer
    } else { // 新数据长度大于ringbuffer的总容量。数据太大，仅保留最后RX_BUFFER_SIZE字节
        const uint8_t* byte_ptr = (const uint8_t*)data;
        data = (const void*)(byte_ptr + (len - RX_BUFFER_SIZE)); // 指针偏移
        lwrb_reset((lwrb_t*)&g_Usart1RxRBHandler);
        lwrb_write((lwrb_t*)&g_Usart1RxRBHandler, data, RX_BUFFER_SIZE);
        ret = 2;
    }
    
    return ret;
}

/**
  * @brief  将DMA接收到的新数据拷贝到ringbuffer中（适用于循环缓冲DMA方式）
  * 
  * 本函数用于DMA接收环形缓冲区的数据同步，将DMA新接收到的数据从rx_buffer中拷贝到软件ringbuffer。
  * 支持处理DMA缓冲区指针环绕的情况，保证数据不丢失且顺序正确。
  * 
  * @note
  * - 必须周期性在DMA相关中断（如空闲中断/定时调度）中调用本函数，否则数据会被新数据覆盖丢失。
  * - 通过`LL_DMA_GetDataLength()`获取当前DMA剩余传输量，反推出当前硬件写指针。
  * - g_DMARxLastPos需全局保存上一次已读位置，避免重复处理。
  *
  * @par 注意事项
  * - 若`curr_pos < last_pos`，说明DMA写指针已环绕，需要分两段分别拷贝数据。
  * - 本函数仅负责数据同步，不涉及DMA使能/禁用及中断标志处理。
  */
__STATIC_INLINE void USART1_DMA_RX_Copy(void)
{
    uint16_t bufsize = sizeof(rx_buffer);
    uint16_t curr_pos = bufsize - LL_DMA_GetDataLength(DMA1, LL_DMA_CHANNEL_5); // 计算当前的写指针位置
    uint16_t last_pos = g_DMARxLastPos; // 上一次读指针位置
    
    if (curr_pos != last_pos) {
        if (curr_pos > last_pos) {
            // 普通情况，未环绕
            USART1_Put_Data_Into_Ringbuffer((uint8_t*)rx_buffer + last_pos, curr_pos - last_pos);
            g_Usart1_RXCount += (curr_pos - last_pos);
        } else {
            // 环绕，分两段处理
            USART1_Put_Data_Into_Ringbuffer((uint8_t*)rx_buffer + last_pos, bufsize - last_pos);
            USART1_Put_Data_Into_Ringbuffer((uint8_t*)rx_buffer, curr_pos);
            g_Usart1_RXCount += (bufsize - last_pos) + curr_pos;
        }
    }
    g_DMARxLastPos = curr_pos; // 更新读指针位置
}

/**
 * @brief  USART1接收DMA1_Channel5中断处理函数
 * 
 * 处理USART1通过DMA1通道5接收过程中产生的中断，包括：
 * - DMA传输错误
 * - 半传输完成（HT，Half Transfer）
 * - 全传输完成（TC，Transfer Complete）
 * 
 * @note
 * - 使用LL库操作DMA中断标志，确保高效处理中断源。
 * - 半传输中断表示前半区(0~511字节)接收完成；
 * - 完成中断表示后半区(512~1023字节)接收完成；
 * - 接收数据通过memcpy拷贝到发送缓存tx_buffer，并置位rx_complete。
 * 
 * @details
 * 1. 检查DMA传输错误并统计错误次数。
 * 2. 半传输中断：
 *    - 清除HT标志。
 *    - 复制接收缓存前半部分数据。
 *    - 设置接收数据长度与完成标志。
 * 3. 完成中断：
 *    - 清除TC标志。
 *    - 复制接收缓存后半部分数据。
 *    - 设置接收数据长度与完成标志。
 * 
 * @warning
 * - 中断服务函数中务必及时清除HT/TC中断标志，否则中断会持续触发。
 * - 注意同步DMA接收缓存(rx_buffer)与用户缓存(tx_buffer)的数据一致性。
 */
void USART1_RX_DMA1_Channel5_Interrupt_Handler(void)
{
    if (DMA1_Channel5_Error_Hanlder()) { // 监控传输失败
        dma1Channel5Error++;
    } else if(LL_DMA_IsActiveFlag_HT5(DMA1)) { // 判断是否产生半传输中断（前半区完成）
        // 清除半传输标志
        LL_DMA_ClearFlag_HT5(DMA1);
        USART1_DMA_RX_Copy(); // 将DMA接收到的数据放到ringbuffer
    } else if(LL_DMA_IsActiveFlag_TC5(DMA1)) { // 判断是否产生传输完成中断（后半区完成）
        // 清除传输完成标志
        LL_DMA_ClearFlag_TC5(DMA1);
        USART1_DMA_RX_Copy(); // 将DMA接收到的数据放到ringbuffer
    }
}

/**
  * @brief  检查USART1错误标志，并清除相关错误标志（ORE、NE、FE、PE）。
  * @note   此函数基于LL库实现。当检测到USART1错误标志时，通过读取SR和DR清除错误，
  *         并返回1表示存在错误；否则返回0。
  * @retval uint8_t 错误状态：1表示检测到错误并已清除，0表示无错误。
  */
__STATIC_INLINE uint8_t USART1_Error_Handler(void) {
    // 错误处理：检查USART错误标志（ORE、NE、FE、PE）
    if (LL_USART_IsActiveFlag_ORE(USART1) ||
        LL_USART_IsActiveFlag_NE(USART1)  ||
        LL_USART_IsActiveFlag_FE(USART1)  ||
        LL_USART_IsActiveFlag_PE(USART1))
    {
        // 通过读SR和DR来清除错误标志
        volatile uint32_t tmp = USART1->SR;
        tmp = USART1->DR;
        (void)tmp;
        return 1;
    } else {
        return 0;
    }
}

/**
 * @brief  USART1中断处理函数
 * 
 * 处理USART1产生的中断，包括：
 * - 错误检测（帧错误、噪声错误、溢出错误等）
 * - 接收空闲中断（IDLE）触发的数据搬运与DMA通道重新配置
 * 
 * @note
 * - 使用LL库直接操作USART寄存器和DMA寄存器，确保高效处理。
 * - 空闲中断用于快速判断一帧接收完成，无需等待缓冲区填满。
 * - 为避免DMA半传输中断或完成中断时数据重复拷贝，需根据剩余数据量判断当前帧数据。
 * 
 * @details
 * 1. 检查并处理USART硬件错误（如有）。
 * 2. 检查USART空闲中断（IDLE）：
 *    - 清除IDLE标志（读SR→读DR）。
 *    - 禁用DMA接收通道，防止数据继续写入。
 *    - 依据DMA剩余数据判断已接收的数据量，并复制到发送缓存tx_buffer。
 *    - 设置接收到的数据长度(recved_length)，并置位rx_complete。
 *    - 重置DMA传输长度，重新启用DMA接收。
 * 
 * @warning
 * - 必须正确清除IDLE中断标志，否则USART中断将持续触发。
 * - 注意DMA接收缓存边界条件，避免数据错位或覆盖。
 */
void USART1_RX_Interrupt_Handler(void)
{
    if (USART1_Error_Handler()) { // 检查错误标志
        usart1Error++; // 有错误，记录事件
    } else if (LL_USART_IsActiveFlag_IDLE(USART1)) {  // 检查 USART1 是否因空闲而中断
        uint32_t tmp;
        /* 清除IDLE标志：必须先读SR，再读DR */
        tmp = USART1->SR;
        tmp = USART1->DR;
        (void)tmp;
        USART1_DMA_RX_Copy(); // 将接收到的数据放到ringbuffer
    }
}

/**
  * @brief   将数据写入 USART1 发送 ringbuffer 中
  * @param[in] data 指向要写入的待发送数据缓冲区
  * @param[in] len  要写入的数据长度（单位：字节）
  * @retval  0 数据成功写入，无数据丢弃
  * @retval  1 ringbuffer 空间不足，丢弃部分旧数据以容纳新数据
  * @retval  2 数据长度超过 ringbuffer 总容量，仅保留最后 TX_BUFFER_SIZE 字节
  * @retval  3 空数据指针
  * @note
  *   - 使用 lwrb 库操作发送 ringbuffer（g_Usart1TxRBHandler）
  *   - 若 len > TX_BUFFER_SIZE，会截断为最后 TX_BUFFER_SIZE 字节
  *   - 若空间不足，将调用 lwrb_skip() 丢弃旧数据
  */
uint8_t USART1_Put_TxData_To_Ringbuffer(const void* data, uint16_t len)
{
    uint8_t ret = 0;
    if (data == NULL) {
        return 3;
    }

    lwrb_sz_t capacity  = TX_BUFFER_SIZE;
    lwrb_sz_t freeSpace = lwrb_get_free((lwrb_t*)&g_Usart1TxRBHandler); // 获取剩余空间

    if (len < capacity) {
        if (len <= freeSpace) {
            // 直接写入
            lwrb_write((lwrb_t*)&g_Usart1TxRBHandler, data, len);
        } else {
            // 空间不足，丢弃旧数据
            lwrb_sz_t used     = lwrb_get_full((lwrb_t*)&g_Usart1TxRBHandler);
            lwrb_sz_t skip_len = len - freeSpace;
            if (skip_len > used) {
                skip_len = used;
            }
            lwrb_skip((lwrb_t*)&g_Usart1TxRBHandler, skip_len);
            lwrb_write((lwrb_t*)&g_Usart1TxRBHandler, data, len);
            ret = 1;
        }
    } else if (len == capacity) {
        // 刚好等于容量
        if (freeSpace < capacity) {
            lwrb_reset((lwrb_t*)&g_Usart1TxRBHandler);
            ret = 1;
        }
        lwrb_write((lwrb_t*)&g_Usart1TxRBHandler, data, len);
    } else {
        // len > 容量，仅保留最后 capacity 字节
        const uint8_t* ptr = (const uint8_t*)data + (len - capacity);
        lwrb_reset((lwrb_t*)&g_Usart1TxRBHandler);
        lwrb_write((lwrb_t*)&g_Usart1TxRBHandler, ptr, capacity);
        ret = 2;
    }

    return ret;
}

/**
 * @brief  USART1模块运行任务，回调周期建议1ms
 * 
 * 在主循环中调用本函数，当ringbuffer有数据时，可以处理
 * 
 * @note
 */
void USART1_Module_Run(void)
{
    /* 1. 接收数据处理（示例） */
    if (lwrb_get_full((lwrb_t*)&g_Usart1RxRBHandler)) {
        // TODO: 调用你的接收处理函数，比如：
        // Process_Usart1_RxData();
    }
    
    /* 2. 发送数据：有数据 & DMA 空闲 */
    uint16_t available = lwrb_get_full((lwrb_t*)&g_Usart1TxRBHandler);
    if (available && 0x00 == USART1_Get_TX_DMA_Busy()) {
        uint16_t len = (available > TX_BUFFER_SIZE) ? TX_BUFFER_SIZE : (uint16_t)available; // 计算待发长度
        lwrb_read((lwrb_t*)&g_Usart1TxRBHandler, (uint8_t*)tx_buffer, len); // 从环形缓冲区读取到 DMA 缓冲区
        g_Usart1_TXCount += len; // 统计发送总数
        USART1_SendString_DMA((uint8_t*)tx_buffer, len); // 发起 DMA 发送
    }
}

/**
 * @brief  USART1与DMA模块配置
 * 
 * 配置USART1外设和DMA通道以实现高效的接收和发送功能。
 * 包括使能USART的DMA请求、空闲中断，以及DMA的传输中断。
 * 同时调用DMA1_Channel5_Configure()进行DMA接收通道初始化配置。
 * 
 * @note
 * - LL_USART_EnableDMAReq_RX: 允许USART1接收数据时通过DMA搬运，减少CPU负担。
 * - LL_USART_EnableIT_IDLE: 打开USART1接收空闲中断，用于帧边界检测。
 * - LL_DMA_EnableIT_HT: 开启DMA半传输中断（处理前半区数据）。
 * - LL_DMA_EnableIT_TC: 开启DMA传输完成中断（处理后半区数据）。
 * - 依赖外部函数DMA1_Channel5_Configure()进行接收通道基本配置。
 */
void USART1_Config(void)
{
    lwrb_init((lwrb_t*)&g_Usart1RxRBHandler, (uint8_t*)g_Usart1RxRB, sizeof(g_Usart1RxRB) + 1); // RX ringbuffer初始化
    lwrb_init((lwrb_t*)&g_Usart1TxRBHandler, (uint8_t*)g_Usart1TxRB, sizeof(g_Usart1TxRB) + 1); // TX ringbuffer初始化
    
    LL_USART_EnableDMAReq_RX(USART1); // 使能USART1_RX的DMA请求
    LL_USART_EnableIT_IDLE(USART1);   // 开启USART1接收空闲中断
  
    LL_DMA_EnableIT_HT(DMA1, LL_DMA_CHANNEL_5); // 使能DMA1通道5的传输过半中断
    LL_DMA_EnableIT_TC(DMA1, LL_DMA_CHANNEL_5); // 使能DMA1通道5的传输完成中断
  
    LL_DMA_EnableIT_TC(DMA1, LL_DMA_CHANNEL_4); // 使能DMA1通道4的传输完成中断
    
    DMA1_Channel5_Configure(); 
}