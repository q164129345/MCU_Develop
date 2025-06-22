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
  * @brief  Flash整区域擦除（按页为单位擦除指定区域）
  * @param  start_addr 起始地址（必须是有效的Flash地址，并且为页首地址）
  * @param  length     擦除长度（字节），建议为页大小的整数倍，不足时向上取整
  * @retval OP_FlashStatus_t 操作结果，成功返回 OP_FLASH_OK，失败返回错误码
  */
OP_FlashStatus_t OP_Flash_Erase(uint32_t start_addr, uint32_t length);

/**
  * @brief  Flash写入（以32位为单位，要求地址和数据4字节对齐）
  * @param  addr    目标地址，必须是有效Flash地址且4字节对齐
  * @param  data    数据指针，指向待写入的数据缓冲区
  * @param  length  写入长度（单位：字节），必须为4的整数倍
  * @retval OP_FlashStatus_t 操作结果，成功返回OP_FLASH_OK，失败返回错误码
  */
OP_FlashStatus_t OP_Flash_Write(uint32_t addr, uint8_t *data, uint32_t length);

/**
  * @brief  从源区拷贝数据到目标区（支持整区域拷贝，自动擦除目标区域）
  */
OP_FlashStatus_t OP_Flash_Copy(uint32_t src_addr, uint32_t dest_addr, uint32_t length);

#ifdef __cplusplus
}
#endif

#endif // __OP_FLASH_H
