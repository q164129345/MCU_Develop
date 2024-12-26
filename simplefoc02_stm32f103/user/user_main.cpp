#include "user_main.h"
#include "AS5600_I2C.h"

// 外部变量
extern struct AS5600_I2CConfig_s AS5600_I2C_Config;
extern I2C_HandleTypeDef hi2c1;

// 全局变量
AS5600_I2C AS5600_1(AS5600_I2C_Config); // 创建AS5600_I2C对象

void SEGGER_Printf_Float(float value){
    char buffer[50];
    sprintf(buffer, "Value: %.2f\n", value); // 格式化为字符串
    SEGGER_RTT_printf(0, "AS5600_Angle:%s", buffer); // 打印字符串
}

/**
 * @brief C++环境入口函数
 * 
 */
void main_Cpp(void)
{
    AS5600_1.init(&hi2c1); // 初始化AS5600
    HAL_Delay(1000); // 延时1s
    while(1) {
        HAL_GPIO_TogglePin(run_led_GPIO_Port,run_led_Pin); // 心跳灯跑起来
        SEGGER_RTT_printf(0,"Dwt_time_us:%u\n",_micros()); // 打印DWT定时器的us级时间戳，看看有没有问题
        SEGGER_Printf_Float(AS5600_1.getSensorAngle());    // 打印电机角度
        delayMicroseconds(100000U); // 延时100ms
    }
}











