// retarget_rtt.h
#ifndef __RETARGET_RTT_H
#define __RETARGET_RTT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/**
  * @brief  格式化打印调试信息（带函数名与行号）
  * @note   用法与printf一致，输出自动包含当前函数名和行号
  */
#define log_printf(fmt, ...) \
printf("%s-%s(%d):" fmt, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)

/**
  * @brief  重定向printf输出到RTT通道0
  */
int fputc(int ch, FILE *f);

/**
  * @brief  初始化SEGGER RTT重定向（上行缓冲区配置）
  */
void Retarget_RTT_Init(void);


#ifdef __cplusplus
}
#endif

#endif
