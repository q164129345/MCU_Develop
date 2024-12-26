#include "user_main.h"
#include "AS5600_I2C.h"

// �ⲿ����
extern struct AS5600_I2CConfig_s AS5600_I2C_Config;
extern I2C_HandleTypeDef hi2c1;

// ȫ�ֱ���
AS5600_I2C AS5600_1(AS5600_I2C_Config); // ����AS5600_I2C����

void SEGGER_Printf_Float(float value){
    char buffer[50];
    sprintf(buffer, "Value: %.2f\n", value); // ��ʽ��Ϊ�ַ���
    SEGGER_RTT_printf(0, "AS5600_Angle:%s", buffer); // ��ӡ�ַ���
}

/**
 * @brief C++������ں���
 * 
 */
void main_Cpp(void)
{
    AS5600_1.init(&hi2c1); // ��ʼ��AS5600
    HAL_Delay(1000); // ��ʱ1s
    while(1) {
        HAL_GPIO_TogglePin(run_led_GPIO_Port,run_led_Pin); // ������������
        SEGGER_RTT_printf(0,"Dwt_time_us:%u\n",_micros()); // ��ӡDWT��ʱ����us��ʱ�����������û������
        SEGGER_Printf_Float(AS5600_1.getSensorAngle());    // ��ӡ����Ƕ�
        delayMicroseconds(100000U); // ��ʱ100ms
    }
}











