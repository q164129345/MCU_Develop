# -*- coding: utf-8 -*-
# Tested with Python 3.13.3

from serial_manager import SerialManager
import argparse
import time
import sys

def parse_args():
    """
    Brief: 解析命令行参数
    Return: 解析后的参数对象
    """
    parser = argparse.ArgumentParser(description="发送跳转到bootloader的命令")
    parser.add_argument('--port', required=True, help='串口号，如COM3')
    parser.add_argument('--baud', type=int, default=115200, help='波特率，默认115200')
    parser.add_argument('--timeout', type=int, default=10, help='等待bootloader心跳的超时时间（秒），默认10秒')
    parser.add_argument('--verbose', '-v', action='store_true', help='显示详细的传输信息')
    return parser.parse_args()

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
            print("💡 新功能：现在bootloader会主动发送'C'字符心跳")
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
                
                if response == 0x43:  # 'C'字符
                    if verbose:
                        print("✅ 收到'C'字符心跳！bootloader已准备就绪")
                        if len(received_responses) > 1:
                            print(f"本次等待期间收到的所有数据：{' '.join(received_responses)}")
                    else:
                        print("成功")
                    return True
                elif response == 0x15:  # NAK
                    if verbose:
                        print("收到NAK（可能是之前的残留数据）")
                elif response == 0x06:  # ACK
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

def main():
    """
    Brief: 主函数 - 发送跳转命令
    """
    # 解析命令行参数
    args = parse_args()
    
    try:
        # 初始化串口管理器
        print(f"正在连接串口 {args.port}，波特率 {args.baud}...")
        serial_mgr = SerialManager(args.port, args.baud, timeout=2)
        
        # 发送跳转命令
        success = send_jump_command(serial_mgr, args.timeout, args.verbose)
        
        if success:
            print("\n✅ 跳转成功！收到bootloader心跳")
            print("💡 功能说明：")
            print("- bootloader会每2秒主动发送'C'字符心跳")
            print("- 上位机可以确切知道bootloader何时准备就绪")
            print("- 现在可以使用main.py --auto-jump发送固件文件")
        else:
            print("\n❌ 跳转失败！未收到bootloader心跳")
            print("请检查：")
            print("1. MCU是否正在运行App程序")
            print("2. 串口连接是否正常")
            print("3. App程序是否正确实现了IAP_Parse_Command()函数")
            print("4. bootloader是否已添加心跳功能")
        
        # 等待一秒确保数据发送完成
        time.sleep(1)
        
        # 关闭串口
        serial_mgr.close()
        print("串口已关闭")
        
    except Exception as e:
        print(f"程序执行出错：{str(e)}")
        sys.exit(1)

if __name__ == '__main__':
    main() 