/**
 * @file    boot_entry.h
 * @brief   Bootloader启动入口模块头文件
 * @author  Wallace.zhang
 * @date    2025-06-27
 * @version 1.0.0
 * 
 * @details
 * 本模块负责系统启动时的决策逻辑，包括：
 * - 固件更新标志检测
 * - App有效性验证
 * - 启动模式选择（App模式 vs IAP模式）
 * - 系统早期初始化
 * 
 * @note
 * 该模块使用 __attribute__((constructor)) 机制实现系统启动前的自动初始化，
 * 确保在main()函数执行前完成关键的启动决策。
 * 
 * @copyright
 * (C) 2025 Wallace.zhang. 保留所有权利.
 * @license SPDX-License-Identifier: MIT
 */

#ifndef __BOOT_ENTRY_H
#define __BOOT_ENTRY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"


#ifdef __cplusplus
}
#endif

#endif /* __BOOT_ENTRY_H */
