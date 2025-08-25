#ifndef _BSP_ADC_H_
#define _BSP_ADC_H_

#include "main.h"
#include "adc.h"

#ifdef __cplusplus
extern "C" {
#endif


// public
float _readADCVoltageInline(uint32_t CH);
float _readADCVoltageLowSide(uint32_t CH);
void _configureADCLowSide(ADC_HandleTypeDef *hadc);

// private


#ifdef __cplusplus
} 
#endif

#endif
