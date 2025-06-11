import serial

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





