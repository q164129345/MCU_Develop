# YModem协议STM32下位机实现（完全解耦版本）

## 概述

这是一个完全解耦的YModem协议下位机实现，专门为STM32F103系列微控制器设计。它就像一个"纯粹的包裹处理专家"，能够：

- 接收来自任何数据源的YModem格式数据（串口、网口、蓝牙、文件等）
- 验证数据包的完整性（CRC16校验）
- 将固件数据写入Flash存储器的指定区域
- 提供传输进度反馈
- 完全独立于通信接口，支持任意数据源

## 文件结构

```
IAP/
├── ymodem.h                    # YModem协议头文件
├── ymodem.c                    # YModem协议实现
├── YModem_README.md            # 本说明文件
└── (已删除旧版示例文件)
```

## 核心特性

### 1. 协议兼容性
- 完全兼容标准YModem协议
- 支持1024字节数据包（STX包头）
- 支持128字节数据包（SOH包头）
- CRC16校验确保数据完整性

### 2. 状态机设计
```
空闲状态 → 等待文件信息包 → 等待数据包 → 接收数据 → 等待结束包 → 完成
                                    ↓
                               写入Flash缓存区
```

### 3. Flash管理
- 自动擦除Flash页面
- 按4字节对齐写入数据
- 地址范围检查
- 写入到App下载缓存区（`FLASH_DL_START_ADDR`）

## 使用方法

### 1. 实际项目中的集成方式

根据当前项目的实际实现，YModem协议已经完全集成到`main.c`中：

```c
#include "ymodem.h"
#include "bsp_usart_hal.h"

// 全局变量声明
YModem_Handler_t gYModemHandler;
USART_Driver_t gUsart1Drv;

int main(void)
{
    // 系统初始化
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_DMA_Init();
    MX_USART1_UART_Init();
    
    // USART1初始化
    USART_Config(&gUsart1Drv,
                 gUsart1RXDMABuffer, gUsart1RXRBBuffer, sizeof(gUsart1RXDMABuffer),
                 gUsart1TXDMABuffer, gUsart1TXRBBuffer, sizeof(gUsart1TXDMABuffer));
    
    // YModem协议处理器初始化（完全解耦版本）
    YModem_Init(&gYModemHandler);
    
    log_printf("Bootloader init successfully.\n");
    
    uint32_t fre = 0;
    while (1) {
        // 2ms周期执行YModem协议处理
        if (0 == fre % 2) {
            // YModem协议处理 - 逐字节从ringbuffer里拿出数据来解析
            while(USART_Get_The_Existing_Amount_Of_Data(&gUsart1Drv)) {
                uint8_t data;
                if (USART_Take_A_Piece_Of_Data(&gUsart1Drv, &data)) {
                    YModem_Result_t ymodem_result = YModem_Run(&gYModemHandler, data);
                    
                    // 检查YModem传输结果
                    if (ymodem_result == YMODEM_RESULT_COMPLETE) {
                        log_printf("YModem: 固件升级完成！准备校验和复制...\r\n");
                        // 这里可以触发固件校验和复制流程
                        // 重置YModem处理器，准备下次传输
                        YModem_Reset(&gYModemHandler);
                    } else if (ymodem_result == YMODEM_RESULT_ERROR) {
                        log_printf("YModem: 传输出错，重置协议处理器\r\n");
                        YModem_Reset(&gYModemHandler);
                    }
                }
            }
            
            // 检查是否有YModem响应数据需要发送
            if (YModem_Has_Response(&gYModemHandler)) {
                uint8_t response_buffer[16];
                uint8_t response_length = YModem_Get_Response(&gYModemHandler, 
                                                             response_buffer, 
                                                             sizeof(response_buffer));
                if (response_length > 0) {
                    // 将响应数据发送给上位机
                    USART_Put_TxData_To_Ringbuffer(&gUsart1Drv, response_buffer, response_length);
                }
            }
            
            USART_Module_Run(&gUsart1Drv); // Usart1模块运行
        }
        
        fre++;
        HAL_Delay(1);
    }
}
```

### 2. 关键集成要点

1. **初始化顺序**：
   ```c
   // 1. 先初始化USART驱动
   USART_Config(&gUsart1Drv, ...);
   
   // 2. 再初始化YModem处理器
   YModem_Init(&gYModemHandler);
   ```

2. **数据处理流程**：
   ```c
   // 在2ms定时循环中处理
   while(USART_Get_The_Existing_Amount_Of_Data(&gUsart1Drv)) {
       uint8_t data;
       if (USART_Take_A_Piece_Of_Data(&gUsart1Drv, &data)) {
           YModem_Result_t result = YModem_Run(&gYModemHandler, data);
           // 处理结果...
       }
   }
   ```

