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
    parser.add_argument('--retries', type=int, default=3, help='每个数据包的最大重试次数，默认3次')
    parser.add_argument('--verbose', '-v', action='store_true', help='显示详细的传输信息')
    parser.add_argument('--auto-jump', action='store_true', help='自动发送跳转命令到bootloader')
    return parser.parse_args()

def wait_for_ack_and_c(serial_mgr, timeout=5, verbose=False):
    """
    Brief: 等待接收ACK和C字符（用于第0包后的响应）
    Params: 
        serial_mgr - 串口管理器实例
        timeout - 超时时间（秒）
        verbose - 是否显示详细信息
    Return: 
        bool - 是否同时收到ACK和C
    """
    if verbose:
        print(f"等待ACK+C应答（超时时间：{timeout}秒）...")
    else:
        print("等待ACK+C...", end=" ")
    
    start_time = time.time()
    received_responses = []
    received_ack = False
    received_c = False
    
    while time.time() - start_time < timeout:
        response = serial_mgr.Receive_Byte(timeout=0.1)
        if response is not None:
            received_responses.append(f"0x{response:02X}")
            if verbose:
                print(f"收到应答：0x{response:02X}")
            
            if response == YModem.ACK:  # 0x06
                if verbose:
                    print("✓ 收到ACK")
                received_ack = True
            elif response == YModem.C:  # 0x43
                if verbose:
                    print("✓ 收到C字符，下位机准备接收数据")
                received_c = True
            elif response == YModem.NAK:  # 0x15
                if verbose:
                    print("✗ 收到NAK，需要重传")
                else:
                    print("NAK")
                return False
            elif response == YModem.CAN:  # 0x18
                if verbose:
                    print("✗ 收到CAN，传输被取消")
                else:
                    print("CAN")
                return False
            elif response == YModem.EOT:  # 0x04
                if verbose:
                    print("收到EOT字符（忽略）")
                continue
            else:
                if verbose:
                    print(f"收到其他应答：0x{response:02X}（继续等待）")
                continue
            
            # 检查是否同时收到了ACK和C
            if received_ack and received_c:
                if verbose:
                    print("✓ 已收到ACK+C，下位机准备就绪，可以发送数据包")
                    print(f"本次等待期间收到的所有应答：{' '.join(received_responses)}")
                else:
                    print("完成")
                return True
    
    if verbose:
        print("✗ 等待ACK+C超时")
        if received_responses:
            print(f"超时期间收到的应答：{' '.join(received_responses)}")
            print(f"ACK状态：{'✓' if received_ack else '✗'}，C状态：{'✓' if received_c else '✗'}")
    else:
        print("超时")
    return False

def wait_for_ack(serial_mgr, timeout=5, verbose=False):
    """
    Brief: 等待接收ACK应答（用于数据包的响应）
    Params: 
        serial_mgr - 串口管理器实例
        timeout - 超时时间（秒）
        verbose - 是否显示详细信息
    Return: 
        bool - 是否收到ACK
    """
    if verbose:
        print(f"等待ACK应答（超时时间：{timeout}秒）...")
    else:
        print("等待ACK...", end=" ")
    
    start_time = time.time()
    received_responses = []  # 记录收到的所有应答
    
    while time.time() - start_time < timeout:
        response = serial_mgr.Receive_Byte(timeout=0.1)  # 每次等待0.1秒
        if response is not None:
            received_responses.append(f"0x{response:02X}")
            if verbose:
                print(f"收到应答：0x{response:02X}")
            
            if response == YModem.ACK:  # 0x06
                if verbose:
                    print("✓ 收到ACK，继续传输")
                    if len(received_responses) > 1:
                        print(f"本次等待期间收到的所有应答：{' '.join(received_responses)}")
                else:
                    print("ACK")
                return True
            elif response == YModem.NAK:  # 0x15
                if verbose:
                    print("✗ 收到NAK，需要重传")
                else:
                    print("NAK")
                return False
            elif response == YModem.CAN:  # 0x18
                if verbose:
                    print("✗ 收到CAN，传输被取消")
                else:
                    print("CAN")
                return False
            elif response == YModem.C:  # 0x43 ('C')
                if verbose:
                    print("收到C字符（可能是延迟的响应，继续等待ACK）")
                continue
            elif response == YModem.EOT:  # 0x04
                if verbose:
                    print("收到EOT字符，传输结束标志（继续等待ACK）")
                continue
            else:
                if verbose:
                    print(f"收到未知应答：0x{response:02X}（继续等待）")
                continue
    
    if verbose:
        print("✗ 等待ACK超时")
        if received_responses:
            print(f"超时期间收到的应答：{' '.join(received_responses)}")
    else:
        print("超时")
    return False

