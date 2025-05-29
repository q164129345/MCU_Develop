/**
  ******************************************************************************
  * @file    bsp_systick.h
  * @brief   SysTick��ʱ���ײ������ӿڣ�֧��1ms Tick��������ʱ���������/LL����Ŀ��
  * @author  WallaceZhang
  * @date    2025-05-27
  ******************************************************************************
  */

#ifndef __BSP_SYSTICK_H
#define __BSP_SYSTICK_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"


/**
  * @brief  SysTick��ʱ���жϷ�����
  */
void SysTick_Interrupt(void);

/**
  * @brief  ��ȡϵͳ��ǰTick����λms
  */
uint64_t SysTick_GetTicks(void);

/**
  * @brief  ��ʼ��SysTick��ʱ����ʵ��1ms�����ж�
  */
void SysTick_Init(void);


#ifdef __cplusplus
}
#endif

#endif // __BSP_SYSTICK_H