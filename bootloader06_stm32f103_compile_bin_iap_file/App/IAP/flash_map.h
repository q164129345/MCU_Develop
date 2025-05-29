/**
 * @file    flash_map.h
 * @brief   STM32F103ZET6 Flash������ַ���С��������
 * @author  Wallace.zhang
 * @date    2025-05-25
 * @version 1.0.0
 * @copyright
 * (C) 2025 Wallace.zhang. ��������Ȩ��.
 * @license SPDX-License-Identifier: MIT
 */

#ifndef __FLASH_MAP_H
#define __FLASH_MAP_H

#ifdef __cplusplus
extern "C" {
#endif

/** 
 * @brief STM32F103ZET6 Flash ��������
 */
#define STM32_FLASH_BASE_ADDR      0x08000000U      /**< Flash��ʼ����ַ */
#define STM32_FLASH_SIZE           (512 * 1024U)    /**< Flash�ܴ�С���ֽڣ� */
#define STM32_FLASH_PAGE_SIZE      (2 * 1024U)      /**< Flash��ҳ��С���ֽڣ� */

/**
 * @brief Bootloader��
 */
#define FLASH_BOOT_START_ADDR      0x08000000U      /**< Bootloader��ʼ��ַ */
#define FLASH_BOOT_END_ADDR        0x0800FFFFU      /**< Bootloader������ַ */
#define FLASH_BOOT_SIZE            (FLASH_BOOT_END_ADDR - FLASH_BOOT_START_ADDR + 1) /**< Bootloader����С */

/**
 * @brief ������App��
 */
#define FLASH_APP_START_ADDR       0x08010000U      /**< App��ʼ��ַ */
#define FLASH_APP_END_ADDR         0x0803FFFFU      /**< App������ַ */
#define FLASH_APP_SIZE             (FLASH_APP_END_ADDR - FLASH_APP_START_ADDR + 1)   /**< App����С */

/**
 * @brief App���������¹̼���������
 */
#define FLASH_DL_START_ADDR        0x08040000U      /**< ��������ʼ��ַ */
#define FLASH_DL_END_ADDR          0x0806FFFFU      /**< ������������ַ */
#define FLASH_DL_SIZE              (FLASH_DL_END_ADDR - FLASH_DL_START_ADDR + 1)     /**< ��������С */

/**
 * @brief ���������û���������ʷ���ݵȣ�
 */
#define FLASH_PARAM_START_ADDR     0x08070000U      /**< ��������ʼ��ַ */
#define FLASH_PARAM_END_ADDR       0x0807FFFFU      /**< ������������ַ */
#define FLASH_PARAM_SIZE           (FLASH_PARAM_END_ADDR - FLASH_PARAM_START_ADDR + 1) /**< ��������С */

#ifdef __cplusplus
}
#endif

#endif /* __FLASH_MAP_H */
