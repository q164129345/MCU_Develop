#include "myUsartDrive/myUsartDrive_reg.h"
#include "jump_boot.h"

volatile uint8_t rx_buffer[RX_BUFFER_SIZE]; // 接收DMA专用缓冲区
uint64_t g_Usart1_RXCount = 0;              // 统计接收的字节数
volatile uint16_t g_DMARxLastPos = 0;       // 记录上一次读取消息的地址位置

volatile uint8_t tx_buffer[TX_BUFFER_SIZE]; // 发送DMA专用缓存区
volatile uint8_t tx_dma_busy = 0;           // DMA发送标志位（1：正在发送，0：空闲）
uint64_t g_Usart1_TXCount = 0;              // 统计发送的字节数

volatile uint16_t usart1Error = 0;
volatile uint16_t dma1Channel4Error = 0;
volatile uint16_t dma1Channel5Error = 0;

/* usart1 ringbuffer */
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
    /* 寄存器方式 */
    while(*str) {
        while(!(USART1->SR & (0x01UL << 7UL))); // 等待TXE = 1
        USART1->DR = *str++;
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
  * @brief  配置 DMA1 通道5，用于 USART1 RX 的循环接收
  * @note   本函数完成以下操作：
  *         1. 使能 DMA1 时钟，并配置 DMA1_Channel5 中断优先级与使能中断  
  *         2. 禁用 DMA 通道5，并等待其完全关闭  
  *         3. 设置外设地址 (CPAR=USART1->DR)、存储器地址 (CMAR=rx_buffer)  
  *            以及传输数据长度 (CNDTR=RX_BUFFER_SIZE)  
  *         4. 配置 CCR 寄存器各项参数：
  *            - DIR   = 0 (外设 → 存储器)
  *            - CIRC  = 1 (循环模式)
  *            - PINC  = 0 (外设地址不递增)
  *            - MINC  = 1 (存储器地址递增)
  *            - PSIZE = 00 (外设数据宽度 8 位)
  *            - MSIZE = 00 (存储器数据宽度 8 位)
  *            - PL    = 11 (优先级：非常高)
  *            - MEM2MEM = 0 (非存储器到存储器模式)
  *            - 传输完成中断 (TCIE) 与 过半传输中断 (HTIE) 使能
  *         5. 使能 DMA 通道5，启动循环接收
  * @retval None
  */
static void DMA1_Channel5_Configure(void)
{
    // 时钟
    RCC->AHBENR |= (1UL << 0UL); // 开启DMA1时钟
    // 开启全局中断
    NVIC_SetPriority(DMA1_Channel5_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),0, 0));
    NVIC_EnableIRQ(DMA1_Channel5_IRQn);
    /* 1. 禁用DMA通道5，等待其完全关闭 */
    DMA1_Channel5->CCR &= ~(1UL << 0);  // 清除EN位
    while(DMA1_Channel5->CCR & 1UL);    // 等待DMA通道5关闭

    /* 2. 配置外设地址和存储器地址 */
    DMA1_Channel5->CPAR = (uint32_t)&USART1->DR;  // 外设地址为USART1数据寄存器
    DMA1_Channel5->CMAR = (uint32_t)rx_buffer;    // 存储器地址为rx_buffer
    DMA1_Channel5->CNDTR = RX_BUFFER_SIZE;        // 传输数据长度为缓冲区大小
    /* 3. 配置DMA1通道5 CCR寄存器
         - DIR   = 0：外设→存储器
         - CIRC  = 1：循环模式
         - PINC  = 0：外设地址不自增
         - MINC  = 1：存储器地址自增
         - PSIZE = 00：外设数据宽度8位
         - MSIZE = 00：存储器数据宽度8位
         - PL    = 10：优先级设为高
         - MEM2MEM = 0：非存储器到存储器模式
    */
    DMA1_Channel5->CCR = 0;  // 清除之前的配置
    DMA1_Channel5->CCR |= (1UL << 5);       // 使能循环模式 (CIRC，bit5)
    DMA1_Channel5->CCR |= (1UL << 7);       // 使能存储器自增 (MINC，bit7)
    DMA1_Channel5->CCR |= (3UL << 12);      // 设置优先级为非常高 (PL置为“11”，bit12-13)
    
    // 增加传输完成与传输过半中断
    DMA1_Channel5->CCR |= (1UL << 1);       // 传输完成中断 (TCIE)
    DMA1_Channel5->CCR |= (1UL << 2);       // 传输过半中断 (HTIE)
    /* 4. 使能DMA通道5 */
    DMA1_Channel5->CCR |= 1UL;  // 置EN位启动通道
}

