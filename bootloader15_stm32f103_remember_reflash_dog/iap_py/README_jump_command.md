# IAP跳转命令使用说明 - 心跳版本 💓

## 重大改进 🎉

经过你的建议，我们为bootloader添加了**主动心跳功能**！现在bootloader会每2秒主动发送'C'字符，告知上位机它已准备就绪。

### ✅ 新功能优势：
- **主动通知** - bootloader主动告知上位机何时准备好
- **精确时机** - 无需盲等，收到心跳立即开始传输
- **提高可靠性** - 消除时间估算不准确的问题
- **用户友好** - 实时反馈，提升用户体验
- **符合标准** - 遵循YModem协议的最佳实践

## 改进后的IAP流程

```
1. 上位机发送"A5A5A5A5" 
   ↓
2. MCU App程序检测到命令，跳转到bootloader
   ↓
3. bootloader启动，开始每2秒发送'C'字符心跳 💓
   ↓
4. 上位机收到'C'字符，确认bootloader准备就绪 ✅
   ↓
5. 上位机立即发送YModem第0包（文件信息包）
   ↓
6. bootloader停止发送心跳，开始正常YModem传输
```

## 使用方法

### 方法一：自动模式（推荐）

现在自动模式更智能，会等待心跳确认：

```bash
# 自动跳转+固件传输（智能心跳检测）
python main.py --port COM3 --auto-jump --verbose

# 指定固件文件
python main.py --port COM3 --auto-jump --file firmware/App_crc.bin
```

### 方法二：分步骤操作

```bash
# 步骤1：发送跳转命令并等待心跳
python jump_command.py --port COM3 --verbose

# 步骤2：收到心跳后立即发送固件
python main.py --port COM3 --file firmware/App_crc.bin
```

## bootloader端实现

### 新增变量
```c
//! 添加定期发送'C'字符的功能，告知上位机bootloader已就绪
#define IAP_HEARTBEAT_INTERVAL_MS   2000    // 每2秒发送一次'C'字符
static uint32_t iap_heartbeat_counter = 0;
static bool iap_heartbeat_enabled = true;  // 默认启用心跳
```

### 心跳发送逻辑
```c
//! IAP心跳处理 - 每1ms执行
if (iap_heartbeat_enabled) {
    iap_heartbeat_counter++;
    
    //! 每2秒发送一次'C'字符，告知上位机bootloader已就绪
    if (iap_heartbeat_counter >= IAP_HEARTBEAT_INTERVAL_MS) {
        //! 只在等待文件信息状态时发送心跳
        if (gYModemHandler.state == YMODEM_STATE_WAIT_FILE_INFO) {
            uint8_t heartbeat_char = 0x43; // 'C'字符
            USART_Put_TxData_To_Ringbuffer(&gUsart1Drv, &heartbeat_char, 1);
            log_printf("IAP heartbeat: sent 'C' to notify host - bootloader ready.\r\n");
        }
        iap_heartbeat_counter = 0; // 重置心跳计数器
    }
}
```

### 智能控制机制
- **启动时开启** - bootloader启动后立即开始发送心跳
- **传输时关闭** - 一旦开始接收YModem数据包，自动停止心跳
- **错误时重启** - 传输错误或CRC验证失败后，重新启用心跳
- **状态感知** - 只在等待文件信息状态时发送心跳

## 命令行参数

### main.py参数
- `--port` - 串口号（必需），如COM3
- `--baud` - 波特率，默认115200
- `--file` - 固件文件路径，默认firmware/App_crc.bin
- `--auto-jump` - **自动发送跳转命令并等待心跳**
- `--timeout` - 等待ACK超时时间，默认5秒
- `--retries` - 重试次数，默认3次
- `--verbose` - 显示详细信息

### jump_command.py参数
- `--port` - 串口号（必需），如COM3
- `--baud` - 波特率，默认115200
- `--timeout` - **等待心跳超时时间，默认10秒**
- `--verbose` - 显示详细信息

## 故障排除

### 问题1：未收到'C'字符心跳
**检查项目：**
- bootloader是否已添加心跳功能代码
- 心跳发送间隔是否正确（2秒）
- 串口连接是否正常
- MCU是否成功跳转到bootloader

### 问题2：收到心跳但YModem传输失败
**检查项目：**
- 心跳停止机制是否正常工作
- YModem状态机是否正确切换
- 固件文件是否存在且完整

### 问题3：传输过程中断
**检查项目：**
- 心跳是否在开始传输后正确停止
- 是否有多余的'C'字符干扰传输
- 错误恢复机制是否重新启用心跳

## 示例输出

### 成功的心跳检测输出：
```
=== 自动跳转到bootloader ===
发送跳转命令... 完成(8字节)
等待'C'心跳... 收到：0x43
✅ 收到'C'字符心跳！bootloader已准备就绪
收到bootloader心跳，开始YModem传输...
文件信息：App_crc.bin, 大小：15234 字节
将分为 15 个数据包传输
发送第0包（文件信息包）... 完成(1029字节)
等待ACK+C... 完成
发送第1/15包 [6%]... 完成(1029字节)
...
🎉 YModem完整传输流程完成！
```

### 详细模式的心跳检测：
```
=== 发送跳转命令 ===
命令内容：'A5A5A5A5'
ASCII码：0x41 0x35 0x41 0x35 0x41 0x35 0x41 0x35
成功发送 8 字节
💡 新功能：现在bootloader会主动发送'C'字符心跳
等待bootloader发送'C'字符心跳...
bootloader每2秒发送一次'C'字符，超时时间：6秒
收到：0x43
✅ 收到'C'字符心跳！bootloader已准备就绪
```

## 技术细节

### 心跳时序
- **发送间隔** - 每2000ms发送一次'C'字符
- **发送条件** - 仅在YMODEM_STATE_WAIT_FILE_INFO状态
- **停止条件** - 收到任何YModem数据包后自动停止
- **重启条件** - 传输错误或重置时重新启用

### 上位机检测逻辑
- **超时时间** - 默认6秒（足够收到3次心跳）
- **响应速度** - 收到第一个'C'字符立即开始传输
- **错误处理** - 超时后提供详细的故障排除建议

### 兼容性
- **向后兼容** - 如果bootloader未添加心跳功能，会超时但不影响后续传输
- **协议标准** - 'C'字符是YModem协议标准的开始字符
- **干扰最小** - 心跳只在空闲状态发送，不干扰正常传输

---

**🎉 恭喜！** 通过添加心跳功能，你的IAP系统现在具备了**主动状态通知**能力，大大提高了系统的可靠性和用户体验！ 