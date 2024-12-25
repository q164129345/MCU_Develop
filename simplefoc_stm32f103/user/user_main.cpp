#include "user_main.h"

/**
 * @brief C++环境入口函数
 * 
 */
void main_Cpp(void)
{
    while(1) {
        HAL_GPIO_TogglePin(run_led_GPIO_Port,run_led_Pin); // 心跳灯跑起来
        SEGGER_RTT_printf(0,"Dwt_time_us:%d\n",_micros()); // 打印DWT定时器的us级时间戳，看看有没有问题
        delayMicroseconds(100000U); // 延时100ms
    }
}











