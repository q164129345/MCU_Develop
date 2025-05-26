/**
 * @file    flash_map.h
 * @brief   STM32F103ZET6 Flash分区地址与大小常量定义
 * @author  Wallace.zhang
 * @date    2025-05-25
 * @version 1.0.0
 * @copyright
 * (C) 2025 Wallace.zhang. 保留所有权利.
 * @license SPDX-License-Identifier: MIT
 */

#ifndef __FLASH_MAP_H
#define __FLASH_MAP_H

#ifdef __cplusplus
extern "C" {
#endif

/** 
 * @brief STM32F103ZET6 Flash 基础参数
 */
#define STM32_FLASH_BASE_ADDR      0x08000000U      /**< Flash起始基地址 */
#define STM32_FLASH_SIZE           (512 * 1024U)    /**< Flash总大小（字节） */
#define STM32_FLASH_PAGE_SIZE      (2 * 1024U)      /**< Flash单页大小（字节） */

/**
 * @brief Bootloader区
 */
#define FLASH_BOOT_START_ADDR      0x08000000U      /**< Bootloader起始地址 */
#define FLASH_BOOT_END_ADDR        0x0800FFFFU      /**< Bootloader结束地址 */
#define FLASH_BOOT_SIZE            (FLASH_BOOT_END_ADDR - FLASH_BOOT_START_ADDR + 1) /**< Bootloader区大小 */

/**
 * @brief 主程序App区
 */
#define FLASH_APP_START_ADDR       0x08010000U      /**< App起始地址 */
#define FLASH_APP_END_ADDR         0x0803FFFFU      /**< App结束地址 */
#define FLASH_APP_SIZE             (FLASH_APP_END_ADDR - FLASH_APP_START_ADDR + 1)   /**< App区大小 */

/**
 * @brief App缓存区（新固件下载区）
 */
#define FLASH_DL_START_ADDR        0x08040000U      /**< 下载区起始地址 */
#define FLASH_DL_END_ADDR          0x0806FFFFU      /**< 下载区结束地址 */
#define FLASH_DL_SIZE              (FLASH_DL_END_ADDR - FLASH_DL_START_ADDR + 1)     /**< 下载区大小 */

/**
 * @brief 参数区（用户参数、历史数据等）
 */
#define FLASH_PARAM_START_ADDR     0x08070000U      /**< 参数区起始地址 */
#define FLASH_PARAM_END_ADDR       0x0807FFFFU      /**< 参数区结束地址 */
#define FLASH_PARAM_SIZE           (FLASH_PARAM_END_ADDR - FLASH_PARAM_START_ADDR + 1) /**< 参数区大小 */

#ifdef __cplusplus
}
#endif

#endif /* __FLASH_MAP_H */
