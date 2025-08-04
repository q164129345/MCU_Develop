# -*- coding: utf-8 -*-
# Tested with Python 3.13.3

import serial
import sys
import time

class SerialManager:
    """
    Brief: 串口管理类，负责串口初始化和数据收发
    Params: port 串口号，如COM3
            baudrate 波特率
    """
    def __init__(self, port, baudrate, timeout=1):
        """
        Brief: 初始化串口
        Params: port - 串口号，如COM3
                baudrate - 波特率
                timeout - 读取超时时间（秒），默认1秒
        """
        # 明确设置串口参数，确保与串口助手一致
        self.ser = serial.Serial(
            port=port,
            baudrate=baudrate,
            bytesize=serial.EIGHTBITS,    # 8数据位
            parity=serial.PARITY_NONE,    # 无校验
            stopbits=serial.STOPBITS_ONE, # 1停止位
            timeout=timeout,
            xonxoff=False,                # 禁用软件流控
            rtscts=False,                 # 禁用硬件流控
            dsrdtr=False                  # 禁用DTR/DSR
        )
        
        # 关键修复：避免DTR/RTS信号干扰MCU
        # 很多MCU的复位线连接到DTR，设置为False避免意外复位
        self.ser.dtr = False  # 避免触发MCU复位
        self.ser.rts = False  # 避免干扰MCU
        
        # 清空缓冲区
        self.ser.reset_input_buffer()
        self.ser.reset_output_buffer()

    def Send_String(self, data):
        """
        Brief: 发送字符串数据
        Params: data - 要发送的字符串
        Return: 实际发送的字节数
        """
        bytes_sent = self.ser.write(data.encode('utf-8'))
        self.ser.flush()  # 强制刷新发送缓冲区，确保数据立即发送
        return bytes_sent
    
    def Send_Bytes(self, data):
        """
        Brief: 发送字节数据
        Params: data - 要发送的bytes对象
        Return: 实际发送的字节数
        """
        bytes_sent = self.ser.write(data)
        self.ser.flush()  # 强制刷新发送缓冲区，确保数据立即发送
        return bytes_sent

    def Receive_Byte(self, timeout=None):
        """
        Brief: 接收单个字节
        Params: timeout - 超时时间（秒），None表示使用默认超时
        Return: 接收到的字节值(int)，超时返回None
        """
        if timeout is not None:
            old_timeout = self.ser.timeout
            self.ser.timeout = timeout
        
        try:
            data = self.ser.read(1)
            if len(data) == 1:
                return data[0]  # 返回字节值
            else:
                return None  # 超时或无数据
        finally:
            if timeout is not None:
                self.ser.timeout = old_timeout

    def Receive_Bytes(self, count, timeout=None):
        """
        Brief: 接收指定数量的字节
        Params: 
            count - 要接收的字节数
            timeout - 超时时间（秒），None表示使用默认超时
        Return: 
            bytes - 接收到的字节数据，长度可能小于count（超时情况）
        """
        if timeout is not None:
            old_timeout = self.ser.timeout
            self.ser.timeout = timeout
        
        try:
            return self.ser.read(count)
        finally:
            if timeout is not None:
                self.ser.timeout = old_timeout

    def Available(self):
        """
        Brief: 获取接收缓冲区中可读的字节数
        Return: int - 可读字节数
        """
        return self.ser.in_waiting

    def Clear_Buffer(self):
        """
        Brief: 清空接收缓冲区
        """
        self.ser.reset_input_buffer()

    def close(self):
        """
        Brief: 关闭串口
        """
        self.ser.close()

# 当直接运行serial_manager.py时执行以下代码
if __name__ == "__main__":
    # 检查命令行参数
    if len(sys.argv) < 3:
        print("用法: python3 serial_manager.py <串口> <波特率>")
        print("示例: python3 serial_manager.py COM3 115200")
        sys.exit(1)
    
    # 从命令行获取串口和波特率
    port = sys.argv[1]
    baudrate = int(sys.argv[2])
    
    try:
        # 创建SerialManager实例
        print(f"正在连接串口 {port}，波特率 {baudrate}...")
        serial_mgr = SerialManager(port, baudrate)
        
        # 发送测试字符串
        test_str = "Hello,World!"
        print(f"发送字符串: '{test_str}'")
        bytes_sent = serial_mgr.Send_String(test_str)
        print(f"成功发送 {bytes_sent} 字节")

        # 等待一秒，确保数据发送完成
        time.sleep(1)
        
        # 关闭串口
        serial_mgr.close()
        print("串口已关闭")
        
    except Exception as e:
        print(f"错误: {str(e)}")
        sys.exit(1)