/**
  * @brief  配置 DMA1 通道4，用于 USART1 TX 的内存到外设传输
  * @note   本函数完成以下步骤：
  *         1. 使能 DMA1 时钟 (RCC->AHBENR.DMA1EN, bit0)  
  *         2. 设置并开启 DMA1_Channel4 中断，优先级为 2 (NVIC)  
  *         3. 配置传输方向为内存到外设 (CCR.DIR=1, bit4)  
  *         4. 设置通道优先级为高 (CCR.PL=10, bits12–13)  
  *         5. 关闭循环模式 (CCR.CIRC=0, bit5)  
  *         6. 配置地址递增模式：外设地址不递增 (CCR.PINC=0, bit6)，  
  *            存储器地址递增 (CCR.MINC=1, bit7)  
  *         7. 外设与存储器数据宽度均为 8 位 (CCR.PSIZE=00, bits8–9;  
  *            CCR.MSIZE=00, bits10–11)  
  *         8. 使能传输完成中断 (CCR.TCIE=1, bit1)  
  * @retval None
  */
static void DMA1_Channel4_Configure(void)
{
    // 开启时钟
    RCC->AHBENR |= (1UL << 0UL); // 开启DMA1时钟
    // 设置并开启全局中断
    NVIC_SetPriority(DMA1_Channel4_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),2, 0));
    NVIC_EnableIRQ(DMA1_Channel4_IRQn);
    // 数据传输方向
    DMA1_Channel4->CCR &= ~(1UL << 14UL); // 外设到存储器模式
    DMA1_Channel4->CCR |= (1UL << 4UL); // DIR位设置1（内存到外设）
    // 通道优先级
    DMA1_Channel4->CCR &= ~(3UL << 12UL); // 清除PL位
    DMA1_Channel4->CCR |= (2UL << 12UL);  // PL位设置10，优先级高
    // 循环模式
    DMA1_Channel4->CCR &= ~(1UL << 5UL);  // 清除CIRC位，关闭循环模式
    // 增量模式
    DMA1_Channel4->CCR &= ~(1UL << 6UL);  // 外设存储地址不递增
    DMA1_Channel4->CCR |= (1UL << 7UL);   // 存储器地址递增
    // 数据大小
    DMA1_Channel4->CCR &= ~(3UL << 8UL);  // 外设数据宽度8位，清除PSIZE位，相当于00
    DMA1_Channel4->CCR &= ~(3UL << 10UL); // 存储器数据宽度8位，清除MSIZE位，相当于00
    // 中断
    DMA1_Channel4->CCR |= (1UL << 1UL);   // 开启发送完成中断
}

/**
  * @brief  USART1 错误处理函数
  * @note   本函数以寄存器方式检查 USART1 状态寄存器（SR）中的错误标志：
  *            - PE  (位0)：奇偶校验错误  
  *            - FE  (位1)：帧错误  
  *            - NE  (位2)：噪声错误  
  *            - ORE (位3)：接收过载错误  
  *         若任一错误标志被置位，则通过依次读取 SR 和 DR 清除所有错误标志。
  * @retval 0 无错误  
  * @retval 1 检测到错误并已清除
  */
__STATIC_INLINE uint8_t USART1_Error_Handler(void)
{
    // 寄存器方式：检查错误标志（PE、FE、NE、ORE分别位0~3）
    if (USART1->SR & ((1UL << 0) | (1UL << 1) | (1UL << 2) | (1UL << 3))) {
        // 清除错误标志
        volatile uint32_t tmp = USART1->SR;
        tmp = USART1->DR;
        (void)tmp;
        return 1;
    } else {
        return 0;
    }
}

