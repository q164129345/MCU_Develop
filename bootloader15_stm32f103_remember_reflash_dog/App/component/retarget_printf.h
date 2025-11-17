// retarget_printf.h
#ifndef __RETARGET_PRINTF_H
#define __RETARGET_PRINTF_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/* 日志控制宏 - 设置为1启用日志，设置为0禁用日志 */
#ifndef LOG_ENABLE
#define LOG_ENABLE 1  /* 默认启用日志输出 */
#endif

#if LOG_ENABLE
/**
  * @brief  格式化打印调试信息（带函数名与行号）
  * @note   用法与printf一致，输出自动包含当前函数名和行号
  *         当LOG_ENABLE为1时，正常输出日志
  */
#define log_printf(fmt, ...) \
    printf("%s-%s(%d):" fmt, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
/**
  * @brief  空的日志宏（禁用状态）
  * @note   当LOG_ENABLE为0时，log_printf不会产生任何输出，也不会占用运行时间
  */
#define log_printf(fmt, ...) ((void)0)
#endif

/**
  * @brief  重定向printf输出到USART2
  */
int _write(int file, char *ptr, int len);


#ifdef __cplusplus
}
#endif

#endif
