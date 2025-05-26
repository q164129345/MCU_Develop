/**
  ******************************************************************************
  * @file    app_jump.h
  * @brief   Bootloader → App 跳转接口声明
  * @note    仅负责“检测 + 跳转”核心流程，便于后续复用
  * @author  Wallace
  * @date    2025-05-26
  ******************************************************************************
  */

#ifndef __APP_JUMP_H
#define __APP_JUMP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "main.h"

/**
  * @brief  跳转到指定应用入口（不返回）
  * @param  app_addr  应用向量表地址
  * @note   仅在 Boot_CheckApp() = true 时调用
  */
void IAP_JumpToApp(uint32_t app_addr);

#ifdef __cplusplus
}
#endif

#endif /* __APP_JUMP_H */

