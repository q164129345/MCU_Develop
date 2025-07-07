#include "user_main.h"
#include "tim.h"
#include "BLDCDriver6PWM.h"
#include "BLDCMotor.h"
//#include "InlineCurrentSense.h"

// J-LINK Scope消息结构
typedef struct {
    uint32_t timestamp;
    float a;
    float b;
    float c;
}J_LINK_Scope_Message;

// 全局变量
float g_Velocity; // 便于使用J-LINK Scope观察曲线
volatile uint8_t JS_RTT_BufferUp1[2048] = {0,};
const uint8_t JS_RTT_Channel = 1;
J_LINK_Scope_Message JS_Message;

BLDCDriver6PWM motorDriver(&htim1); //! 初始化驱动器(htim1是TIM1定时器)
//BLDCDriver3PWM motorDriver(GPIO_PIN_0,GPIO_PIN_1,GPIO_PIN_2); // PA0,PA1,PA2
BLDCMotor motor(21); // 创建BLDCMotor对象,电机是21对极
//InlineCurrentSense currentSense(0.001f,50.0f,ADC_CHANNEL_3,ADC_CHANNEL_4,NOT_SET); // 创建电流传感器对象

float targetVel = 4.0f; //! 目标速度

/**
 * @brief C++环境入口函数
 * 
 */
void main_Cpp(void)
{
    SEGGER_RTT_ConfigUpBuffer(JS_RTT_Channel,               // 通道号
                            // 通道名字（命名有意义的，一定要按照官方文档“RTT channel naming convention”的规范来）
                            "JScope_t4f4f4f4",              // 数据包含1个32位的时间戳与1个uint32_t变量、1个uint32_t变量
                            (uint8_t*)&JS_RTT_BufferUp1[0], // 缓存地址
                            sizeof(JS_RTT_BufferUp1),       // 缓存大小
                            SEGGER_RTT_MODE_NO_BLOCK_SKIP); // 非阻塞
                            
    motorDriver.voltage_power_supply = DEF_POWER_SUPPLY; // 设置电压
    motorDriver.voltage_limit = 2.0f; //! 设置电压限制
    motorDriver.init();   // 初始化电机驱动
//    motor.voltage_sensor_align = 6; // 校准偏移offset时，所用到的电压值（相当于占空比4V / 12V = 1/3）

//    motor.PID_velocity.P = 0.30f; // 设置速度P
//    motor.PID_velocity.I = 20.0f; // 设置速度I
//    motor.PID_velocity.D = 0; // 设置速度D
//    motor.PID_velocity.output_ramp = 0; // 0：不设置斜坡
//    motor.LPF_velocity.Tf = 0.01f; // 设置速度低通滤波器

//    motor.voltage_limit = DEF_POWER_SUPPLY * 2.0f; // 电压限制
    motor.linkDriver(&motorDriver); // 将电机驱动器与电机对象关联
    motor.voltage_limit = 2.0f; //! 设置电压限制
    motor.torque_controller = TorqueControlType::voltage; // Iq闭环，Id = 0
    motor.controller = MotionControlType::velocity_openloop; // 设置控制器模式，开环速度控制模式
    motor.init(); // 初始化电机
    // motor.PID_velocity.limit = DEF_POWER_SUPPLY * 2.0f;
    
//    motor.foc_modulation = FOCModulationType::SpaceVectorPWM; // 正弦波改为马鞍波
//    motor.sensor_direction = Direction::CCW; // 之前校准传感器的时候，知道传感器的方向是CCW（翻开校准传感器的章节就知道）

//    motor.initFOC(); // 初始化FOC

//    SEGGER_RTT_printf(0,"motor.zero_electric_angle:");
//    SEGGER_Printf_Float(motor.zero_electric_angle); // 打印电机零电角度
//    SEGGER_RTT_printf(0,"Sensor:");
//    SEGGER_Printf_Float(AS5600_1.getMechanicalAngle()); // 打印传感器角度
    HAL_Delay(1000); // 延时1s
    HAL_TIM_Base_Start_IT(&htim4); // 启动TIM4定时器
    HAL_Delay(1000);
    while(1) {
        HAL_GPIO_TogglePin(RUN_LED_GPIO_Port,RUN_LED_Pin); // 心跳灯跑起来
        //SEGGER_RTT_printf(0, "System Tick: %lu ms\n", HAL_GetTick()); // 获取系统滴答数（毫秒），打印出来
        delayMicroseconds(200000U); // 延时200ms
    }
}
/**
 * @brief 定时器中断回调函数
 * 
 * @param htim 定时器句柄
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if(htim->Instance == TIM1)
    {
    } else if(htim->Instance == TIM4) {
        
        motor.loopFOC(); // 执行FOC
        motor.move(targetVel); // 控制目标速度
        
        JS_Message.timestamp = _micros(); // 获取时间戳
        // 将占空比放大10倍，便于观察
        JS_Message.a = motor.driver->dc_a * 10; // A相占空比
        JS_Message.b = motor.driver->dc_b * 10; // B相占空比
        JS_Message.c = motor.driver->dc_c * 10; // C相占空比
        SEGGER_RTT_Write(JS_RTT_Channel, (uint8_t*)&JS_Message, sizeof(JS_Message)); // 写入J-LINK Scope
    }
}

