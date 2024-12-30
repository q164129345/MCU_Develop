/*
 *
 */

#ifndef _BSP_PWM_H_
#define _BSP_PWM_H_

#include "main.h"
#include "tim.h"

#ifdef __cplusplus
extern "C" {
#endif


// public
void _writeDutyCycle3PWM(float dc_a, float dc_b, float dc_c);


// private


#ifdef __cplusplus
} 
#endif

#endif
