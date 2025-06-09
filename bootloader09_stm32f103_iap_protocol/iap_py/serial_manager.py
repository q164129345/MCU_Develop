import serial

class SerialManager:
    """
    Brief: 串口管理类，负责串口初始化和数据收发
    Params: port 串口号，如COM3
            baudrate 波特率
    """
    def __init__(self, port, baudrate):
        self.ser = serial.Serial(port, baudrate, timeout=1)

    def Send_String(self, data):
        self.ser.write(data.encode('utf-8'))
    
    def Send_Bytes(self, data):
        self.ser.write(data)

    def close(self):
        self.ser.close()





