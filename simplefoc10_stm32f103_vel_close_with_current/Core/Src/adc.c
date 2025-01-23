/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    adc.c
  * @brief   This file provides code for the configuration
  *          of the ADC instances.
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
/* Includes ------------------------------------------------------------------*/
#include "adc.h"

/* USER CODE BEGIN 0 */
// 全局变量方便观察。以后当变量多了，后续可以用struct组织起来
volatile uint32_t gChannel3 = 0; // A相
volatile uint32_t gChannel4 = 0; // B相
/* USER CODE END 0 */

ADC_HandleTypeDef hadc1;

/* ADC1 init function */
void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = ENABLE;
  hadc1.Init.NbrOfDiscConversion = 1;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 2;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_3;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_7CYCLES_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_4;
  sConfig.Rank = ADC_REGULAR_RANK_2;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */
  HAL_ADCEx_Calibration_Start(&hadc1); // 启动ADC1校准
  /* USER CODE END ADC1_Init 2 */

}

void HAL_ADC_MspInit(ADC_HandleTypeDef* adcHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(adcHandle->Instance==ADC1)
  {
  /* USER CODE BEGIN ADC1_MspInit 0 */

  /* USER CODE END ADC1_MspInit 0 */
    /* ADC1 clock enable */
    __HAL_RCC_ADC1_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**ADC1 GPIO Configuration
    PA3     ------> ADC1_IN3
    PA4     ------> ADC1_IN4
    */
    GPIO_InitStruct.Pin = GPIO_PIN_3|GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN ADC1_MspInit 1 */

  /* USER CODE END ADC1_MspInit 1 */
  }
}

void HAL_ADC_MspDeInit(ADC_HandleTypeDef* adcHandle)
{

  if(adcHandle->Instance==ADC1)
  {
  /* USER CODE BEGIN ADC1_MspDeInit 0 */

  /* USER CODE END ADC1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_ADC1_CLK_DISABLE();

    /**ADC1 GPIO Configuration
    PA3     ------> ADC1_IN3
    PA4     ------> ADC1_IN4
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_3|GPIO_PIN_4);

  /* USER CODE BEGIN ADC1_MspDeInit 1 */

  /* USER CODE END ADC1_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */
/**
  * @brief  读取ADC的两个通道（已在CubeMX配置Scan模式）
  * @param  hadc  指向ADC句柄（如 &hadc1）
  * @retval HAL_StatusTypeDef 返回HAL_OK表示成功，否则为错误状态
  */
HAL_StatusTypeDef ADC_StartReadVoltageFromChannels(void)
{
    HAL_StatusTypeDef status;
    // 启动ADC
    status = HAL_ADC_Start(&hadc1);
    if (status != HAL_OK) {
        return status;  // 启动失败则直接返回
    }

    // 2. 等待并读取第1通道（Rank1 --> Channel3）
    status = HAL_ADC_PollForConversion(&hadc1, 10); // 超时10ms，可根据需求调整
    if (status == HAL_OK) {
        gChannel3 = HAL_ADC_GetValue(&hadc1); // 读取第1个转换结果
    } else {
        // 读取失败，记得停止ADC
        HAL_ADC_Stop(&hadc1);
        return status;
    }
    // 启动ADC
    status = HAL_ADC_Start(&hadc1);
    if (status != HAL_OK) {
        return status;  // 启动失败则直接返回
    }
    // 3. 等待并读取第2通道（Rank2 --> Channel4）
    status = HAL_ADC_PollForConversion(&hadc1, 10);
    if (status == HAL_OK) {
        gChannel4 = HAL_ADC_GetValue(&hadc1); // 读取第2个转换结果
    } else {
        HAL_ADC_Stop(&hadc1);
        return status;
    }
    // 这里可以填写第三个通道，如果板子有三个采样电阻的话
    // 停止ADC
    status = HAL_ADC_Stop(&hadc1);
    return status;  // 一般可忽略，但如需判断是否Stop成功，也可检查返回值
}

/**
 * @brief 读取ADC的值
 * 
 * @param CH 通道号
 * @return uint32_t 返回ADC值
 */
static uint32_t adcRead(uint32_t CH) 
{
    // 根据通道号返回对应的ADC值，暂时只用到通道3与通道4，后续按需增加
    switch (CH)
    {
    case ADC_CHANNEL_3:
        return gChannel3;
      break;
    case ADC_CHANNEL_4:
        return gChannel4;
      break;
    default:
        return 0;
      break;
    }
}

/**
 * @brief function reading an ADC value and returning the read voltage
 * 
 * @param CH 通道号
 * @return float 返回电压值
 */
float _readADCVoltageInline(uint32_t CH)
{
    uint16_t adcValue = adcRead(CH);
    return (float)adcValue * 3.3f / 4096.0f;
}

/* USER CODE END 1 */