def send_ymodem_packet(serial_mgr, ymodem, packet_num, data, packet_name="数据包", verbose=False):
    """
    Brief: 发送YModem数据包
    Params: 
        serial_mgr - 串口管理器实例
        ymodem - YModem协议处理器实例
        packet_num - 数据包序号
        data - 要发送的数据
        packet_name - 数据包名称（用于显示）
        verbose - 是否显示详细信息
    Return: 
        bool - 发送是否成功
    """
    try:
        # 构建数据包
        packet = ymodem.build_packet(packet_num, data)
        
        # 显示数据包信息
        if verbose:
            print(f"\n=== 发送{packet_name} ===")
            print(f"包序号：{packet_num}")
            print(f"数据长度：{len(data)} 字节")
            print(f"数据包总长度：{len(packet)} 字节")
            
            # 显示数据包前32字节的十六进制内容
            hex_preview = " ".join([f"{b:02X}" for b in packet[:32]])
            print(f"数据包前32字节：{hex_preview}")
            
            print(f"正在发送{packet_name}...")
        else:
            # 简化模式只显示关键信息
            print(f"发送{packet_name}...", end=" ")
        
        # 发送数据包
        bytes_sent = serial_mgr.Send_Bytes(packet)
        
        if verbose:
            print(f"成功发送 {bytes_sent} 字节")
        else:
            print(f"完成({bytes_sent}字节)")
        
        return True
        
    except Exception as e:
        print(f"发送{packet_name}时出错：{str(e)}")
        return False

def send_jump_command(serial_mgr, timeout=10, verbose=False):
    """
    Brief: 发送跳转到bootloader的命令并等待'C'字符心跳
    Params: 
        serial_mgr - 串口管理器实例
        timeout - 等待bootloader心跳的超时时间（秒）
        verbose - 是否显示详细信息
    Return: 
        bool - 是否成功收到bootloader的'C'字符心跳
    """
    jump_command = "A5A5A5A5"
    
    if verbose:
        print(f"\n=== 发送跳转命令 ===")
        print(f"命令内容：'{jump_command}'")
        print(f"ASCII码：{' '.join([f'0x{ord(c):02X}' for c in jump_command])}")
    else:
        print("发送跳转命令...", end=" ")
    
    try:
        # 清空接收缓冲区
        serial_mgr.Clear_Buffer()
        
        # 等待MCU稳定（很重要！）
        time.sleep(0.1)
        
        # 发送跳转命令
        bytes_sent = serial_mgr.Send_String(jump_command)
        
        if verbose:
            print(f"成功发送 {bytes_sent} 字节")
            print("等待bootloader发送'C'字符心跳...")
            print(f"bootloader每2秒发送一次'C'字符，超时时间：{timeout}秒")
        else:
            print(f"完成({bytes_sent}字节)")
            print("等待'C'心跳...", end=" ")
        
        # 等待bootloader发送'C'字符心跳
        start_time = time.time()
        received_responses = []
        
        while time.time() - start_time < timeout:
            response = serial_mgr.Receive_Byte(timeout=0.1)
            if response is not None:
                received_responses.append(f"0x{response:02X}")
                if verbose:
                    print(f"收到：0x{response:02X}")
                
                if response == YModem.C:  # 0x43 ('C')
                    if verbose:
                        print("✅ 收到'C'字符心跳！bootloader已准备就绪")
                        if len(received_responses) > 1:
                            print(f"本次等待期间收到的所有数据：{' '.join(received_responses)}")
                    else:
                        print("成功")
                    return True
                elif response == YModem.NAK:  # 0x15
                    if verbose:
                        print("收到NAK（可能是之前的残留数据）")
                elif response == YModem.ACK:  # 0x06
                    if verbose:
                        print("收到ACK（可能是之前的残留数据）")
                else:
                    if verbose:
                        print(f"收到其他字符：0x{response:02X}")
        
        if verbose:
            print("⚠️ 等待'C'字符心跳超时")
            if received_responses:
                print(f"超时期间收到的数据：{' '.join(received_responses)}")
            print("可能原因：")
            print("1. MCU还未完成跳转（需要更长等待时间）")
            print("2. bootloader心跳功能未启用")
            print("3. 串口连接问题")
        else:
            print("超时")
        return False
        
    except Exception as e:
        print(f"发送跳转命令时出错：{str(e)}")
        return False

