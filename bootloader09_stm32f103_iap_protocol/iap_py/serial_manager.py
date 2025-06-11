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
    def __init__(self, port, baudrate):
        """
        Brief: 初始化串口
        Params: port - 串口号，如COM3
                baudrate - 波特率
        """
        self.ser = serial.Serial(port, baudrate, timeout=1)

    def Send_String(self, data):
        """
        Brief: 发送字符串数据
        Params: data - 要发送的字符串
        Return: 实际发送的字节数
        """
        return self.ser.write(data.encode('utf-8'))
    
    def Send_Bytes(self, data):
        """
        Brief: 发送字节数据
        Params: data - 要发送的bytes对象
        Return: 实际发送的字节数
        """
        return self.ser.write(data)

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





