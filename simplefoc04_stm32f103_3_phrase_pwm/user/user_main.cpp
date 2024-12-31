#include "user_main.h"
#include "AS5600_I2C.h"
#include "BLDCDriver3PWM.h"

// �ⲿ����
extern struct AS5600_I2CConfig_s AS5600_I2C_Config;
extern I2C_HandleTypeDef hi2c1;

// ȫ�ֱ���
AS5600_I2C AS5600_1(AS5600_I2C_Config); // ����AS5600_I2C����
float g_Velocity; // ����ʹ��J-LINK Scope�۲�����
BLDCDriver3PWM motorDriver(GPIO_PIN_0,GPIO_PIN_1,GPIO_PIN_2); // PA0,PA1,PA2

/**
 * @brief C++������ں���
 * 
 */
void main_Cpp(void)
{
    AS5600_1.init(&hi2c1); // ��ʼ��AS5600
    motorDriver.init(); // ��ʼ���������
    motorDriver.enable(); // ʹ�ܵ������
    motorDriver.setPwm(6,6,6); // ����50%ռ�ձ�
    HAL_Delay(1000); // ��ʱ1s
    while(1) {
        //HAL_GPIO_TogglePin(run_led_GPIO_Port,run_led_Pin); // ������������
        //SEGGER_RTT_printf(0,"Dwt_time_us:%u\n",_micros()); // ��ӡDWT��ʱ����us��ʱ�����������û������
        AS5600_1.update(); // ����λ�ã���ȡ�ٶ�
        g_Velocity = AS5600_1.getVelocity(); // ��ȡ�ٶ�
        SEGGER_RTT_printf(0,"motor_Velocity:%f\n",g_Velocity);
        SEGGER_Printf_Float(g_Velocity); // ��ӡ����ٶ�
        delayMicroseconds(100000U); // ��ʱ10ms
    }
}











