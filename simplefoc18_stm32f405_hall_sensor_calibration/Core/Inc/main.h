/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "SEGGER_RTT.h"
#include "dwt_timer.h"
#include "math.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
#define delayMicroseconds DWT_Delay_us // 替代Arduino的us级延时
#define delay _delay

// 定义一个宏，将SIMPLEFOC_DEBUG替换为SEGGER_RTT_printf
#define SIMPLEFOC_DEBUG(fmt, ...) SEGGER_RTT_printf(0, "[%s] " fmt "\n", __FUNCTION__, ##__VA_ARGS__)
/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */
void SEGGER_Printf_Float(float value);
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define M0_ENC_C_Pin GPIO_PIN_9
#define M0_ENC_C_GPIO_Port GPIOC
#define M0_ENC_C_EXTI_IRQn EXTI9_5_IRQn
#define RUN_LED_Pin GPIO_PIN_2
#define RUN_LED_GPIO_Port GPIOD
#define M0_ENC_A_Pin GPIO_PIN_4
#define M0_ENC_A_GPIO_Port GPIOB
#define M0_ENC_A_EXTI_IRQn EXTI4_IRQn
#define M0_ENC_B_Pin GPIO_PIN_5
#define M0_ENC_B_GPIO_Port GPIOB
#define M0_ENC_B_EXTI_IRQn EXTI9_5_IRQn

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
