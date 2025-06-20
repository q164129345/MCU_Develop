# BSP USART LL 驱动使用示例

## 概述

本文档演示如何使用基于LL库的多实例化USART驱动，支持同时管理多个串口（USART1、USART2、USART3等）。

## 核心特性

🚀 **高性能**：基于LL库直接操作寄存器，性能比HAL库更高  
🔄 **多实例**：支持同时管理多个USART，每个都有独立的配置  
📦 **DMA支持**：异步收发，CPU占用率低  
🔒 **RingBuffer**：智能缓冲区管理，防止数据丢失  
⚡ **中断驱动**：实时响应，延迟低  

## 使用步骤

### 第1步：定义驱动实例和缓冲区

```c
#include "bsp_usart_drive/bsp_usart_drive.h"

/* ===== USART1 实例定义 ===== */
USART_LL_Driver_t usart1_driver;

// USART1 缓冲区定义（这些就像每个邮递员的"工具包"）
static uint8_t usart1_rx_dma_buffer[1024];      // DMA接收缓冲区：临时存放
static uint8_t usart1_rx_rb_buffer[1024];       // 接收RingBuffer：长期存储
static uint8_t usart1_tx_dma_buffer[2048];      // DMA发送缓冲区：临时存放  
static uint8_t usart1_tx_rb_buffer[2048];       // 发送RingBuffer：发送队列

/* ===== USART2 实例定义（可选） ===== */
USART_LL_Driver_t usart2_driver;

// USART2 缓冲区定义
static uint8_t usart2_rx_dma_buffer[512];
static uint8_t usart2_rx_rb_buffer[512];
static uint8_t usart2_tx_dma_buffer[1024];
static uint8_t usart2_tx_rb_buffer[1024];
```

### 第2步：初始化驱动实例

```c
void User_USART_Init(void)
{
    /* 注意：必须先通过CubeMX配置好USART和DMA硬件 */
    
    // 初始化 USART1 驱动（就像给邮递员分配工具和路线）
    USART_LL_Config(&usart1_driver,
                    USART1,                    // USART寄存器
                    DMA1,                      // DMA控制器  
                    LL_DMA_CHANNEL_4,          // TX DMA通道
                    LL_DMA_CHANNEL_5,          // RX DMA通道
                    usart1_rx_dma_buffer,      // RX DMA缓冲区
                    usart1_rx_rb_buffer,       // RX RingBuffer
                    sizeof(usart1_rx_dma_buffer),
                    usart1_tx_dma_buffer,      // TX DMA缓冲区
                    usart1_tx_rb_buffer,       // TX RingBuffer
                    sizeof(usart1_tx_dma_buffer));
    
    // 初始化 USART2 驱动（如果需要）
    USART_LL_Config(&usart2_driver,
                    USART2,                    // USART寄存器
                    DMA1,                      // DMA控制器
                    LL_DMA_CHANNEL_7,          // TX DMA通道
                    LL_DMA_CHANNEL_6,          // RX DMA通道
                    usart2_rx_dma_buffer,
                    usart2_rx_rb_buffer,
                    sizeof(usart2_rx_dma_buffer),
                    usart2_tx_dma_buffer,
                    usart2_tx_rb_buffer,
                    sizeof(usart2_tx_dma_buffer));
}
```

### 第3步：在中断服务程序中调用处理函数

```c
/* ===== DMA中断处理 ===== */
void DMA1_Channel4_IRQHandler(void)  // USART1 TX DMA
{
    USART_LL_DMA_TX_Interrupt_Handler(&usart1_driver);
}

void DMA1_Channel5_IRQHandler(void)  // USART1 RX DMA  
{
    USART_LL_DMA_RX_Interrupt_Handler(&usart1_driver);
}

void DMA1_Channel6_IRQHandler(void)  // USART2 RX DMA
{
    USART_LL_DMA_RX_Interrupt_Handler(&usart2_driver);
}

void DMA1_Channel7_IRQHandler(void)  // USART2 TX DMA
{
    USART_LL_DMA_TX_Interrupt_Handler(&usart2_driver);
}

/* ===== USART中断处理 ===== */
void USART1_IRQHandler(void)
{
    USART_LL_RX_Interrupt_Handler(&usart1_driver);
}

void USART2_IRQHandler(void)
{
    USART_LL_RX_Interrupt_Handler(&usart2_driver);
}
```

### 第4步：在主循环中运行驱动

```c
int main(void)
{
    /* 系统初始化... */
    User_USART_Init();
    
    while (1)
    {
        // 运行USART驱动（就像邮局管理员定期检查邮件）
        USART_LL_Module_Run(&usart1_driver);
        USART_LL_Module_Run(&usart2_driver);
        
        // 处理接收到的数据
        Process_Received_Data();
        
        // 其他业务逻辑...
        
        HAL_Delay(1);  // 1ms间隔
    }
}
```

