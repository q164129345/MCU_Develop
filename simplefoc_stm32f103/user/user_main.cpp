#include "user_main.h"

/**
 * @brief C++������ں���
 * 
 */
void main_Cpp(void)
{
    while(1) {
        HAL_GPIO_TogglePin(run_led_GPIO_Port,run_led_Pin); // ������������
        SEGGER_RTT_printf(0,"Dwt_time_us:%d\n",_micros()); // ��ӡDWT��ʱ����us��ʱ�����������û������
        delayMicroseconds(100000U); // ��ʱ100ms
    }
}