/**
  * @brief  DMA1 通道4 错误处理函数
  * @note   以寄存器方式检查 DMA1 ISR 寄存器中的传输错误标志 TEIF4 (ISR bit15)，
  *         若检测到错误，则执行以下操作：
  *           - 在 IFCR 寄存器中写 1 清除 TEIF4 标志 (IFCR bit15)
  *           - 禁用 DMA1 通道4（CCR.EN = 0）
  *           - 清除 USART1 CR3 寄存器中的 DMAT 位（bit7），停止 DMA 发送请求
  *           - 将全局标志 tx_dma_busy 清零，允许重新发起 DMA 传输
  * @retval 0 无错误
  * @retval 1 检测到传输错误并已处理
  */
__STATIC_INLINE uint8_t DMA1_Channel4_Error_Handler(void)
{
    // 检查传输错误（TE）标志，假设TE对应位(1UL << 15)（请根据具体芯片参考手册确认）
    if (DMA1->ISR & (1UL << 15)) {
        // 清除TE错误标志
        DMA1->IFCR |= (1UL << 15);
        // 禁用DMA通道4
        DMA1_Channel4->CCR &= ~(1UL << 0);
        // 清除USART1中DMAT位
        USART1->CR3 &= ~(1UL << 7);
        // 清除发送标志！！
        tx_dma_busy = 0;
        return 1;
    } else {
        return 0;
    }
}

/**
  * @brief  DMA1 通道5 错误处理函数
  * @note   以寄存器方式检查 DMA1 ISR 寄存器中的传输错误标志 TEIF5 (假设为 bit19)，
  *         若检测到错误，则按以下步骤处理：
  *           1. 在 IFCR 寄存器中写 1 清除 TEIF5 标志 (IFCR bit19)
  *           2. 禁用 DMA1 通道5（CCR.EN = 0）
  *           3. 重置传输计数寄存器 CNDTR 为 RX_BUFFER_SIZE
  *           4. 重新使能 DMA1 通道5（CCR.EN = 1）
  * @retval 0 无错误
  * @retval 1 检测到传输错误并已处理
  */
__STATIC_INLINE uint8_t DMA1_Channel5_Error_Hanlder(void)
{
    // 检查传输错误（TE）标志，假设TE对应位(1UL << 19)（请确认具体位）
    if (DMA1->ISR & (1UL << 19)) {
        // 清除错误标志
        DMA1->IFCR |= (1UL << 19);
        // 禁用DMA通道5
        DMA1_Channel5->CCR &= ~(1UL << 0);
        // 重置传输计数
        DMA1_Channel5->CNDTR = RX_BUFFER_SIZE;
        // 重新使能DMA通道5
        DMA1_Channel5->CCR |= 1UL;
        return 1;
    } else {
        return 0;
    }
}

/**
  * @brief  USART1 TX DMA1_Channel4 传输完成/错误中断处理
  * @note   - 先调用 DMA1_Channel4_Error_Handler() 检查并处理传输错误  
  *         - 若无错误且检测到传输完成 (ISR.TCIF4, bit13)，则：  
  *           1. 在 IFCR 中写1清除 TCIF4 标志 (IFCR bit13)  
  *           2. 禁用 DMA 通道4（CCR.EN = 0）  
  *           3. 清除 USART1 CR3 中的 DMAT 位 (bit7)，停止 DMA 发送请求  
  *           4. 将 tx_dma_busy 标志置 0，通知发送已完成  
  * @retval None
  */
