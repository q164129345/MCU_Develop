/*
 * �汾��
 * ʱ�䣺2023/8/3:
   - ����DWT_Count_Timestamp_microsecond()������һ����ں�̨whlie��ѭ��û����ʱ����ѯ
   - ����DWT_Get_Microsecond�������������ⲿ�ṩus��ʱ���
   ʱ�䣺2024/12/4��
   - �Ż�������ɾ�����������Ϊ�޷������λ��Զ������������
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
