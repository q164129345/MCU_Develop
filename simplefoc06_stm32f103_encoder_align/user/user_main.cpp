#include "user_main.h"
#include "AS5600_I2C.h"
#include "BLDCDriver3PWM.h"
#include "BLDCMotor.h"

// J-LINK Scope��Ϣ�ṹ
typedef struct {
    uint32_t timestamp;
    float a;
    float b;
    float c;
}J_LINK_Scope_Message;

// �ⲿ����
extern struct AS5600_I2CConfig_s AS5600_I2C_Config;
extern I2C_HandleTypeDef hi2c1;

// ȫ�ֱ���
float g_Velocity; // ����ʹ��J-LINK Scope�۲�����
volatile uint8_t JS_RTT_BufferUp1[2048] = {0,};
const uint8_t JS_RTT_Channel = 1;
J_LINK_Scope_Message JS_Message;

AS5600_I2C AS5600_1(AS5600_I2C_Config); // ����AS5600_I2C����
BLDCDriver3PWM motorDriver(GPIO_PIN_0,GPIO_PIN_1,GPIO_PIN_2); // PA0,PA1,PA2
BLDCMotor motor(7); // ����BLDCMotor����,�����7�Լ�

/**
 * @brief C++������ں���
 * 
 */
void main_Cpp(void)
{
    SEGGER_RTT_ConfigUpBuffer(JS_RTT_Channel,                  // ͨ����
                            // ͨ�����֣�����������ģ�һ��Ҫ���չٷ��ĵ���RTT channel naming convention���Ĺ淶����
                            "JScope_t4f4f4f4",              // ���ݰ���1��32λ��ʱ�����1��uint32_t������1��uint32_t����
                            (uint8_t*)&JS_RTT_BufferUp1[0], // �����ַ
                            sizeof(JS_RTT_BufferUp1),       // �����С
                            SEGGER_RTT_MODE_NO_BLOCK_SKIP); // ������
    AS5600_1.init(&hi2c1); // ��ʼ��AS5600
    motorDriver.voltage_power_supply = 12; // ���õ�ѹ
    motorDriver.voltage_limit = 6; // ���õ�ѹ����
    motorDriver.init();   // ��ʼ���������
    motor.linkDriver(&motorDriver); // �����������������
    motor.voltage_limit = 3; // ���õ���ĵ�ѹ����
    motor.controller = MotionControlType::velocity_openloop; // ���ÿ�����ģʽ(�����ٶȿ���)
    motor.init(); // ��ʼ�����
    motor.foc_modulation = FOCModulationType::SpaceVectorPWM; // ���Ҳ���Ϊ����
    motor.linkSensor(&AS5600_1); // ����������������
    motor.sensor_direction = Direction::UNKNOWN; // ����UNKNOW��Ŀ������initFOC()�����ڳ�ʼ��ʱ���Զ�̽�Ⲣ���ô������ķ���
    motor.initFOC(); // ��ʼ��FOC
    //motor.enable(); // ʹ�ܵ��
    HAL_Delay(1000); // ��ʱ1s
    //HAL_TIM_Base_Start_IT(&htim4); // ����TIM4��ʱ��
    while(1) {
        HAL_GPIO_TogglePin(run_led_GPIO_Port,run_led_Pin); // ������������
        AS5600_1.update(); // ����λ�ã���ȡ�ٶ�
        g_Velocity = AS5600_1.getVelocity(); // ��ȡ�ٶ�
        //SEGGER_RTT_printf(0,"motor_Velocity:%f\n",g_Velocity);
        //SEGGER_Printf_Float(g_Velocity); // ��ӡ����ٶ�
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
    if(htim->Instance == TIM2)
    {
    } else if(htim->Instance == TIM4) {
        motor.move(4); // ���õ���ٶ�
        JS_Message.timestamp = _micros(); // ��ȡʱ���
        // ��ռ�ձȷŴ�10�������ڹ۲�
        JS_Message.a = motor.driver->dc_a * 10; // A��ռ�ձ�
        JS_Message.b = motor.driver->dc_b * 10; // B��ռ�ձ�
        JS_Message.c = motor.driver->dc_c * 10; // C��ռ�ձ�
        SEGGER_RTT_Write(JS_RTT_Channel, (uint8_t*)&JS_Message, sizeof(JS_Message)); // д��J-LINK Scope
    }
}












