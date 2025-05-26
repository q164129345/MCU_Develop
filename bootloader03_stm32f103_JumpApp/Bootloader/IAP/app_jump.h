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
  * @brief  ��ת��ָ��Ӧ����ڣ������أ�
  * @param  app_addr  Ӧ���������ַ
  * @note   ���� Boot_CheckApp() = true ʱ����
  */
void IAP_JumpToApp(uint32_t app_addr);

#ifdef __cplusplus
}
#endif

#endif /* __APP_JUMP_H */