def send_ymodem_transmission(serial_mgr, ymodem, bin_file_path, ack_timeout=5, max_retries=3, verbose=False, auto_jump=False):
    """
    Brief: 执行完整的YModem传输流程
    Params: 
        serial_mgr - 串口管理器实例
        ymodem - YModem协议处理器实例
        bin_file_path - 二进制文件路径
        ack_timeout - 等待ACK的超时时间
        max_retries - 每个数据包的最大重试次数
        verbose - 是否显示详细信息
        auto_jump - 是否自动发送跳转命令
    Return: 
        bool - 传输是否成功
    """
    try:
        # === 步骤0：如果启用自动跳转，先发送跳转命令 ===
        if auto_jump:
            print("=== 自动跳转到bootloader ===")
            if not send_jump_command(serial_mgr, timeout=6, verbose=verbose):
                print("跳转到bootloader失败，请手动重启到bootloader模式")
                return False
            
            # 收到bootloader心跳后，可以立即开始传输
            print("收到bootloader心跳，开始YModem传输...")
        
        # 加载二进制文件
        if not ymodem.bin_reader.load_file(bin_file_path):
            print(f"错误：无法加载文件 {bin_file_path}")
            return False
        
        # 获取文件信息
        file_size = ymodem.bin_reader.get_file_size()
        file_name = os.path.basename(bin_file_path)
        
        print(f"文件信息：{file_name}, 大小：{file_size} 字节")
        
        # 计算需要发送的数据包总数
        total_packets = (file_size + ymodem.PACKET_SIZE_1024 - 1) // ymodem.PACKET_SIZE_1024
        print(f"将分为 {total_packets} 个数据包传输")
        
        # 如果没有自动跳转，清空接收缓冲区
        if not auto_jump:
            serial_mgr.Clear_Buffer()
        
        # === 步骤1：发送第0包（文件信息包） ===
        file_info = f"{file_name}\0{file_size}\0".encode('utf-8')
        if not send_ymodem_packet(serial_mgr, ymodem, 0, file_info, "第0包（文件信息包）", verbose):
            return False
        
        # 等待第0包的ACK+C响应
        if not wait_for_ack_and_c(serial_mgr, ack_timeout, verbose):
            print("第0包未收到完整的ACK+C响应，传输失败")
            return False
        
        # 给下位机处理时间，延时1秒后再发送数据包
        print("延时1秒，给下位机处理时间...")
        time.sleep(1)
        
        # === 步骤2：循环发送所有数据包 ===
        print(f"\n开始发送数据包（共{total_packets}包）...")
        
        for packet_num in range(1, total_packets + 1):
            # 计算当前包的数据偏移和大小
            data_offset = (packet_num - 1) * ymodem.PACKET_SIZE_1024
            remaining_bytes = file_size - data_offset
            chunk_size = min(ymodem.PACKET_SIZE_1024, remaining_bytes)
            
            # 读取当前包的数据
            data_chunk = ymodem.bin_reader.get_data(data_offset, chunk_size)
            if data_chunk is None:
                print(f"读取第{packet_num}包数据失败")
                return False
            
            # 发送数据包（带重传机制）
            packet_sent = False
            
            for retry in range(max_retries):
                # 显示进度信息
                progress = (packet_num * 100) // total_packets
                retry_info = f"（重试{retry+1}/{max_retries}）" if retry > 0 else ""
                packet_name = f"第{packet_num}/{total_packets}包 [{progress}%]{retry_info}"
                
                # 发送数据包
                if not send_ymodem_packet(serial_mgr, ymodem, packet_num, data_chunk, packet_name, verbose):
                    if retry == max_retries - 1:
                        print(f"第{packet_num}包发送失败，已重试{max_retries}次")
                        return False
                    continue
                
                # 等待ACK响应
                if wait_for_ack(serial_mgr, ack_timeout, verbose):
                    packet_sent = True
                    
                    # 如果不是最后一包，给下位机处理时间
                    if packet_num < total_packets:
                        if verbose:
                            print("延时1秒，给下位机处理时间...")
                        else:
                            print("延时1秒...")
                        time.sleep(1)
                    
                    break
                else:
                    print(f"第{packet_num}包未收到ACK，准备重传...")
                    if retry < max_retries - 1:
                        time.sleep(0.5)  # 重传前稍作等待
            
            if not packet_sent:
                print(f"第{packet_num}包发送失败，传输中断")
                return False
        
        # === 步骤3：发送结束包（EOT） ===
        print(f"\n=== 发送传输结束信号 ===")
        
        # 发送EOT字符表示数据传输结束
        eot_bytes = bytes([ymodem.EOT])
        print("发送EOT字符...")
        serial_mgr.Send_Bytes(eot_bytes)
        
        # 等待EOT的ACK响应
        if not wait_for_ack(serial_mgr, ack_timeout, verbose):
            print("EOT未收到ACK响应")
            # 注：即使EOT没收到ACK，数据传输也可能已经成功
        
        # === 步骤4：发送最终结束包（空文件名包） ===
        print(f"\n=== 发送最终结束包 ===")
        
        # 等待下位机发送C字符（请求下一个文件）
        print("等待下位机请求下一个文件...")
        start_time = time.time()
        received_c = False
        
        while time.time() - start_time < ack_timeout:
            response = serial_mgr.Receive_Byte(timeout=0.1)
            if response is not None:
                if verbose:
                    print(f"收到：0x{response:02X}")
                else:
                    print(f"收到应答：0x{response:02X}")
                    
                if response == ymodem.C:
                    print("✓ 收到C字符，下位机请求下一个文件")
                    received_c = True
                    break
                elif response == ymodem.ACK:
                    if verbose:
                        print("收到额外的ACK（可能是延迟响应）")
                elif response == ymodem.EOT:
                    if verbose:
                        print("收到EOT（可能是下位机重发）")
                else:
                    if verbose:
                        print(f"收到其他字符：0x{response:02X}")
        
        if received_c:
            # 发送空文件名包表示传输彻底结束
            end_packet = ymodem.build_end_packet()
            print("发送空文件名包（传输结束标志）...")
            serial_mgr.Send_Bytes(end_packet)
            
            # 等待最终ACK
            if wait_for_ack(serial_mgr, ack_timeout, verbose):
                print("✓ 收到最终ACK，传输完全结束")
            else:
                print("未收到最终ACK，但传输可能已完成")
        else:
            print("未收到下位机的C字符请求，可能传输已完成")
        
        print("\n🎉 YModem完整传输流程完成！")
        print(f"- 文件信息包发送成功")
        print(f"- 数据包发送成功（共{total_packets}包）")
        print(f"- 传输结束信号发送完成")
        print(f"- 总传输字节数：{file_size} 字节")
        
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
        success = send_ymodem_transmission(serial_mgr, ymodem, args.file, args.timeout, args.retries, args.verbose, args.auto_jump)
        
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
