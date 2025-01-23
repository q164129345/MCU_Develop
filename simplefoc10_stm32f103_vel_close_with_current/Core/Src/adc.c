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
// ȫ�ֱ�������۲졣�Ժ󵱱������ˣ�����������struct��֯����
volatile uint32_t gChannel3 = 0; // A��
volatile uint32_t gChannel4 = 0; // B��
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
  HAL_ADCEx_Calibration_Start(&hadc1); // ����ADC1У׼
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
  * @brief  ��ȡADC������ͨ��������CubeMX����Scanģʽ��
  * @param  hadc  ָ��ADC������� &hadc1��
  * @retval HAL_StatusTypeDef ����HAL_OK��ʾ�ɹ�������Ϊ����״̬
  */
HAL_StatusTypeDef ADC_StartReadVoltageFromChannels(void)
{
    HAL_StatusTypeDef status;
    // ����ADC
    status = HAL_ADC_Start(&hadc1);
    if (status != HAL_OK) {
        return status;  // ����ʧ����ֱ�ӷ���
    }

    // 2. �ȴ�����ȡ��1ͨ����Rank1 --> Channel3��
    status = HAL_ADC_PollForConversion(&hadc1, 10); // ��ʱ10ms���ɸ����������
    if (status == HAL_OK) {
        gChannel3 = HAL_ADC_GetValue(&hadc1); // ��ȡ��1��ת�����
    } else {
        // ��ȡʧ�ܣ��ǵ�ֹͣADC
        HAL_ADC_Stop(&hadc1);
        return status;
    }
    // ����ADC
    status = HAL_ADC_Start(&hadc1);
    if (status != HAL_OK) {
        return status;  // ����ʧ����ֱ�ӷ���
    }
    // 3. �ȴ�����ȡ��2ͨ����Rank2 --> Channel4��
    status = HAL_ADC_PollForConversion(&hadc1, 10);
    if (status == HAL_OK) {
        gChannel4 = HAL_ADC_GetValue(&hadc1); // ��ȡ��2��ת�����
    } else {
        HAL_ADC_Stop(&hadc1);
        return status;
    }
    // ���������д������ͨ�������������������������Ļ�
    // ֹͣADC
    status = HAL_ADC_Stop(&hadc1);
    return status;  // һ��ɺ��ԣ��������ж��Ƿ�Stop�ɹ���Ҳ�ɼ�鷵��ֵ
}

/**
 * @brief ��ȡADC��ֵ
 * 
 * @param CH ͨ����
 * @return uint32_t ����ADCֵ
 */
static uint32_t adcRead(uint32_t CH) 
{
    // ����ͨ���ŷ��ض�Ӧ��ADCֵ����ʱֻ�õ�ͨ��3��ͨ��4��������������
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
 * @param CH ͨ����
 * @return float ���ص�ѹֵ
 */
float _readADCVoltageInline(uint32_t CH)
{
    uint16_t adcValue = adcRead(CH);
    return (float)adcValue * 3.3f / 4096.0f;
}

/* USER CODE END 1 */