3. **响应数据处理**：
   ```c
   if (YModem_Has_Response(&gYModemHandler)) {
       uint8_t response_buffer[16];
       uint8_t length = YModem_Get_Response(&gYModemHandler, response_buffer, sizeof(response_buffer));
       if (length > 0) {
           USART_Put_TxData_To_Ringbuffer(&gUsart1Drv, response_buffer, length);
       }
   }
   ```

### 3. 传输完成后的处理

当YModem传输完成后，您可以在代码中添加固件校验和复制逻辑：

```c
if (ymodem_result == YMODEM_RESULT_COMPLETE) {
    log_printf("YModem: 固件升级完成！准备校验和复制...\r\n");
    
    // 可选：添加固件校验和复制逻辑
    // if (HAL_OK == FW_Firmware_Verification(FLASH_DL_START_ADDR, FW_TOTAL_LEN)) {
    //     if (OP_FLASH_OK == OP_Flash_Copy(FLASH_DL_START_ADDR, FLASH_APP_START_ADDR, FLASH_APP_SIZE)) {
    //         log_printf("固件复制成功，准备跳转App\r\n");
    //         Delay_MS_By_NOP(500);
    //         IAP_Ready_To_Jump_App();
    //     }
    // }
    
    YModem_Reset(&gYModemHandler);
}
```

## API参考

### 主要函数

#### `YModem_Init()`
```c
YModem_Result_t YModem_Init(YModem_Handler_t *handler);
```
- **功能**: 初始化YModem协议处理器
- **参数**: `handler`: YModem处理器指针
- **返回**: 初始化结果
- **说明**: 完全独立的初始化，不依赖任何通信接口

#### `YModem_Run()`
```c
YModem_Result_t YModem_Run(YModem_Handler_t *handler, uint8_t data);
```
- **功能**: 逐字节处理YModem协议数据
- **参数**: 
  - `handler`: YModem处理器指针
  - `data`: 输入的数据字节
- **返回**: 处理结果
- **说明**: 完全解耦数据来源，支持任何数据源

#### `YModem_Reset()`
```c
void YModem_Reset(YModem_Handler_t *handler);
```
- **功能**: 重置YModem处理器状态
- **参数**: `handler`: YModem处理器指针

#### `YModem_Get_Progress()`
```c
uint8_t YModem_Get_Progress(YModem_Handler_t *handler);
```
- **功能**: 获取传输进度
- **参数**: `handler`: YModem处理器指针
- **返回**: 进度百分比（0-100）

#### `YModem_Has_Response()`
```c
uint8_t YModem_Has_Response(YModem_Handler_t *handler);
```
- **功能**: 检查是否有响应数据需要发送
- **参数**: `handler`: YModem处理器指针
- **返回**: 1-有响应数据，0-无响应数据

#### `YModem_Get_Response()`
```c
uint8_t YModem_Get_Response(YModem_Handler_t *handler, uint8_t *buffer, uint8_t max_length);
```
- **功能**: 获取响应数据
- **参数**: 
  - `handler`: YModem处理器指针
  - `buffer`: 存储响应数据的缓冲区
  - `max_length`: 缓冲区最大长度
- **返回**: 实际获取的响应数据长度
- **说明**: 获取后会自动清除响应数据缓冲区

### 返回值说明

```c
typedef enum {
    YMODEM_RESULT_OK = 0,           // 处理成功
    YMODEM_RESULT_CONTINUE,         // 继续处理
    YMODEM_RESULT_NEED_MORE_DATA,   // 需要更多数据
    YMODEM_RESULT_ERROR,            // 处理错误
    YMODEM_RESULT_COMPLETE,         // 传输完成
} YModem_Result_t;
```

## 协议流程

### 1. 上位机发送流程
```
上位机 → 第0包（文件信息）→ 下位机
下位机 → ACK + 'C' → 上位机
上位机 → 第1包（数据）→ 下位机
下位机 → ACK → 上位机
...（重复数据包传输）
上位机 → EOT → 下位机
下位机 → ACK → 上位机
上位机 → 结束包（空文件名）→ 下位机
下位机 → ACK → 上位机
```

### 2. 数据包格式
```
STX(0x02) | 包序号 | 包序号取反 | 1024字节数据 | CRC16高字节 | CRC16低字节
```

## 内存映射

根据`flash_map.h`的定义：

```c
#define FLASH_DL_START_ADDR    0x08040000U  // App下载缓存区起始地址
#define FLASH_DL_END_ADDR      0x0806FFFFU  // App下载缓存区结束地址
#define FLASH_DL_SIZE          (192 * 1024) // 缓存区大小：192KB
```

## 错误处理

### 1. CRC校验失败
- 发送NAK给上位机
- 要求重传当前包

### 2. 包序号错误
- 发送NAK给上位机
- 要求重传当前包

