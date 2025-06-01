/**
  ******************************************************************************
  * @file    fw_verify.h
  * @brief   ��App���ػ������Ĺ̼�����CRC32У�飬ȷ�Ϲ̼�������
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
  * @brief  �̼�������CRC32У�麯��
  */
HAL_StatusTypeDef FW_Firmware_Verification(uint32_t startAddr, uint32_t firmwareLen);

#ifdef __cplusplus
}
#endif

#endif // __FW_VERIFY_H
