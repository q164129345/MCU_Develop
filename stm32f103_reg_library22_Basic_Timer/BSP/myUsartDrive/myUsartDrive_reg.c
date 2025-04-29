#include "myUsartDrive/myUsartDrive_reg.h"

volatile uint8_t rx_buffer[RX_BUFFER_SIZE];
volatile uint8_t rx_complete = 0;
volatile uint16_t recvd_length = 0;

volatile uint8_t tx_buffer[TX_BUFFER_SIZE];
volatile uint8_t tx_dma_busy = 0;

volatile uint16_t usart1Error = 0;
volatile uint16_t dma1Channel4Error = 0;
volatile uint16_t dma1Channel5Error = 0;


__STATIC_INLINE void Enable_Peripherals_Clock(void)
{
    SET_BIT(RCC->APB2ENR, 1UL << 0UL);  // 启动AFIO时钟  // 一般的工程都要开
    SET_BIT(RCC->APB1ENR, 1UL << 28UL); // 启动PWR时钟   // 一般的工程都要开
    SET_BIT(RCC->APB2ENR, 1UL << 5UL);  // 启动GPIOD时钟 // 晶振时钟
    SET_BIT(RCC->APB2ENR, 1UL << 2UL);  // 启动GPIOA时钟 // SWD接口
    __NOP(); // 稍微延时一下下
}

// 配置USART1_RX的DMA1通道5
__STATIC_INLINE void DMA1_Channel5_Configure(void)
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

// 配置DMA1的通道4：普通模式，内存到外设(flash->USART1_TX)，优先级高，存储器地址递增、数据大小都是8bit
__STATIC_INLINE void DMA1_Channel4_Configure(void)
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

__STATIC_INLINE void USART1_RX_DMA1_Channel5_Interrupt_Handler(void)
{
    if (DMA1_Channel5_Error_Hanlder()) { // 监控传输错误
        dma1Channel5Error++;
    } else if (DMA1->ISR & (1UL << 18)) { // 半传输中断
        DMA1->IFCR |= (1UL << 18);
        // 前半缓冲区处理
        memcpy((void*)tx_buffer, (const void*)rx_buffer, RX_BUFFER_SIZE / 2);
        recvd_length = RX_BUFFER_SIZE / 2;
        rx_complete = 1;
    } else if (DMA1->ISR & (1UL << 17)) { // 传输完成中断
        DMA1->IFCR |= (1UL << 17);
        // 后半缓冲区处理
        memcpy((void*)tx_buffer, (const void*)(rx_buffer + RX_BUFFER_SIZE / 2), RX_BUFFER_SIZE / 2);
        recvd_length = RX_BUFFER_SIZE / 2;
        rx_complete = 1;
    }
}

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
        // 禁用DMA1通道5：清除CCR寄存器的EN位（bit0）
        DMA1_Channel5->CCR &= ~(1UL << 0);
        // 获取剩余的数据量
        uint16_t remaining = DMA1_Channel5->CNDTR;
        uint16_t count = 0;

        // 根据剩余字节判断当前正在哪个半区
        // 注意：为避免当数据长度刚好为512字节或1024字节时，与DMA半传输/传输完成中断冲突，
        // 在count不为0时才进行数据复制
        if (remaining > (RX_BUFFER_SIZE / 2)) {
            // 还在接收前半区：接收数据量 = (总长度 - remaining)，不足512字节
            count = RX_BUFFER_SIZE - remaining;
            if (count != 0) {  // 避免与传输完成中断冲突，多复制一次
                memcpy((void*)tx_buffer, (const void*)rx_buffer, count);
            }
        } else {
            // 前半区已满，当前在后半区：后半区接收数据量 = (前半区长度 - remaining)
            count = (RX_BUFFER_SIZE / 2) - remaining;
            if (count != 0) {  // 避免与传输过半中断冲突，多复制一次
                memcpy((void*)tx_buffer, (const void*)rx_buffer + (RX_BUFFER_SIZE / 2), count);
            }
        }
        if (count != 0) {
            recvd_length = count;
            rx_complete = 1;
        }
        // 重置DMA接收：设置CNDTR寄存器为RX_BUFFER_SIZE
        DMA1_Channel5->CNDTR = RX_BUFFER_SIZE;
        // 重新使能DMA1通道5：设置EN位（bit0）
        DMA1_Channel5->CCR |= 1UL;
    }
}

/**
  * @brief  使用DMA发送字符串，采用USART1_TX对应的DMA1通道4
  * @param  data: 待发送数据指针（必须指向独立发送缓冲区）
  * @param  len:  待发送数据长度
  * @retval None
  */
void USART1_SendString_DMA(const char *data, uint16_t len)
{
    if (len == 0) {
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

void USART1_Module_Run(void)
{
    if (rx_complete) {
        USART1_SendString_DMA((const char*)tx_buffer, recvd_length);
        rx_complete = 0; // 清除标志，等待下一次接收
    }
}

void USART1_Configure(void) {
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

void DMA1_Channel4_IRQHandler(void)
{
    USART1_TX_DMA1_Channel4_Interrupt_Handler();
}

void DMA1_Channel5_IRQHandler(void)
{
    USART1_RX_DMA1_Channel5_Interrupt_Handler();
}

void USART1_IRQHandler(void)
{
    USART1_RX_Interrupt_Handler();
}
