### 3. Flash写入失败
- 发送CAN取消传输
- 进入错误状态

### 4. 地址超出范围
- 发送CAN取消传输
- 记录错误日志

## 调试信息

通过RTT输出详细的调试信息：

```
YModem: 协议处理器初始化完成
YModem: 文件信息 - 名称: App_crc.bin, 大小: 12345 字节
YModem: 进度 10% (1024/12345 字节)
YModem: 成功写入 1024 字节到地址 0x08040000
YModem: 传输完成！总共接收 12345 字节
```

## 注意事项

### 1. Flash写入限制
- STM32F103的Flash写入最小单位是4字节（1个字）
- 写入前必须先擦除对应的Flash页面
- 每页大小为2KB

### 2. 中断安全
- YModem_Run()函数设计为非阻塞式
- 可以在主循环或定时器中安全调用
- 不会影响其他中断处理

### 3. 内存使用
- YModem_Handler_t结构体大小约2KB
- 主要用于接收缓冲区和数据包解析

### 4. 兼容性
- 与标准YModem协议完全兼容
- 可以与任何支持YModem的上位机软件配合使用

## 完整使用示例

### 当前项目实现

当前项目中的YModem协议已经完全集成到bootloader的main.c中，实现了：

1. **串口数据源**: 从USART1 ringbuffer逐字节读取YModem数据
2. **实时处理**: 在2ms定时循环中处理接收到的数据
3. **自动响应**: 自动处理ACK/NAK响应并发送给上位机
4. **进度监控**: 通过RTT输出详细的传输进度信息
5. **错误处理**: 自动重置协议处理器，支持重新传输

### 多种数据源支持（扩展能力）

完全解耦的设计使得YModem协议可以支持任何数据源：

- **串口**: 从USART ringbuffer逐字节读取（当前实现）
- **网口**: 从以太网接收缓冲区读取（可扩展）
- **蓝牙**: 从蓝牙接收缓冲区读取（可扩展）
- **文件**: 从文件系统读取（用于测试，可扩展）
- **CAN总线**: 从CAN接收缓冲区读取（可扩展）
- **USB**: 从USB接收缓冲区读取（可扩展）

### 扩展到其他数据源的方法

如果需要支持其他数据源，只需要：

```c
// 示例：从CAN总线接收YModem数据
while(CAN_Get_Received_Data_Count() > 0) {
    uint8_t data;
    if (CAN_Get_One_Byte(&data)) {
        YModem_Result_t result = YModem_Run(&gYModemHandler, data);
        // 处理结果...
    }
}

// 示例：从网口接收YModem数据  
while(Ethernet_Has_Data()) {
    uint8_t data;
    if (Ethernet_Read_Byte(&data)) {
        YModem_Result_t result = YModem_Run(&gYModemHandler, data);
        // 处理结果...
    }
}
```

## 技术支持

### 常见问题排查

如果遇到问题，请按以下顺序检查：

1. **串口通信**：
   - 确认USART1配置正确（波特率、数据位、停止位等）
   - 检查DMA配置是否正常
   - 验证ringbuffer是否正常工作

2. **YModem协议**：
   - 检查上位机发送的YModem格式是否正确
   - 确认CRC16校验算法一致
   - 验证数据包大小设置（1024字节）

3. **Flash操作**：
   - 确认Flash地址映射配置正确
   - 检查Flash擦除和写入权限
   - 验证地址范围是否在有效区域内

4. **调试信息**：
   - 确保RTT配置正确，能正常输出日志
   - 检查log_printf函数是否正常工作
   - 观察YModem传输过程中的详细日志

### 调试建议

1. **使用RTT查看详细日志**：
   ```
   YModem: 协议处理器初始化完成
   YModem: 文件信息 - 名称: App_crc.bin, 大小: 12345 字节
   YModem: 进度 10% (1024/12345 字节)
   YModem: 成功写入 1024 字节到地址 0x08040000
   YModem: 传输完成！总共接收 12345 字节
   ```

2. **检查传输状态**：
   - 观察YModem_Run()的返回值
   - 监控YMODEM_RESULT_COMPLETE和YMODEM_RESULT_ERROR状态
   - 确认响应数据是否正确发送给上位机

3. **验证数据完整性**：
   - 可以添加额外的CRC32校验
   - 比较接收到的文件大小与预期大小
   - 检查Flash中写入的数据是否正确

### 性能优化建议

1. **提高传输效率**：
   - 保持2ms的处理周期，避免数据积压
   - 确保USART DMA缓冲区足够大
   - 优化Flash写入策略

2. **错误恢复**：
   - 利用YModem的自动重试机制
   - 在传输错误时及时重置协议处理器
   - 记录错误统计信息用于分析

---

*本实现基于STM32 HAL库，专门为STM32F103系列微控制器优化，已在实际项目中验证可用性。* 