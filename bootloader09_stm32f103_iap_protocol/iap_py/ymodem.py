# -*- coding: utf-8 -*-
# Tested with Python 3.13.3

import os
import struct
from bin_reader import BinReader

class YModem:
    """
    Brief: YModem协议实现，专注于数据包的构建与解析
    """
    # YModem协议常量
    SOH = 0x01  # 128字节数据包开始标记
    STX = 0x02  # 1024字节数据包开始标记
    EOT = 0x04  # 传输结束标记
    ACK = 0x06  # 应答，接收正确
    NAK = 0x15  # 否定应答，接收错误
    CAN = 0x18  # 取消传输
    C = 0x43    # ASCII 'C'，表示CRC校验方式

    # 响应状态码
    STATUS_OK = 0       # 成功
    STATUS_NAK = 1      # 需要重传
    STATUS_CANCEL = 2   # 传输取消
    STATUS_TIMEOUT = 3  # 超时
    STATUS_SUCCESS = 4  # 升级成功

    # 数据包大小
    PACKET_SIZE_128 = 128
    PACKET_SIZE_1024 = 1024
    
    def __init__(self):
        """
        Brief: 初始化YModem协议处理器
        """
        self.bin_reader = BinReader()
    
    def calculate_crc(self, data):
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
    
    def build_packet(self, packet_num, data):
        """
        Brief: 构建YModem数据包
        Params:
            packet_num: 数据包序号(0-255)
            data: 要发送的数据
        Return:
            bytes: 构建好的数据包
        """
        # 统一使用STX作为包头，数据包大小始终为1024字节
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
        packet = struct.pack('>B', packet_header)  # 起始标记(统一使用STX)
        packet += struct.pack('>B', packet_num)    # 包序号
        packet += struct.pack('>B', 255 - packet_num)  # 包序号取反
        packet += data  # 数据部分
        
        # 计算并添加CRC校验
        crc = self.calculate_crc(data)
        packet += struct.pack('>H', crc)  # 两字节CRC校验值
        
        return packet
    
    def parse_response(self, response_byte):
        """
        Brief: 解析接收方的响应
        Params:
            response_byte: 接收到的响应字节
        Return:
            int: 状态码
        """
        if response_byte == self.ACK:
            return self.STATUS_OK
        elif response_byte == self.NAK:
            return self.STATUS_NAK
        elif response_byte == self.CAN:
            return self.STATUS_CANCEL
        else:
            # 其他字节可能是特定的成功指示等
            return self.STATUS_TIMEOUT
    
    def get_eot_packet(self):
        """
        Brief: 获取传输结束标记包
        Return:
            bytes: EOT字节
        """
        return bytes([self.EOT])
    
    def build_end_packet(self):
        """
        Brief: 构建YModem的结束帧,表示传输结束
        Return:
            bytes: 结束帧数据包
        """
        # 发送空文件名数据包，表示传输结束
        return self.build_packet(0, b'\0')

# 当直接运行ymodem.py时执行以下代码
if __name__ == "__main__":
    # 创建YModem实例和BinReader实例
    ymodem = YModem()
    bin_file_path = "firmware/App_crc.bin"
    
    # 加载二进制文件
    if not ymodem.bin_reader.load_file(bin_file_path):
        print(f"错误：无法加载文件 {bin_file_path}")
        exit(1)
    
    # 获取文件信息
    file_size = ymodem.bin_reader.get_file_size()
    file_name = os.path.basename(bin_file_path)
    
    # 打印文件信息
    print(f"\n文件信息：{file_name}, 大小：{file_size} 字节")
    
    # 创建第0包(文件信息包)
    file_info = f"{file_name}\0{file_size}\0".encode('utf-8')
    packet0 = ymodem.build_packet(0, file_info)
    
    # 创建第1包(数据包)，从文件中读取前1024字节
    data_chunk = ymodem.bin_reader.get_data(0, ymodem.PACKET_SIZE_1024)
    packet1 = ymodem.build_packet(1, data_chunk)
    
    # 计算最后一包的位置和大小
    last_packet_offset = (file_size // ymodem.PACKET_SIZE_1024) * ymodem.PACKET_SIZE_1024
    last_packet_size = file_size - last_packet_offset
    
    # 如果文件大小正好是1024的倍数，最后一包就是最后1024字节
    if last_packet_size == 0 and file_size > 0:
        last_packet_offset = file_size - ymodem.PACKET_SIZE_1024
        last_packet_size = ymodem.PACKET_SIZE_1024
    
    # 创建最后一包
    last_packet_num = (last_packet_offset // ymodem.PACKET_SIZE_1024) + 1
    last_data_chunk = ymodem.bin_reader.get_data(last_packet_offset, last_packet_size)
    last_packet = ymodem.build_packet(last_packet_num, last_data_chunk)
    
    # 打印第0包(文件信息包)
    print("第0包(文件信息包):")
    hex_packet0 = " ".join([f"{b:02X}" for b in packet0])
    print(hex_packet0)
    print(f"第0包长度: {len(packet0)-5} 字节数据 + 5 字节头尾 = {len(packet0)} 字节")  # 减去包头(1)+序号(1)+序号取反(1)+CRC(2)=5字节
    
    # 打印第1包(数据包)
    print("\n第1包(数据包):")
    hex_packet1 = " ".join([f"{b:02X}" for b in packet1])
    print(hex_packet1)
    print(f"第1包长度: {len(packet1)-5} 字节数据 + 5 字节头尾 = {len(packet1)} 字节")
    
    # 打印最后一包(数据包)
    print(f"\n最后一包(第{last_packet_num}包):")
    print(f"偏移量: {last_packet_offset}, 实际数据大小: {last_packet_size} 字节")
    hex_last_packet = " ".join([f"{b:02X}" for b in last_packet])
    print(hex_last_packet)
    print(f"最后一包长度: {len(last_packet)-5} 字节数据 + 5 字节头尾 = {len(last_packet)} 字节")
    
    # 打印最后一包数据的填充情况
    if last_packet_size < ymodem.PACKET_SIZE_1024:
        padding_start = min(20, last_packet_size)  # 显示最多20个字节的实际数据
        padding_bytes = min(20, ymodem.PACKET_SIZE_1024 - last_packet_size)  # 显示最多20个填充字节
        
        # 显示最后一包的部分实际数据
        actual_data = " ".join([f"{b:02X}" for b in last_data_chunk[-padding_start:]])
        print(f"\n最后一包末尾{padding_start}个实际数据字节: {actual_data}")
        
        # 显示填充部分的开始
        padding_preview = " ".join([f"{b:02X}" for b in last_packet[3+last_packet_size:3+last_packet_size+padding_bytes]])
        print(f"填充部分开始{padding_bytes}个字节(应为0x1A): {padding_preview}")

