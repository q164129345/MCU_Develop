/*
 * 版本：
 * 时间：2023/8/3:
   - 增加DWT_Count_Timestamp_microsecond()方法，一般放在后台whlie大循环没有延时地轮询
   - 增加DWT_Get_Microsecond（）方法，给外部提供us级时间戳
   时间：2024/12/4：
   - 优化函数，删除溢出处理，因为无符号整形会自动处理溢出问题
 *
 */

#ifndef _DWT_TIMER_H_
#define _DWT_TIMER_H_

#include <stdint.h>
#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif


// public
HAL_StatusTypeDef DWT_Timer_Init(void);
uint32_t DWT_Get_CNT(void);
void DWT_Delay_us(uint32_t us);
uint32_t DWT_Get_Microsecond(void);
uint32_t DWT_Get_System_Clock_Freq(void);

    
// private


#ifdef __cplusplus
} 
#endif

#endif
