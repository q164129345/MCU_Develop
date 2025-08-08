#include "bsp_adc.h"

extern ADC_HandleTypeDef hadc1;

// 全局变量方便观察。以后当变量多了，后续可以用struct组织起来
volatile uint16_t gADC_IN10 = 0; // M0_B相
volatile uint16_t gADC_IN11 = 0; // M0_C相

/**
 * @brief function reading an ADC value and returning the read voltage
 * 
 * @param CH 通道号
 * @return float 返回电压值
 */
float _readADCVoltageInline(uint32_t CH)
{
    float adcValue = 0.0f;
    
    //! 根据CubeMX设置的INx通道来编写case
    switch(CH) {
        case ADC_CHANNEL_10:
            adcValue = (float)gADC_IN10 * 3.3f / 4096;
            break;
        case ADC_CHANNEL_11:
            adcValue = (float)gADC_IN11 * 3.3f / 4096;;
            break;
        default:
            break;
    }
    return adcValue;
}

/**
 * @brief function reading an ADC value and returning the read voltage
 * 
 * @param CH 通道号
 * @return float 返回电压值
 */
float _readADCVoltageLowSide(uint32_t CH)
{
    float adcValue = 0.0f;
    
    //! 根据CubeMX设置的INx通道来编写case
    switch(CH) {
        case ADC_CHANNEL_10:
            adcValue = (float)gADC_IN10 * 3.3f / 4096;
            break;
        case ADC_CHANNEL_11:
            adcValue = (float)gADC_IN11 * 3.3f / 4096;;
            break;
        default:
            break;
    }
    return adcValue;
}


/**
 * @brief 配置ADC低侧采样
 * 
 * @param hadc ADC句柄
 */
void _configureADCLowSide(ADC_HandleTypeDef *hadc)
{
    __HAL_ADC_ENABLE_IT(hadc, ADC_IT_JEOC); // 启动注入采样中断
    HAL_ADCEx_InjectedStart(hadc);          // 开启注入采样
}

/**
 * @brief 注入式ADC转换完成回调函数
 * @note 用于获取注入式ADC的转换结果
 * 
 * @param hadc 注入式ADC句柄
 */
void HAL_ADCEx_InjectedConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (&hadc1 == hadc) {
        gADC_IN10 = hadc->Instance->JDR1; // Injected Rank1
        gADC_IN11 = hadc->Instance->JDR2; // Injected Rank2
    }
}

/**
 * @brief 初始化ADC
 * @note 初始化ADC，并开启注入采样中断
 * 
 */
void ADC_Init(void)
{
    __HAL_ADC_ENABLE_IT(&hadc1,ADC_IT_JEOC); // 启动注入采样中断
    HAL_ADCEx_InjectedStart(&hadc1);         // 开启注入采样
}

