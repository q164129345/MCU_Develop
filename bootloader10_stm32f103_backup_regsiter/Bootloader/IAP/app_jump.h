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
  * @brief  获取固件升级标志位
  */
uint64_t IAP_GetUpdateFlag(void);

/**
  * @brief  设置固件升级标志位
  */
void IAP_SetUpdateFlag(uint64_t flag);

/**
  * @brief  准备跳转到App程序
  */
void IAP_Ready_To_Jump_App(void);

#ifdef __cplusplus
}
#endif

#endif /* __APP_JUMP_H */