__STATIC_INLINE void USART1_TX_DMA1_Channel4_Interrupt_Handler(void)
{
    if (DMA1_Channel4_Error_Handler()) { // 监控传输错误
        dma1Channel4Error++;
    } else if (DMA1->ISR & (1UL << 13)) {
        // 清除DMA传输完成标志：在IFCR寄存器中写1清除对应标志
        DMA1->IFCR |= (1UL << 13);
        // 禁用DMA通道4（清除CCR寄存器的EN位，位0）
        DMA1_Channel4->CCR &= ~(1UL << 0);
        // 清除USART1中DMAT位，关闭DMA发送请求（CR3寄存器的位7）
        USART1->CR3 &= ~(1UL << 7);
        // 标记DMA发送完成
        tx_dma_busy = 0;
    }
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
  * @brief  USART1 RX DMA1_Channel5 半传输/传输完成/错误中断处理
  * @note   - 先调用 DMA1_Channel5_Error_Hanlder() 检查并处理传输错误  
  *         - 若检测到半传输 (ISR.HTIF5, bit18)：  
  *           1. 在 IFCR 中写1清除 HTIF5 标志 (IFCR bit18)  
  *           2. 累加 RX_BUFFER_SIZE/2 字节到 g_Usart1_RXCount  
  *           3. 将缓冲区前半区数据写入 ringbuffer  
  *         - 若检测到传输完成 (ISR.TCIF5, bit17)：  
  *           1. 在 IFCR 中写1清除 TCIF5 标志 (IFCR bit17)  
  *           2. 累加 RX_BUFFER_SIZE/2 字节到 g_Usart1_RXCount  
  *           3. 将缓冲区后半区数据写入 ringbuffer  
  * @retval None
  */
__STATIC_INLINE void USART1_RX_DMA1_Channel5_Interrupt_Handler(void)
{
    if (DMA1_Channel5_Error_Hanlder()) { // 监控传输错误
        dma1Channel5Error++;
    } else if (DMA1->ISR & (1UL << 18)) { // 半传输中断
        DMA1->IFCR |= (1UL << 18);        // 清除标志
        USART1_DMA_RX_Copy();             // 将DMA接收的数据放入ringbuffer
    } else if (DMA1->ISR & (1UL << 17)) { // 传输完成中断
        DMA1->IFCR |= (1UL << 17);        // 清除标志
        USART1_DMA_RX_Copy();             // 将DMA接收的数据放入ringbuffer
    }
}

/**
  * @brief  USART1 IDLE 中断及错误中断处理
  * @note   - 调用 USART1_Error_Handler() 检测并清除 PE/FE/NE/ORE 错误  
  *         - 若检测到 IDLE (SR.IDLE, bit4)：  
  *           1. 读 SR、DR 清除 IDLE 标志  
  *           2. 禁用 DMA1_Channel5（CCR.EN = 0），获取 CNDTR 剩余字节数  
  *           3. 计算本次接收数据长度，并写入 ringbuffer  
  *           4. 重置 CNDTR 为 RX_BUFFER_SIZE 并重新使能通道  
  * @retval None
  */
__STATIC_INLINE void USART1_RX_Interrupt_Handler(void)
{
    if (USART1_Error_Handler()) { // 监控串口错误
        usart1Error++; // 有错误，记录事件
    } else if (USART1->SR & (1UL << 4)) { // 检查USART1 SR寄存器的IDLE标志（bit4）
        uint32_t tmp;
        // 清除IDLE标志：先读SR再读DR
        tmp = USART1->SR;
        tmp = USART1->DR;
        (void)tmp;
        USART1_DMA_RX_Copy();  // 将DMA接收的数据放入ringbuffer
    }
}

/**
  * @brief  使用DMA发送字符串，采用USART1_TX对应的DMA1通道4
  * @param  data: 待发送数据指针（必须指向独立发送缓冲区）
  * @param  len:  待发送数据长度
  * @retval None
  */
void USART1_SendString_DMA(const uint8_t *data, uint16_t len)
{
    if (len == 0 || len > TX_BUFFER_SIZE) { // 不能让DMA发送0个字节，会导致没办法进入发送完成中断，然后卡死这个函数。
        return;
    }
    // 等待上一次DMA传输完成（也可以添加超时机制）
    while(tx_dma_busy);
    tx_dma_busy = 1; // 标记DMA正在发送
    
    // 如果DMA通道4正在使能，则先禁用以便重新配置
    if(DMA1_Channel4->CCR & 1UL) { // 检查EN位（bit0）是否置位
        DMA1_Channel4->CCR &= ~1UL;  // 禁用DMA通道4（清除EN位）
        while(DMA1_Channel4->CCR & 1UL); // 等待DMA通道4完全关闭
    }
    // 配置DMA通道4：内存地址、外设地址及数据传输长度
    DMA1_Channel4->CMAR  = (uint32_t)data;         // 配置内存地址
    DMA1_Channel4->CPAR  = (uint32_t)&USART1->DR;  // 配置外设地址
    DMA1_Channel4->CNDTR = len;                    // 配置传输数据长度
    // 开启USART1的DMA发送请求：CR3中DMAT（第7位）置1
    USART1->CR3 |= (1UL << 7UL);
    // 启动DMA通道4：设置EN位启动DMA传输
    DMA1_Channel4->CCR |= 1UL;
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

void USART1_Module_Run(void)
{
    /* 1. 接收数据处理（示例） */
    if (lwrb_get_full((lwrb_t*)&g_Usart1RxRBHandler)) {
        // TODO: 调用你的接收处理函数，比如：
        // 从接收环形缓冲区一个字节一个字节地读取数据并处理
        uint8_t rx_byte;
        while (lwrb_read((lwrb_t*)&g_Usart1RxRBHandler, &rx_byte, 1) == 1) {
            // 在这里处理接收到的字节，例如：
            // - 打印到调试终端
            // - 解析协议帧
            // - 存储到应用层缓冲区等
            IAP_Parse_Command(rx_byte); //! 处理IAP报文
            // 示例：简单回显接收到的字节（可根据需要修改）
            // printf("收到字节: 0x%02X ('%c')\n", rx_byte, (rx_byte >= 32 && rx_byte <= 126) ? rx_byte : '.');

        }
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
  * @brief  配置 USART1 外设，包括时钟、GPIO、串口参数、中断与 DMA
  * @note   本函数按以下步骤完成 USART1 的初始化：
  *         1. 初始化收发环形缓冲区（lwrb）  
  *         2. 使能 USART1 与 GPIOA 外设时钟  
  *         3. 配置 PA9（TX）为 10MHz 复用推挽输出，PA10（RX）为浮空输入  
  *         4. 配置 USART1 全局中断，优先级为 0  
  *         5. 设置波特率：72MHz PCLK2，过采样16，BRR=39.0625  
  *         6. 配置数据格式：8 位数据、1 个停止位、无校验  
  *         7. 使能发送（TE）与接收（RE）功能  
  *         8. 关闭硬件流控、LIN、时钟输出、智能卡、IrDA、半双工模式  
  *         9. 使能空闲中断（IDLEIE）  
  *        10. 使能 DMA 接收请求（DMAR），并调用 DMA1_Channel5/4 初始化函数  
  *        11. 最后使能 USART（UE）  
  * @retval None
  */
void USART1_Configure(void) {
    
    /* ringbuffer */
    lwrb_init((lwrb_t*)&g_Usart1RxRBHandler, (uint8_t*)g_Usart1RxRB, sizeof(g_Usart1RxRB) + 1); // RX ringbuffer初始化
    lwrb_init((lwrb_t*)&g_Usart1TxRBHandler, (uint8_t*)g_Usart1TxRB, sizeof(g_Usart1TxRB) + 1); // TX ringbuffer初始化
    
    /* 1. 使能外设时钟 */
    // RCC->APB2ENR 寄存器控制 APB2 外设时钟
    RCC->APB2ENR |= (1UL << 14UL); // 使能 USART1 时钟 (位 14)
    RCC->APB2ENR |= (1UL << 2UL);  // 使能 GPIOA 时钟 (位 2)
    /* 2. 配置 GPIO (PA9 - TX, PA10 - RX) */
    // GPIOA->CRH 寄存器控制 PA8-PA15 的模式和配置
    // PA9: 复用推挽输出 (模式: 10, CNF: 10)
    GPIOA->CRH &= ~(0xF << 4UL);        // 清零 PA9 的配置位 (位 4-7)
    GPIOA->CRH |= (0xA << 4UL);         // PA9: 10MHz 复用推挽输出 (MODE9 = 10, CNF9 = 10)
    // PA10: 浮空输入 (模式: 00, CNF: 01)
    GPIOA->CRH &= ~(0xF << 8UL);        // 清零 PA10 的配置位 (位 8-11)
    GPIOA->CRH |= (0x4 << 8UL);         // PA10: 输入模式，浮空输入 (MODE10 = 00, CNF10 = 01)
    // 开启USART1全局中断
    NVIC_SetPriority(USART1_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),0, 0)); // 优先级1（优先级越低相当于越优先）
    NVIC_EnableIRQ(USART1_IRQn);
    /* 3. 配置 USART1 参数 */
    // (1) 设置波特率 115200 (系统时钟 72MHz, 过采样 16)
    // 波特率计算: USART_BRR = fPCLK / (16 * BaudRate)
    // 72MHz / (16 * 115200) = 39.0625
    // 整数部分: 39 (0x27), 小数部分: 0.0625 * 16 = 1 (0x1)
    USART1->BRR = (39 << 4UL) | 1;      // BRR = 0x271 (39.0625)
    // (2) 配置数据帧格式 (USART_CR1 和 USART_CR2)
    USART1->CR1 &= ~(1UL << 12UL);      // M 位 = 0, 8 位数据
    USART1->CR2 &= ~(3UL << 12UL);      // STOP 位 = 00, 1 个停止位
    USART1->CR1 &= ~(1UL << 10UL);      // 没奇偶校验
    // (3) 配置传输方向 (收发双向)
    USART1->CR1 |= (1UL << 3UL);        // TE 位 = 1, 使能发送
    USART1->CR1 |= (1UL << 2UL);        // RE 位 = 1, 使能接收
    // (4) 禁用硬件流控 (USART_CR3)
    USART1->CR3 &= ~(3UL << 8UL);       // CTSE 和 RTSE 位 = 0, 无流控
    // (5) 配置异步模式 (清除无关模式位)
    USART1->CR2 &= ~(1UL << 14UL);      // LINEN 位 = 0, 禁用 LIN 模式
    USART1->CR2 &= ~(1UL << 11UL);      // CLKEN 位 = 0, 禁用时钟输出
    USART1->CR3 &= ~(1UL << 5UL);       // SCEN 位 = 0, 禁用智能卡模式
    USART1->CR3 &= ~(1UL << 1UL);       // IREN 位 = 0, 禁用 IrDA 模式
    USART1->CR3 &= ~(1UL << 3UL);       // HDSEL 位 = 0, 禁用半双工
    // (6) 中断
    USART1->CR1 |= (1UL << 4UL);        // 使能USART1空闲中断 (IDLEIE, 位4)
    // (7) 关联DMA的接收请求
    USART1->CR3 |= (1UL << 6UL);        // 使能USART1的DMA接收请求（DMAR，位6） 
    // (7) 启用 USART
    USART1->CR1 |= (1UL << 13UL);       // UE 位 = 1, 启用 USART
    
    // DMA初始化
    DMA1_Channel5_Configure();
    DMA1_Channel4_Configure();
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
    DMA1_Channel4->CCR &= ~(1UL << 0); // 禁用DMA1通道4：清除CCR的EN位（位0）
    while (DMA1_Channel4->CCR & 1UL) { // 等待DMA1通道4完全关闭（建议增加超时处理）
        // 空循环等待
    }
    DMA1_Channel5->CCR &= ~(1UL << 0); // 禁用DMA1通道5：清除CCR的EN位（位0）
    while (DMA1_Channel5->CCR & 1UL) { // 等待DMA1通道5完全关闭（建议增加超时处理）
        // 空循环等待
    }
    NVIC_DisableIRQ(USART1_IRQn); // 禁用USART1全局中断，避免重启过程中产生新的中断干扰
    USART1->CR3 &= ~(1UL << 6); // 禁用USART1的DMA接收请求：清除CR3的DMAR位（位6）
    USART1->CR1 &= ~(1UL << 13); // 禁用USART1：清除CR1的UE位（位13）
    
    // 读SR和DR以清除挂起的错误标志（例如IDLE、ORE、NE、FE、PE）
    volatile uint32_t tmp = USART1->SR;
    tmp = USART1->DR;
    (void)tmp;
    
    for (volatile uint32_t i = 0; i < 1000; i++); // 可选：短暂延时，确保USART1完全关闭
    tx_dma_busy = 0; // 复位发送标志！！！！！
    
    /* 重新初始化USART1、DMA1 */
    USART1_Configure();
}

/**
  * @brief This function handles DMA1 channel4 global interrupt.
  */
void DMA1_Channel4_IRQHandler(void)
{
    USART1_TX_DMA1_Channel4_Interrupt_Handler();
}

/**
  * @brief This function handles DMA1 channel5 global interrupt.
  */
void DMA1_Channel5_IRQHandler(void)
{
    USART1_RX_DMA1_Channel5_Interrupt_Handler();
}

/**
  * @brief This function handles USART1 global interrupt.
  */
void USART1_IRQHandler(void)
{
    USART1_RX_Interrupt_Handler();
}




