#include "user_main.h"
#include "AS5600_I2C.h"
#include "BLDCDriver3PWM.h"

// 外部变量
extern struct AS5600_I2CConfig_s AS5600_I2C_Config;
extern I2C_HandleTypeDef hi2c1;

// 全局变量
AS5600_I2C AS5600_1(AS5600_I2C_Config); // 创建AS5600_I2C对象
float g_Velocity; // 便于使用J-LINK Scope观察曲线
BLDCDriver3PWM motorDriver(GPIO_PIN_0,GPIO_PIN_1,GPIO_PIN_2); // PA0,PA1,PA2

/**
 * @brief C++环境入口函数
 * 
 */
void main_Cpp(void)
{
    AS5600_1.init(&hi2c1); // 初始化AS5600
    motorDriver.init(); // 初始化电机驱动
    motorDriver.enable(); // 使能电机驱动
    motorDriver.setPwm(6,6,6); // 设置50%占空比
    HAL_Delay(1000); // 延时1s
    while(1) {
        //HAL_GPIO_TogglePin(run_led_GPIO_Port,run_led_Pin); // 心跳灯跑起来
        //SEGGER_RTT_printf(0,"Dwt_time_us:%u\n",_micros()); // 打印DWT定时器的us级时间戳，看看有没有问题
        AS5600_1.update(); // 更新位置，获取速度
        g_Velocity = AS5600_1.getVelocity(); // 获取速度
        SEGGER_RTT_printf(0,"motor_Velocity:%f\n",g_Velocity);
        SEGGER_Printf_Float(g_Velocity); // 打印电机速度
        delayMicroseconds(100000U); // 延时10ms
    }
}











