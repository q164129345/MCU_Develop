from serial_manager import SerialManager
from ymodem import YModem
from bin_reader import BinReader
import argparse
import time
import os

def parse_args():
    """
    Brief: 解析命令行参数
    Return: 解析后的参数对象
    """
    parser = argparse.ArgumentParser(description="通过串口发送Ymodem协议数据包")
    parser.add_argument('--port', required=True, help='串口号，如COM3')
    parser.add_argument('--baud', type=int, default=115200, help='波特率，默认115200')
    parser.add_argument('--file', default='firmware/App_crc.bin', help='要发送的二进制文件路径')
    parser.add_argument('--timeout', type=int, default=5, help='等待ACK的超时时间（秒），默认5秒')
    return parser.parse_args()

def wait_for_ack_and_c(serial_mgr, timeout=5):
    """
    Brief: 等待接收ACK和C字符（用于第0包后的响应）
    Params: 
        serial_mgr - 串口管理器实例
        timeout - 超时时间（秒）
    Return: 
        bool - 是否同时收到ACK和C
    """
    print(f"等待ACK+C应答（超时时间：{timeout}秒）...")
    
    start_time = time.time()
    received_responses = []
    received_ack = False
    received_c = False
    
    while time.time() - start_time < timeout:
        response = serial_mgr.Receive_Byte(timeout=0.1)
        if response is not None:
            received_responses.append(f"0x{response:02X}")
            print(f"收到应答：0x{response:02X}")
            
            if response == YModem.ACK:  # 0x06
                print("✓ 收到ACK")
                received_ack = True
            elif response == YModem.C:  # 0x43
                print("✓ 收到C字符，下位机准备接收数据")
                received_c = True
            elif response == YModem.NAK:  # 0x15
                print("✗ 收到NAK，需要重传")
                return False
            elif response == YModem.CAN:  # 0x18
                print("✗ 收到CAN，传输被取消")
                return False
            elif response == YModem.EOT:  # 0x04
                print("收到EOT字符（忽略）")
                continue
            else:
                print(f"收到其他应答：0x{response:02X}（继续等待）")
                continue
            
            # 检查是否同时收到了ACK和C
            if received_ack and received_c:
                print("✓ 已收到ACK+C，下位机准备就绪，可以发送数据包")
                print(f"本次等待期间收到的所有应答：{' '.join(received_responses)}")
                return True
    
    print("✗ 等待ACK+C超时")
    if received_responses:
        print(f"超时期间收到的应答：{' '.join(received_responses)}")
        print(f"ACK状态：{'✓' if received_ack else '✗'}，C状态：{'✓' if received_c else '✗'}")
    return False

def wait_for_ack(serial_mgr, timeout=5):
    """
    Brief: 等待接收ACK应答（用于数据包的响应）
    Params: 
        serial_mgr - 串口管理器实例
        timeout - 超时时间（秒）
    Return: 
        bool - 是否收到ACK
    """
    print(f"等待ACK应答（超时时间：{timeout}秒）...")
    
    start_time = time.time()
    received_responses = []  # 记录收到的所有应答
    
    while time.time() - start_time < timeout:
        response = serial_mgr.Receive_Byte(timeout=0.1)  # 每次等待0.1秒
        if response is not None:
            received_responses.append(f"0x{response:02X}")
            print(f"收到应答：0x{response:02X}")
            
            if response == YModem.ACK:  # 0x06
                print("✓ 收到ACK，继续传输")
                if len(received_responses) > 1:
                    print(f"本次等待期间收到的所有应答：{' '.join(received_responses)}")
                return True
            elif response == YModem.NAK:  # 0x15
                print("✗ 收到NAK，需要重传")
                return False
            elif response == YModem.CAN:  # 0x18
                print("✗ 收到CAN，传输被取消")
                return False
            elif response == YModem.C:  # 0x43 ('C')
                print("收到C字符（可能是延迟的响应，继续等待ACK）")
                continue
            elif response == YModem.EOT:  # 0x04
                print("收到EOT字符，传输结束标志（继续等待ACK）")
                continue
            else:
                print(f"收到未知应答：0x{response:02X}（继续等待）")
                continue
    
    print("✗ 等待ACK超时")
    if received_responses:
        print(f"超时期间收到的应答：{' '.join(received_responses)}")
    return False