## 发送数据

### 方法1：异步发送（推荐）

```c
void Send_Data_Example(void)
{
    char message[] = "Hello from USART1!\r\n";
    
    // 将数据放入发送队列（就像把信件投入邮箱）
    uint8_t result = USART_LL_Put_TxData_To_Ringbuffer(&usart1_driver, 
                                                       message, 
                                                       strlen(message));
    
    switch(result) {
        case 0: printf("发送成功\n"); break;
        case 1: printf("缓冲区满，丢弃了部分旧数据\n"); break;
        case 2: printf("数据太长，被截断\n"); break;
        case 3: printf("参数错误\n"); break;
    }
    
    // 数据会在主循环的 USART_LL_Module_Run() 中自动发送
}
```

### 方法2：阻塞发送（调试用）

```c
void Send_Debug_Message(void)
{
    // 立即发送，会阻塞直到发送完成（就像亲自送信）
    USART_LL_SendString_Blocking(&usart1_driver, "Debug message\r\n");
}
```

## 接收数据

```c
void Process_Received_Data(void)
{
    // 检查USART1是否有数据
    uint32_t available = USART_LL_Get_Available_RxData_Length(&usart1_driver);
    
    if (available > 0) {
        printf("USART1收到 %d 字节数据\n", available);
        
        // 逐字节读取数据
        uint8_t byte;
        while (USART_LL_Read_A_Byte_Data(&usart1_driver, &byte)) {
            // 处理接收到的字节
            printf("接收到: 0x%02X ('%c')\n", byte, 
                   (byte >= 32 && byte <= 126) ? byte : '.');
        }
    }
    
    // 同样处理USART2
    available = USART_LL_Get_Available_RxData_Length(&usart2_driver);
    if (available > 0) {
        // 处理USART2数据...
    }
}
```

## 错误处理和监控

```c
void Monitor_USART_Status(void)
{
    // 查看统计信息
    printf("USART1 状态:\n");
    printf("  发送字节数: %llu\n", usart1_driver.txMsgCount);
    printf("  接收字节数: %llu\n", usart1_driver.rxMsgCount);
    printf("  DMA发送错误: %u\n", usart1_driver.errorDMATX);
    printf("  DMA接收错误: %u\n", usart1_driver.errorDMARX);
    printf("  串口接收错误: %u\n", usart1_driver.errorRX);
    printf("  DMA忙碌状态: %s\n", usart1_driver.txDMABusy ? "忙碌" : "空闲");
}
```

## 高级配置示例

### 不同大小的缓冲区配置

```c
// 高速数据采集（大缓冲区）
#define HIGH_SPEED_RX_SIZE  4096
#define HIGH_SPEED_TX_SIZE  4096

// 低速指令通信（小缓冲区）  
#define LOW_SPEED_RX_SIZE   256
#define LOW_SPEED_TX_SIZE   256

USART_LL_Driver_t high_speed_usart;  // 用于数据采集
USART_LL_Driver_t low_speed_usart;   // 用于指令通信
```

### 多串口同时工作

```c
void Multi_USART_Example(void)
{
    // USART1: 与PC通信
    USART_LL_Put_TxData_To_Ringbuffer(&usart1_driver, 
                                      "PC: 状态正常\r\n", 15);
    
    // USART2: 与传感器通信  
    USART_LL_Put_TxData_To_Ringbuffer(&usart2_driver,
                                      "AT+READ\r\n", 9);
    
    // USART3: 与WiFi模块通信
    USART_LL_Put_TxData_To_Ringbuffer(&usart3_driver,
                                      "AT+CWJAP?\r\n", 11);
}
```

## 性能优化建议

1. **缓冲区大小**：根据数据量调整，避免过大浪费内存
2. **中断优先级**：DMA中断优先级设置要合理
3. **主循环频率**：建议1ms调用一次 `USART_LL_Module_Run()`
4. **错误监控**：定期检查错误计数器，及时发现问题

## 常见问题

**Q: 数据发送不出去？**  
A: 检查DMA通道配置是否正确，确保在主循环中调用了 `USART_LL_Module_Run()`

**Q: 接收数据丢失？**  
A: 增大RingBuffer，或者更频繁地读取数据

**Q: 多个串口冲突？**  
A: 确保每个串口使用不同的DMA通道，中断优先级设置合理

这样的设计让你可以像管理多个"邮递员"一样，每个都有自己的工具和路线，互不干扰地高效工作！ 