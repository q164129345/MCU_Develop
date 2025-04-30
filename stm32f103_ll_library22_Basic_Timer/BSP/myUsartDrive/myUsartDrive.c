#include "myUsartDrive/myUsartDrive.h"
#include "usart.h"

volatile uint8_t rx_buffer[RX_BUFFER_SIZE]; // 接收DMA专用缓存区
uint64_t g_Usart1_RXCount = 0;              // 统计接收的字节数

uint8_t tx_buffer[TX_BUFFER_SIZE];          // 发送DMA专用缓存区
volatile uint8_t tx_dma_busy = 0;           // DMA发送标志位（1：正在发送，0：空闲）

volatile uint16_t usart1Error = 0;
volatile uint16_t dma1Channel4Error = 0;
volatile uint16_t dma1Channel5Error = 0;

/* usart1接收ringbuffer */
volatile lwrb_t g_Usart1RxRBHandler; // 实例化ringbuffer
volatile uint8_t g_Usart1RxRB[RX_BUFFER_SIZE] = {0,}; // usart1接收ringbuffer缓存区

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
void USART1_SendString_DMA(const char *data, uint16_t len)
{
    if (len == 0 || len > TX_BUFFER_SIZE) { // 不能让DMA发送0个字节，会导致没办法进入发送完成中断，然后卡死这个函数。
        return;
    }
    // 等待上一次DMA传输完成
    while(tx_dma_busy);
    tx_dma_busy = 1; // 标记DMA正在发送
    memcpy(tx_buffer, data, len); // 将需要发送的数据放入DMA发送缓存区
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
        // 处理前 512 字节数据（偏移 0~511）
        lwrb_write((lwrb_t*)&g_Usart1RxRBHandler, (uint8_t*)rx_buffer, RX_BUFFER_SIZE/2); // 写入ringbuffer
        g_Usart1_RXCount += RX_BUFFER_SIZE/2;
    } else if(LL_DMA_IsActiveFlag_TC5(DMA1)) { // 判断是否产生传输完成中断（后半区完成）
        // 清除传输完成标志
        LL_DMA_ClearFlag_TC5(DMA1);
        // 处理后 512 字节数据（偏移 512~1023）
        lwrb_write((lwrb_t*)&g_Usart1RxRBHandler, (uint8_t*)(rx_buffer + RX_BUFFER_SIZE/2), RX_BUFFER_SIZE/2); // 写入ringbuffer
        g_Usart1_RXCount += RX_BUFFER_SIZE/2;
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

uint8_t USART1_Put_Data_Into_Ringbuffer(const void* data, uint16_t len)
{
    uint8_t ret = 0;
    if (len >= RX_BUFFER_SIZE) {
        if (len > RX_BUFFER_SIZE) {
            len = RX_BUFFER_SIZE;
            ret = 1;  // 数据长度大于ringbuffer总空间，输入的数据被丢弃
        }
        lwrb_reset((lwrb_t*)&g_Usart1RxRBHandler); // 重置ringbuffer
        lwrb_write((lwrb_t*)&g_Usart1RxRBHandler, data, len); // 将数据放入ringbuffer
    } else {
        lwrb_sz_t freeSpace = lwrb_get_free((lwrb_t*)&g_Usart1RxRBHandler);
        
        if (freeSpace > len) { // ringbuffer有多余空闲放入数据
            lwrb_write((lwrb_t*)&g_Usart1RxRBHandler, data, len);
        } else {
            lwrb_skip((lwrb_t*)&g_Usart1RxRBHandler, len - freeSpace); // 只skip必要的空间
            lwrb_write((lwrb_t*)&g_Usart1RxRBHandler, data, len); // 将数据放入ringbuffer
            ret = 2; // ringbuffer的旧数据被丢弃，放入新数据
        }
    }
    return ret;
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
        // 禁用 DMA1 通道5，防止数据继续写入
        LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_5);
        uint16_t remaining = LL_DMA_GetDataLength(DMA1, LL_DMA_CHANNEL_5); // 获取剩余的容量
        uint16_t count = 0;
        // 根据剩余字节判断当前正在哪个半区
        // 还有，避免当数据长度刚好512字节与1024字节时，传输过半中断与空闲中断复制两遍数据，与传输完成中断与空闲中断复制两遍数据。
        if (remaining > (RX_BUFFER_SIZE/2)) {
            // 还在接收前半区：接收数据量 = (1K - remaining)，但肯定不足 512 字节
            count = RX_BUFFER_SIZE - remaining;
            if (count != 0) { // 避免与传输完成中断冲突，多复制一次
                g_Usart1_RXCount += count;
                lwrb_write((lwrb_t*)&g_Usart1RxRBHandler, (uint8_t*)rx_buffer, count); // 写入ringbuffer
            }
        } else {
            // 前半区已写满，当前在后半区：后半区接收数据量 = (RX_BUFFER_SIZE/2 - remaining)
            count = (RX_BUFFER_SIZE/2) - remaining;
            if (count != 0) { // 避免与传输过半中断冲突，多复制一次
                g_Usart1_RXCount += count;
                lwrb_write((lwrb_t*)&g_Usart1RxRBHandler, (uint8_t*)rx_buffer + RX_BUFFER_SIZE/2, count); // 写入ringbuffer
            }
        }

        // 重新设置 DMA 传输长度并使能 DMA
        LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_5, RX_BUFFER_SIZE);
        LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_5);
    }
}

/**
 * @brief  USART1模块运行任务
 * 
 * 在主循环中调用本函数，当ringbuffer有数据时，可以处理
 * 
 * @note
 */
void USART1_Module_Run(void)
{
    if (lwrb_get_full((lwrb_t*)&g_Usart1RxRBHandler)) {
        
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
    
    LL_USART_EnableDMAReq_RX(USART1); // 使能USART1_RX的DMA请求
    LL_USART_EnableIT_IDLE(USART1);   // 开启USART1接收空闲中断
  
    LL_DMA_EnableIT_HT(DMA1, LL_DMA_CHANNEL_5); // 使能DMA1通道5的传输过半中断
    LL_DMA_EnableIT_TC(DMA1, LL_DMA_CHANNEL_5); // 使能DMA1通道5的传输完成中断
  
    LL_DMA_EnableIT_TC(DMA1, LL_DMA_CHANNEL_4); // 使能DMA1通道4的传输完成中断
    
    DMA1_Channel5_Configure(); 
}
