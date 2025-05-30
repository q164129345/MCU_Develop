/**
  ******************************************************************************
  * @file    op_flash.h
  * @brief   STM32F103 Flash����ģ��ͷ�ļ���֧�ַ�����������/д/������
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
  * @brief  Flash�������ö��
  */
typedef enum
{
    OP_FLASH_OK = 0U,      /*!< �����ɹ� */
    OP_FLASH_ERROR,        /*!< ����ʧ�� */
    OP_FLASH_ADDR_INVALID, /*!< ��ַ�Ƿ� */
} OP_FlashStatus_t;

/**
  * @brief  ��Դ���������ݵ�Ŀ������֧�������򿽱����Զ�����Ŀ������
  * @param  src_addr    Դ����ʼ��ַ
  * @param  dest_addr   Ŀ������ʼ��ַ
  * @param  length      �������ȣ��ֽڣ���Ϊ4�ֽڶ��룩
  * @retval OP_FlashStatus_t  �������
  */
OP_FlashStatus_t OP_Flash_Copy(uint32_t src_addr, uint32_t dest_addr, uint32_t length);

#ifdef __cplusplus
}
#endif

#endif // __OP_FLASH_H