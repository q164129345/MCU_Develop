/**
 * @file    bootloader_define.h
 * @brief   Bootloader核心定义与配置常量
 * @author  Wallace.zhang
 * @date    2025-06-27
 * @version 1.0.0
 * 
 * @details
 * 本头文件定义了Bootloader系统的核心常量和配置参数，主要包括：
 * - 编译器兼容性检测宏定义
 * - 固件更新标志位的内存地址配置
 * - Bootloader与App程序间通信的魔术字定义
 * - 系统启动模式控制常量
 * 
 * @note
 * 关键设计要点：
 * - 固件更新标志变量地址必须与App程序保持严格一致
 * - 魔术字用于区分不同的启动模式和跳转流程
 * - 支持ARM Compiler 5和6的变量定位语法差异
 * 
 * @attention
 * - FIRMWARE_UPDATE_VAR_ADDR地址必须位于RAM区域且与App程序协调
 * - 魔术字定义一旦确定不建议随意修改，需与App程序同步更新
 * - 编译器检测宏用于处理不同版本ARM编译器的语法差异
 * 
 * @copyright
 * (C) 2025 Wallace.zhang. 保留所有权利.
 * @license SPDX-License-Identifier: MIT
 */

#ifndef __BOOTLOADER_DEFINE_H__
#define __BOOTLOADER_DEFINE_H__

/**
  * @def   __IS_COMPILER_ARM_COMPILER_5__
  * @brief ARMCC V5 编译器检测宏
  * @note
  *   - 若当前编译器为 ARM Compiler 5（__ARMCC_VERSION >= 5000000 && < 6000000），则定义该宏为 1。
  */
#undef __IS_COMPILER_ARM_COMPILER_5__
#if ((__ARMCC_VERSION >= 5000000) && (__ARMCC_VERSION < 6000000))
#   define __IS_COMPILER_ARM_COMPILER_5__       1
#endif

/**
  * @def   __IS_COMPILER_ARM_COMPILER_6__
  * @brief ARMCC V6（armclang）编译器检测宏
  * @note
  *   - 若当前编译器为 ARM Compiler 6 及以上（armclang，__ARMCC_VERSION >= 6010050），则定义该宏为 1。
  */
#undef __IS_COMPILER_ARM_COMPILER_6__
#if defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
#   define __IS_COMPILER_ARM_COMPILER_6__       1
#endif

/**
 * 【固件更新标志变量存放的内存地址】
 *  固件标志位变量将作为 bootloader 和 APP 之间的沟通桥梁
 */
#define FIRMWARE_UPDATE_VAR_ADDR           0x20000000      /**< 一定要和 APP 保持一致 */



#define FIRMWARE_UPDATE_MAGIC_WORD         0xA5A5A5A5      /**< 固件需要更新的特殊标记（不建议修改，一定要和 APP 一致） */
#define BOOTLOADER_RESET_MAGIC_WORD        0xAAAAAAAA      /**< bootloader 复位的特殊标记（不建议修改，一定要和 APP 一致） */











#endif

