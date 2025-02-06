#include "user_main.h"
#include "AS5600_I2C.h"
#include "BLDCDriver3PWM.h"
#include "BLDCMotor.h"
#include "InlineCurrentSense.h"

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
InlineCurrentSense currentSense(0.001f,50.0f,ADC_CHANNEL_3,ADC_CHANNEL_4,NOT_SET); // ������������������

float targetAngle = 1.0f; // Ŀ��Ƕ�
float curAngle = 0.0f; // ��ǰ�Ƕ�

/**
 * @brief C++������ں���
 * 
 */
void main_Cpp(void)
{
    SEGGER_RTT_ConfigUpBuffer(JS_RTT_Channel,               // ͨ����
                            // ͨ�����֣�����������ģ�һ��Ҫ���չٷ��ĵ���RTT channel naming convention���Ĺ淶����
                            "JScope_t4f4f4f4",              // ���ݰ���1��32λ��ʱ�����1��uint32_t������1��uint32_t����
                            (uint8_t*)&JS_RTT_BufferUp1[0], // �����ַ
                            sizeof(JS_RTT_BufferUp1),       // �����С
                            SEGGER_RTT_MODE_NO_BLOCK_SKIP); // ������
    AS5600_1.init(&hi2c1); // ��ʼ��AS5600
    motorDriver.voltage_power_supply = DEF_POWER_SUPPLY; // ���õ�ѹ
    motorDriver.init();   // ��ʼ���������

    currentSense.skip_align = true; // ����������������
    currentSense.init();   // ��ʼ������������
    currentSense.linkDriver(&motorDriver); // ��������������������
                            
    motor.linkSensor(&AS5600_1); // ���ӱ�����
    motor.linkDriver(&motorDriver); // ����������
    motor.linkCurrentSense(&currentSense); // ���ӵ���������
    motor.voltage_sensor_align = 6; // У׼ƫ��offsetʱ�����õ��ĵ�ѹֵ���൱��ռ�ձ�4V / 12V = 1/3��
    motor.controller = MotionControlType::angle; // ���ÿ�����ģʽ(λ�ñջ�ģʽ)

    motor.PID_velocity.P = 0.50f; // �����ٶ�P
    motor.PID_velocity.I = 10.0f; // �����ٶ�I
    motor.PID_velocity.D = 0; // �����ٶ�D
    motor.PID_velocity.output_ramp = 0; // 0��������б��
    motor.LPF_velocity.Tf = 0.01f; // �����ٶȵ�ͨ�˲���

    motor.P_angle.P = 50.0f; // λ�û�P
    motor.P_angle.I = 1000.0f; // λ�û�I
    motor.P_angle.D = 0.0f;  // λ�û�D
    motor.P_angle.output_ramp = 0; // ������
    
    motor.PID_current_q.P = 0.3f;
    motor.PID_current_q.I = 1.0f;
    motor.PID_current_q.D = 0;
    motor.PID_current_q.output_ramp = 0; // ������
    motor.LPF_current_q.Tf = 0.01f;      // ��ͨ�˲���
    
    motor.PID_current_d.P = 0.3f;
    motor.PID_current_d.I = 1.0f;
    motor.PID_current_d.D = 0;
    motor.PID_current_d.output_ramp = 0; // 0��������б��
    motor.LPF_current_d.Tf = 0.01f;
    
    motor.current_limit = DEF_POWER_SUPPLY; // ��������
    motor.voltage_limit = DEF_POWER_SUPPLY; // ��ѹ����
    motor.velocity_limit = DEF_POWER_SUPPLY; // λ�ñջ�ģʽʱ�����λ�û�PID��limit
    motor.torque_controller = TorqueControlType::dc_current; // Iq�ջ���Id = 0
    
    motor.init(); // ��ʼ�����

    motor.PID_current_q.limit = DEF_POWER_SUPPLY - 8.5f;
    motor.PID_current_d.limit = DEF_POWER_SUPPLY - 8.5f;

    motor.foc_modulation = FOCModulationType::SpaceVectorPWM; // ���Ҳ���Ϊ����
    motor.sensor_direction = Direction::CCW; // ֮ǰУ׼��������ʱ��֪���������ķ�����CCW������У׼���������½ھ�֪����
    motor.initFOC(); // ��ʼ��FOC

    SEGGER_RTT_printf(0,"motor.zero_electric_angle:");
    SEGGER_Printf_Float(motor.zero_electric_angle); // ��ӡ������Ƕ�
    SEGGER_RTT_printf(0,"Sensor:");
    SEGGER_Printf_Float(AS5600_1.getMechanicalAngle()); // ��ӡ�������Ƕ�
    HAL_Delay(1000); // ��ʱ1s
    HAL_TIM_Base_Start_IT(&htim4); // ����TIM4��ʱ��
    while(1) {
        HAL_GPIO_TogglePin(run_led_GPIO_Port,run_led_Pin); // ������������
        curAngle = motor.shaft_angle; // ��ȡ��ǰλ��
        SEGGER_RTT_printf(0,"curAngle:");
        SEGGER_Printf_Float(curAngle); // ��ӡ��ǰλ��
        SEGGER_RTT_printf(0,"targetAngle:");
        SEGGER_Printf_Float(targetAngle); // ��ӡĿ��λ��
        delayMicroseconds(100000U); // ��ʱ100ms
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
        
        HAL_GPIO_WritePin(measure_GPIO_Port,measure_Pin,GPIO_PIN_SET); // ���ߵ�ƽ
        motor.loopFOC(); // ִ��FOC
        motor.move(targetAngle); // ����Ŀ��Ƕ�
        HAL_GPIO_WritePin(measure_GPIO_Port,measure_Pin,GPIO_PIN_RESET); // ���͵�ƽ
        
        JS_Message.timestamp = _micros(); // ��ȡʱ���
        // ��ռ�ձȷŴ�10�������ڹ۲�
        JS_Message.a = motor.driver->dc_a * 10; // A��ռ�ձ�
        JS_Message.b = motor.driver->dc_b * 10; // B��ռ�ձ�
        JS_Message.c = motor.driver->dc_c * 10; // C��ռ�ձ�
        SEGGER_RTT_Write(JS_RTT_Channel, (uint8_t*)&JS_Message, sizeof(JS_Message)); // д��J-LINK Scope
    }
}












