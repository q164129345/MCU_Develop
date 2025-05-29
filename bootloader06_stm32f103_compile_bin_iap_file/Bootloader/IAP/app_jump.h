/**
  ******************************************************************************
  * @file    app_jump.h
  * @brief   Bootloader �� App ��ת�ӿ�����
  * @note    �����𡰼�� + ��ת���������̣����ں�������
  * @author  Wallace
  * @date    2025-05-26
  ******************************************************************************
  */

#ifndef __APP_JUMP_H
#define __APP_JUMP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "main.h"

/**
  * @brief  ��ȡ�̼�������־λ
  */
uint64_t IAP_GetUpdateFlag(void);

/**
  * @brief  ���ù̼�������־λ
  */
void IAP_SetUpdateFlag(uint64_t flag);

/**
  * @brief  ׼����ת��App����
  */
void IAP_Ready_To_Jump_App(void);

#ifdef __cplusplus
}
#endif

#endif /* __APP_JUMP_H */

