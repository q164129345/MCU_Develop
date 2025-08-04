/**
 * @file    jump_boot.h
 * @brief   应用程序与Bootloader跳转功能的头文件声明
 * @author  Wallace.zhang
 * @date    2025-05-25
 * @version 1.0.0
 * @copyright
 * (C) 2025 Wallace.zhang. 保留所有权利.
 * @license SPDX-License-Identifier: MIT
 */

#ifndef __JUMP_BOOT_H
#define __JUMP_BOOT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "main.h"
#include <stdbool.h>

/**
  * @brief  解析串口接收到的数据
  */
void IAP_Parse_Command(uint8_t data);



#ifdef __cplusplus
}
#endif

#endif /* __JUMP_BOOT_H */
