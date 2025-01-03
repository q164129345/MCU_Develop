#include "user_main.h"
#include "AS5600_I2C.h"
#include "BLDCDriver3PWM.h"
#include "BLDCMotor.h"

// 外部变量
extern struct AS5600_I2CConfig_s AS5600_I2C_Config;
extern I2C_HandleTypeDef hi2c1;

// 全局变量
float g_Velocity; // 便于使用J-LINK Scope观察曲线

AS5600_I2C AS5600_1(AS5600_I2C_Config); // 创建AS5600_I2C对象
BLDCDriver3PWM motorDriver(GPIO_PIN_0,GPIO_PIN_1,GPIO_PIN_2); // PA0,PA1,PA2
BLDCMotor motor(7); // 创建BLDCMotor对象,电机是7对极

/**
 * @brief C++环境入口函数
 * 
 */
void main_Cpp(void)
{
    AS5600_1.init(&hi2c1); // 初始化AS5600
    motorDriver.voltage_power_supply = 12; // 设置电压
    motorDriver.voltage_limit = 6; // 设置电压限制
    motorDriver.init();   // 初始化电机驱动
    motor.linkDriver(&motorDriver); // 将电机驱动与电机连接
    motor.voltage_limit = 3; // 设置电机的电压限制
    motor.controller = MotionControlType::velocity_openloop; // 设置控制器模式(开环速度控制)
    motor.init(); // 初始化电机
    HAL_Delay(1000); // 延时1s
    HAL_TIM_Base_Start_IT(&htim4); // 启动TIM4定时器
    while(1) {
        HAL_GPIO_TogglePin(run_led_GPIO_Port,run_led_Pin); // 心跳灯跑起来
        AS5600_1.update(); // 更新位置，获取速度
        g_Velocity = AS5600_1.getVelocity(); // 获取速度
        SEGGER_RTT_printf(0,"motor_Velocity:%f\n",g_Velocity);
        SEGGER_Printf_Float(g_Velocity); // 打印电机速度
        delayMicroseconds(100000U); // 延时10ms
    }
}

/**
 * @brief 定时器中断回调函数
 * 
 * @param htim 定时器句柄
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    // 检查是否是我们期望的定时器
    if(htim->Instance == TIM2) // 假设我们使用TIM2
    {
    } else if(htim->Instance == TIM4) {
        motor.move(4); // 设置电机速度
    }
}












