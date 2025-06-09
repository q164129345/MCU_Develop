from serial_manager import SerialManager
import argparse
import time

def parse_args():
    parser = argparse.ArgumentParser(description="串口发送 'hello, world'")
    parser.add_argument('--port', required=True, help='串口号，如COM3')
    parser.add_argument('--baud', type=int, default=115200, help='波特率')
    return parser.parse_args()

def send_hello_loop(serial_mgr):
    try:
        while True:
            serial_mgr.Send_Bytes(b'\x12\x34\x56\x78')
            time.sleep(1)
    except KeyboardInterrupt:
        print("已停止发送。")

def main():
    args = parse_args()
    serial_mgr = SerialManager(args.port, args.baud)
    send_hello_loop(serial_mgr)
    serial_mgr.close()

if __name__ == '__main__':
    main()
