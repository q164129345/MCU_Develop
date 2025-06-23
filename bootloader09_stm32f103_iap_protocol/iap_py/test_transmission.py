#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Brief: 测试YModem传输逻辑
这个脚本用于验证传输逻辑是否正确，而不需要实际的串口连接
"""

import os
import sys
from ymodem import YModem
from bin_reader import BinReader

def test_file_chunking():
    """
    Brief: 测试文件分块逻辑
    """
    print("=== 测试文件分块逻辑 ===")
    
    # 创建YModem实例
    ymodem = YModem()
    
    # 尝试不同的文件路径
    possible_paths = [
        "firmware/App_crc.bin",           # 从iap_py目录运行
        "iap_py/firmware/App_crc.bin",    # 从项目根目录运行
    ]
    
    bin_file_path = None
    for path in possible_paths:
        if os.path.exists(path):
            bin_file_path = path
            break
    
    # 检查文件是否存在
    if bin_file_path is None:
        print(f"错误：在以下路径都找不到测试文件：")
        for path in possible_paths:
            print(f"  - {path}")
        return False
    
    # 加载文件
    if not ymodem.bin_reader.load_file(bin_file_path):
        print(f"错误：无法加载文件 {bin_file_path}")
        return False
    
    # 获取文件信息
    file_size = ymodem.bin_reader.get_file_size()
    file_name = os.path.basename(bin_file_path)
    
    print(f"文件：{file_name}")
    print(f"大小：{file_size} 字节")
    
    # 计算需要的数据包数量
    total_packets = (file_size + ymodem.PACKET_SIZE_1024 - 1) // ymodem.PACKET_SIZE_1024
    print(f"需要数据包：{total_packets} 个")
    
    # 测试每个数据包的分块
    print("\n数据包分块信息：")
    total_data_sent = 0
    
    for packet_num in range(1, total_packets + 1):
        # 计算当前包的数据偏移和大小
        data_offset = (packet_num - 1) * ymodem.PACKET_SIZE_1024
        remaining_bytes = file_size - data_offset
        chunk_size = min(ymodem.PACKET_SIZE_1024, remaining_bytes)
        
        # 读取数据（验证是否能正确读取）
        data_chunk = ymodem.bin_reader.get_data(data_offset, chunk_size)
        if data_chunk is None:
            print(f"❌ 第{packet_num}包：读取失败")
            return False
        
        # 显示包信息
        progress = (packet_num * 100) // total_packets
        padding_bytes = ymodem.PACKET_SIZE_1024 - len(data_chunk)
        
        print(f"  第{packet_num:2d}包: 偏移0x{data_offset:06X}, "
              f"数据{len(data_chunk):4d}字节, 填充{padding_bytes:3d}字节, "
              f"进度{progress:3d}%")
        
        total_data_sent += len(data_chunk)
    
    print(f"\n验证结果：")
    print(f"  原文件大小：{file_size} 字节")
    print(f"  实际传输数据：{total_data_sent} 字节")
    print(f"  数据完整性：{'✅ 正确' if total_data_sent == file_size else '❌ 错误'}")
    
    return total_data_sent == file_size

def test_packet_building():
    """
    Brief: 测试数据包构建
    """
    print("\n=== 测试数据包构建 ===")
    
    ymodem = YModem()
    
    # 测试第0包（文件信息包）
    file_name = "App_crc.bin"
    file_size = 12345
    file_info = f"{file_name}\0{file_size}\0".encode('utf-8')
    packet0 = ymodem.build_packet(0, file_info)
    
    print(f"第0包（文件信息包）：")
    print(f"  原始数据：{len(file_info)} 字节")
    print(f"  数据包：{len(packet0)} 字节")
    print(f"  数据预览：{file_info[:20]}...")
    
    # 测试普通数据包
    test_data = b"Hello, YModem!" + b"\x00" * 100  # 简单测试数据
    packet1 = ymodem.build_packet(1, test_data)
    
    print(f"\n第1包（数据包）：")
    print(f"  原始数据：{len(test_data)} 字节")
    print(f"  数据包：{len(packet1)} 字节")
    print(f"  数据预览：{test_data[:20]}...")
    
    # 验证数据包结构
    expected_packet_size = 1024 + 3 + 2  # 数据+头+CRC
    
    print(f"\n数据包结构验证：")
    print(f"  期望大小：{expected_packet_size} 字节")
    print(f"  实际大小：{len(packet1)} 字节")
    print(f"  结构正确：{'✅ 是' if len(packet1) == expected_packet_size else '❌ 否'}")
    
    return len(packet1) == expected_packet_size

def main():
    """
    Brief: 主测试函数
    """
    print("YModem传输逻辑测试")
    print("=" * 50)
    
    # 执行测试
    test1_result = test_file_chunking()
    test2_result = test_packet_building()
    
    # 总结
    print("\n" + "=" * 50)
    print("测试结果总结：")
    print(f"  文件分块测试：{'✅ 通过' if test1_result else '❌ 失败'}")
    print(f"  数据包构建测试：{'✅ 通过' if test2_result else '❌ 失败'}")
    
    if test1_result and test2_result:
        print("\n🎉 所有测试通过！代码逻辑正确。")
        return True
    else:
        print("\n❌ 部分测试失败，请检查代码。")
        return False

if __name__ == '__main__':
    success = main()
    sys.exit(0 if success else 1) 