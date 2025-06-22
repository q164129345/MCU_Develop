/**
  ******************************************************************************
  * @file    fw_verify.h
  * @brief   对App下载缓存区的固件进行CRC32校验，确认固件完整性
  * @author  YourName
  * @date    2025-05-30
  ******************************************************************************
  */

#ifndef __FW_VERIFY_H
#define __FW_VERIFY_H

#include "main.h"
#include "flash_map.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
  * @brief  固件完整性CRC32校验函数
  */
HAL_StatusTypeDef FW_Firmware_Verification(uint32_t startAddr, uint32_t firmwareLen);

#ifdef __cplusplus
}
#endif

#endif // __FW_VERIFY_H
