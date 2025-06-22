/**
  ******************************************************************************
  * @file    op_flash.h
  * @brief   STM32F103 Flash操作模块头文件（支持分区拷贝，读/写/擦除）
  * @author  YourName
  * @date    2025-05-30
  ******************************************************************************
  */

#ifndef __OP_FLASH_H
#define __OP_FLASH_H

#include "main.h"
#include "flash_map.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
  * @brief  Flash操作结果枚举
  */
typedef enum
{
    OP_FLASH_OK = 0U,      /*!< 操作成功 */
    OP_FLASH_ERROR,        /*!< 操作失败 */
    OP_FLASH_ADDR_INVALID, /*!< 地址非法 */
} OP_FlashStatus_t;

/**
  * @brief  从源区拷贝数据到目标区（支持整区域拷贝，自动擦除目标区域）
  */
OP_FlashStatus_t OP_Flash_Copy(uint32_t src_addr, uint32_t dest_addr, uint32_t length);

#ifdef __cplusplus
}
#endif

#endif // __OP_FLASH_H
