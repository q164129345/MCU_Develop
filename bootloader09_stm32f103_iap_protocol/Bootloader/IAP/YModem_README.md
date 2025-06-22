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
├── ymodem_usage_example.c      # 完全解耦版使用示例
├── ymodem_example.c            # 旧版使用示例（已废弃）
└── YModem_README.md            # 本说明文件
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

### 1. 基本初始化（完全解耦版本）

```c
#include "ymodem.h"
#include "bsp_usart_hal.h"  // 仅在使用串口时需要

// 声明YModem处理器实例
YModem_Handler_t gYModemHandler;

// 在main函数中初始化
int main(void)
{
    // ... 其他初始化代码 ...
    
    // 初始化USART1（如果使用串口数据源）
    USART_Config(&gUsart1Drv, ...);
    
    // 初始化YModem协议处理器（完全独立）
    YModem_Init(&gYModemHandler);
    
    // 主循环
    while (1) {
        // 从数据源逐字节读取数据
        while (USART_Get_The_Existing_Amount_Of_Data(&gUsart1Drv) > 0) {
            uint8_t data;
            if (USART_Take_A_Piece_Of_Data(&gUsart1Drv, &data)) {
                // 将数据传递给YModem处理器
                YModem_Result_t result = YModem_Run(&gYModemHandler, data);
                
                if (result == YMODEM_RESULT_COMPLETE) {
                    log_printf("固件接收完成！\r\n");
                    YModem_Reset(&gYModemHandler);
                }
            }
        }
        
        // 检查并发送响应数据
        if (YModem_Has_Response(&gYModemHandler)) {
            uint8_t response_buffer[16];
            uint8_t response_length = YModem_Get_Response(&gYModemHandler, 
                                                         response_buffer, 
                                                         sizeof(response_buffer));
            if (response_length > 0) {
                // 发送响应数据（这里使用串口）
                USART_Put_TxData_To_Ringbuffer(&gUsart1Drv, response_buffer, response_length);
            }
        }
        
        HAL_Delay(1);
    }
}
```

### 2. 在现有main.c中集成（完全解耦版本）

已经在`main.c`中集成了完全解耦的YModem处理：

```c
//! 2ms
if (0 == fre % 2) {
    //! YModem协议处理 - 逐字节从ringbuffer里拿出数据来解析
    while(USART_Get_The_Existing_Amount_Of_Data(&gUsart1Drv)) {
        uint8_t data;
        if (USART_Take_A_Piece_Of_Data(&gUsart1Drv, &data)) {
            YModem_Result_t ymodem_result = YModem_Run(&gYModemHandler, data);
            
            //! 检查YModem传输结果
            if (ymodem_result == YMODEM_RESULT_COMPLETE) {
                log_printf("YModem: 固件升级完成！准备校验和复制...\r\n");
                YModem_Reset(&gYModemHandler);
            } else if (ymodem_result == YMODEM_RESULT_ERROR) {
                log_printf("YModem: 传输出错，重置协议处理器\r\n");
                YModem_Reset(&gYModemHandler);
            }
        }
    }
    
    //! 检查是否有YModem响应数据需要发送
    if (YModem_Has_Response(&gYModemHandler)) {
        uint8_t response_buffer[16];
        uint8_t response_length = YModem_Get_Response(&gYModemHandler, response_buffer, sizeof(response_buffer));
        if (response_length > 0) {
            USART_Put_TxData_To_Ringbuffer(&gUsart1Drv, response_buffer, response_length);
        }
    }
    
    USART_Module_Run(&gUsart1Drv);
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

参考`ymodem_usage_example.c`文件中的完整示例代码，包括：

1. 串口数据源使用方法
2. 文件数据源使用方法（模拟）
3. 网口/蓝牙数据源使用方法（预留）
4. 通用的固件升级流程
5. 进度显示功能

### 多种数据源支持

完全解耦的设计使得YModem协议可以支持任何数据源：

- **串口**: 从USART ringbuffer逐字节读取
- **网口**: 从以太网接收缓冲区读取
- **蓝牙**: 从蓝牙接收缓冲区读取
- **文件**: 从文件系统读取（用于测试）
- **CAN总线**: 从CAN接收缓冲区读取
- **USB**: 从USB接收缓冲区读取

## 技术支持

如果遇到问题，请检查：

1. 串口配置是否正确
2. Flash地址映射是否合理
3. 是否正确初始化了所有依赖模块
4. RTT日志输出是否正常

---

*本实现基于STM32 HAL库，适用于STM32F103系列微控制器。* 