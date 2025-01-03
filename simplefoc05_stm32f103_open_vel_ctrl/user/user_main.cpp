#include "user_main.h"
#include "AS5600_I2C.h"
#include "BLDCDriver3PWM.h"
#include "BLDCMotor.h"

// �ⲿ����
extern struct AS5600_I2CConfig_s AS5600_I2C_Config;
extern I2C_HandleTypeDef hi2c1;

// ȫ�ֱ���
float g_Velocity; // ����ʹ��J-LINK Scope�۲�����

AS5600_I2C AS5600_1(AS5600_I2C_Config); // ����AS5600_I2C����
BLDCDriver3PWM motorDriver(GPIO_PIN_0,GPIO_PIN_1,GPIO_PIN_2); // PA0,PA1,PA2
BLDCMotor motor(7); // ����BLDCMotor����,�����7�Լ�

/**
 * @brief C++������ں���
 * 
 */
void main_Cpp(void)
{
    AS5600_1.init(&hi2c1); // ��ʼ��AS5600
    motorDriver.voltage_power_supply = 12; // ���õ�ѹ
    motorDriver.voltage_limit = 6; // ���õ�ѹ����
    motorDriver.init();   // ��ʼ���������
    motor.linkDriver(&motorDriver); // �����������������
    motor.voltage_limit = 3; // ���õ���ĵ�ѹ����
    motor.controller = MotionControlType::velocity_openloop; // ���ÿ�����ģʽ(�����ٶȿ���)
    motor.init(); // ��ʼ�����
    HAL_Delay(1000); // ��ʱ1s
    HAL_TIM_Base_Start_IT(&htim4); // ����TIM4��ʱ��
    while(1) {
        HAL_GPIO_TogglePin(run_led_GPIO_Port,run_led_Pin); // ������������
        AS5600_1.update(); // ����λ�ã���ȡ�ٶ�
        g_Velocity = AS5600_1.getVelocity(); // ��ȡ�ٶ�
        SEGGER_RTT_printf(0,"motor_Velocity:%f\n",g_Velocity);
        SEGGER_Printf_Float(g_Velocity); // ��ӡ����ٶ�
        delayMicroseconds(100000U); // ��ʱ10ms
    }
}

/**
 * @brief ��ʱ���жϻص�����
 * 
 * @param htim ��ʱ�����
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    // ����Ƿ������������Ķ�ʱ��
    if(htim->Instance == TIM2) // ��������ʹ��TIM2
    {
    } else if(htim->Instance == TIM4) {
        motor.move(4); // ���õ���ٶ�
    }
}












