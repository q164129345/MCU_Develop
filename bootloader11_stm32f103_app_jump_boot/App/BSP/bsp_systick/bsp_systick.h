/**
  ******************************************************************************
  * @file    bsp_systick.h
  * @brief   SysTick定时器底层驱动接口（支持1ms Tick、毫秒延时、兼容裸机/LL库项目）
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
  * @brief  SysTick定时器中断服务函数
  */
void SysTick_Interrupt(void);

/**
  * @brief  获取系统当前Tick，单位ms
  */
uint64_t SysTick_GetTicks(void);

/**
  * @brief  初始化SysTick定时器，实现1ms周期中断
  */
void SysTick_Init(void);


#ifdef __cplusplus
}
#endif

#endif // __BSP_SYSTICK_H