def send_ymodem_packet(serial_mgr, ymodem, packet_num, data, packet_name="数据包"):
    """
    Brief: 发送YModem数据包
    Params: 
        serial_mgr - 串口管理器实例
        ymodem - YModem协议处理器实例
        packet_num - 数据包序号
        data - 要发送的数据
        packet_name - 数据包名称（用于显示）
    Return: 
        bool - 发送是否成功
    """
    try:
        # 构建数据包
        packet = ymodem.build_packet(packet_num, data)
        
        # 显示数据包信息
        print(f"\n=== 发送{packet_name} ===")
        print(f"包序号：{packet_num}")
        print(f"数据长度：{len(data)} 字节")
        print(f"数据包总长度：{len(packet)} 字节")
        
        # 显示数据包前32字节的十六进制内容
        hex_preview = " ".join([f"{b:02X}" for b in packet[:32]])
        print(f"数据包前32字节：{hex_preview}")
        
        # 发送数据包
        print(f"正在发送{packet_name}...")
        bytes_sent = serial_mgr.Send_Bytes(packet)
        print(f"成功发送 {bytes_sent} 字节")
        
        return True
        
    except Exception as e:
        print(f"发送{packet_name}时出错：{str(e)}")
        return False

def send_ymodem_transmission(serial_mgr, ymodem, bin_file_path, ack_timeout=5):
    """
    Brief: 执行完整的YModem传输流程
    Params: 
        serial_mgr - 串口管理器实例
        ymodem - YModem协议处理器实例
        bin_file_path - 二进制文件路径
        ack_timeout - 等待ACK的超时时间
    Return: 
        bool - 传输是否成功
    """
    try:
        # 加载二进制文件
        if not ymodem.bin_reader.load_file(bin_file_path):
            print(f"错误：无法加载文件 {bin_file_path}")
            return False
        
        # 获取文件信息
        file_size = ymodem.bin_reader.get_file_size()
        file_name = os.path.basename(bin_file_path)
        
        print(f"文件信息：{file_name}, 大小：{file_size} 字节")
        
        # 清空接收缓冲区
        serial_mgr.Clear_Buffer()
        
        # === 步骤1：发送第0包（文件信息包） ===
        file_info = f"{file_name}\0{file_size}\0".encode('utf-8')
        if not send_ymodem_packet(serial_mgr, ymodem, 0, file_info, "第0包（文件信息包）"):
            return False
        
        # 等待第0包的ACK+C响应
        if not wait_for_ack_and_c(serial_mgr, ack_timeout):
            print("第0包未收到完整的ACK+C响应，传输失败")
            return False
        
        # === 步骤2：等待1秒后发送第1包（数据包） ===
        print("\n等待1秒...")
        time.sleep(1)
        
        # 读取第1包的数据（前1024字节）
        data_chunk = ymodem.bin_reader.get_data(0, ymodem.PACKET_SIZE_1024)
        if data_chunk is None:
            print("读取文件数据失败")
            return False
        
        if not send_ymodem_packet(serial_mgr, ymodem, 1, data_chunk, "第1包（数据包）"):
            return False
        
        # 等待第1包的ACK
        if not wait_for_ack(serial_mgr, ack_timeout):
            print("第1包未收到ACK，传输失败")
            return False
        
        print("\n🎉 YModem传输完成！")
        print("- 第0包（文件信息）发送成功")
        print("- 第1包（数据）发送成功")
        print("- 所有ACK应答正常")
        
        return True
        
    except Exception as e:
        print(f"传输过程中出错：{str(e)}")
        return False

def main():
    """
    Brief: 主函数 - 执行完整的YModem传输流程
    """
    # 解析命令行参数
    args = parse_args()
    
    # 检查文件是否存在
    if not os.path.exists(args.file):
        print(f"错误：文件 {args.file} 不存在")
        return
    
    try:
        # 初始化串口管理器（增加超时时间）
        print(f"正在连接串口 {args.port}，波特率 {args.baud}...")
        serial_mgr = SerialManager(args.port, args.baud, timeout=2)
        
        # 初始化YModem协议处理器
        ymodem = YModem()
        
        # 执行YModem传输
        success = send_ymodem_transmission(serial_mgr, ymodem, args.file, args.timeout)
        
        if success:
            print("\n✅ 传输任务完成！")
        else:
            print("\n❌ 传输任务失败！")
        
        # 等待一秒确保数据发送完成
        time.sleep(1)
        
        # 关闭串口
        serial_mgr.close()
        print("串口已关闭")
        
    except Exception as e:
        print(f"程序执行出错：{str(e)}")

if __name__ == '__main__':
    main()
