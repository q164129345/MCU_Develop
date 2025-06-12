# -*- coding: utf-8 -*-
# Tested with Python 3.13.3

import os
import time
import struct
from bin_reader import BinReader
from serial_manager import SerialManager

class YModem:
    """
    Brief: YModem协议实现，用于通过串口向STM32发送固件
    """
    # YModem协议常量
    SOH = 0x01  # 128字节数据包开始标记
    STX = 0x02  # 1024字节数据包开始标记
    EOT = 0x04  # 传输结束标记
    ACK = 0x06  # 应答，接收正确
    NAK = 0x15  # 否定应答，接收错误
    CAN = 0x18  # 取消传输
    C = 0x43    # ASCII 'C'，表示CRC校验方式

    # 数据包大小
    PACKET_SIZE_128 = 128
    PACKET_SIZE_1024 = 1024
    
    def __init__(self, serial_manager):
        """
        Brief: 初始化YModem协议处理器
        Params:
            serial_manager: SerialManager实例，用于串口通信
        """
        self.serial = serial_manager
        self.bin_reader = BinReader()
    
    def _calculate_crc(self, data):
        """
        Brief: 计算CRC-16校验值
        Params:
            data: 要计算校验值的字节数据
        Return:
            int: 16位CRC校验值
        """
        # CRC-16/XMODEM算法
        crc = 0x0000
        for byte in data:
            crc ^= (byte << 8)
            for _ in range(8):
                if crc & 0x8000:
                    crc = ((crc << 1) ^ 0x1021) & 0xFFFF
                else:
                    crc = (crc << 1) & 0xFFFF
        return crc
    
    def _build_packet(self, packet_num, data):
        """
        Brief: 构建YModem数据包
        Params:
            packet_num: 数据包序号(0-255)
            data: 要发送的数据
        Return:
            bytes: 构建好的数据包
        """
        # 确定包类型和大小
        if len(data) <= self.PACKET_SIZE_128:
            packet_header = self.SOH
            packet_size = self.PACKET_SIZE_128
        else:
            packet_header = self.STX
            packet_size = self.PACKET_SIZE_1024
        
        # 如果数据不足，根据包类型选择填充字符
        if len(data) < packet_size:
            # 第0包（起始帧）用0x00填充，其他包用0x1A填充
            padding_byte = b'\x00' if packet_num == 0 else b'\x1A'
            data = data + padding_byte * (packet_size - len(data))
        elif len(data) > packet_size:
            data = data[:packet_size]  # 截断到指定大小
        
        # 构建数据包：起始标记 + 包序号 + 包序号取反 + 数据 + CRC校验
        packet = struct.pack('>B', packet_header)  # 起始标记(SOH或STX)
        packet += struct.pack('>B', packet_num)    # 包序号
        packet += struct.pack('>B', 255 - packet_num)  # 包序号取反
        packet += data  # 数据部分
        
        # 计算并添加CRC校验
        crc = self._calculate_crc(data)
        packet += struct.pack('>H', crc)  # 两字节CRC校验值
        
        return packet
    
    def _send_packet(self, packet):
        """
        Brief: 发送数据包并等待响应
        Params:
            packet: 要发送的数据包
        Return:
            bool: 是否发送成功并收到正确响应
        """
        # 发送数据包
        self.serial.Send_Bytes(packet)
        
        # 等待接收响应
        start_time = time.time()
        timeout = 5  # 5秒超时
        
        while (time.time() - start_time) < timeout:
            if self.serial.ser.in_waiting > 0:
                response = self.serial.ser.read(1)[0]  # 读取一个字节
                
                if response == self.ACK:
                    return True
                elif response == self.NAK:
                    print("警告: 收到NAK，数据包被拒绝")
                    return False
                elif response == self.CAN:
                    print("错误: 传输被取消")
                    return False
            
            time.sleep(0.01)  # 短暂延时，避免CPU占用过高
        
        print("错误: 等待响应超时")
        return False
    
    def _wait_for_c(self, timeout=10):
        """
        Brief: 等待接收方发送'C'字符，表示准备好接收数据
        Params:
            timeout: 超时时间(秒)
        Return:
            bool: 是否收到'C'字符
        """
        start_time = time.time()
        
        while (time.time() - start_time) < timeout:
            if self.serial.ser.in_waiting > 0:
                response = self.serial.ser.read(1)[0]
                if response == self.C:
                    return True
            time.sleep(0.1)
        
        print("错误: 等待接收方准备信号超时")
        return False
    
    def send_file(self, file_path):
        """
        Brief: 使用YModem协议发送文件
        Params:
            file_path: 要发送的文件路径
        Return:
            bool: 是否成功发送
        """
        # 加载文件
        if not self.bin_reader.load_file(file_path):
            return False
        
        file_size = self.bin_reader.get_file_size()
        file_name = os.path.basename(file_path)
        
        print(f"准备发送文件: {file_name}, 大小: {file_size} 字节")
        
        # 等待接收方准备好
        if not self._wait_for_c():
            return False
        
        # 发送文件名数据包(第0个包)
        file_info = f"{file_name}\0{file_size}\0".encode('utf-8')
        filename_packet = self._build_packet(0, file_info)
        if not self._send_packet(filename_packet):
            print("错误: 发送文件信息失败")
            return False
        
        # 等待接收方准备好接收数据
        if not self._wait_for_c():
            return False
        
        # 发送文件数据
        packet_num = 1
        bytes_sent = 0
        
        while bytes_sent < file_size:
            # 每次读取1024字节
            chunk_size = self.PACKET_SIZE_1024
            data = self.bin_reader.get_data(bytes_sent, chunk_size)
            
            if not data:
                print("错误: 读取文件数据失败")
                return False
            
            # 构建并发送数据包
            data_packet = self._build_packet(packet_num, data)
            if not self._send_packet(data_packet):
                print(f"错误: 发送数据包 {packet_num} 失败")
                return False
            
            bytes_sent += len(data)
            packet_num = (packet_num + 1) % 256  # 包序号循环使用0-255
            
            # 显示进度
            progress = min(100, int(bytes_sent * 100 / file_size))
            print(f"\r发送进度: {progress}% ({bytes_sent}/{file_size} 字节)", end="")
        
        print("\n所有数据包发送完成")
        
        # 发送传输结束标记
        self.serial.Send_Bytes(bytes([self.EOT]))
        
        # 等待接收方的最终确认
        start_time = time.time()
        timeout = 5
        
        while (time.time() - start_time) < timeout:
            if self.serial.ser.in_waiting > 0:
                response = self.serial.ser.read(1)[0]
                if response == self.ACK:
                    print("文件传输成功完成")
                    return True
            time.sleep(0.1)
        
        print("错误: 等待最终确认超时")
        return False
    
    def send_empty_packet(self):
        """
        Brief: 发送空数据包，表示没有更多文件要传输
        Return:
            bool: 是否成功发送
        """
        # 等待接收方准备好
        if not self._wait_for_c():
            return False
        
        # 发送空文件名数据包，表示传输结束
        empty_packet = self._build_packet(0, b'\0')
        return self._send_packet(empty_packet)

# 当直接运行ymodem.py时执行以下代码
if __name__ == "__main__":
    import sys
    
    # 检查命令行参数
    if len(sys.argv) < 3:
        print("用法: python3 ymodem.py <串口> <固件文件>")
        print("示例: python3 ymodem.py COM3 firmware.bin")
        sys.exit(1)
    
    port = sys.argv[1]
    firmware_file = sys.argv[2]
    
    try:
        # 创建SerialManager实例
        serial_mgr = SerialManager(port, 115200)  # YModem通常使用115200波特率
        
        # 创建YModem实例
        ymodem = YModem(serial_mgr)
        
        # 发送固件文件
        if ymodem.send_file(firmware_file):
            # 发送空数据包表示传输完成
            ymodem.send_empty_packet()
            print("固件传输完成")
        else:
            print("固件传输失败")
        
        # 关闭串口
        serial_mgr.close()
        
    except Exception as e:
        print(f"错误: {str(e)}")
        sys.exit(1)
