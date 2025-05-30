/**
 * @file    soft_crc32.h
 * @brief   ����� CRC-32/ISO-HDLC ���㣨RefIn/RefOut = 1��
 * @author  Wallace
 * @date    2025-05-29
 * @version 1.0.0
 */

#ifndef __SOFT_CRC32_H
#define __SOFT_CRC32_H

#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   ��ʼ�� CRC ������
 * @return  ��ʼֵ 0xFFFF?FFFF
 */
static inline uint32_t CRC32_Init(void) { return 0xFFFFFFFFu; }

/**
 * @brief   ���������ֽڲ����� CRC
 */
uint32_t CRC32_Update(uint32_t crc, const void *buf, uint32_t len);

/**
 * @brief   ������ɺ�ȡ���õ����� CRC
 */
static inline uint32_t CRC32_Final(uint32_t crc) { return crc ^ 0xFFFFFFFFu; }

/**
 * @brief   ��ݽӿڣ�һ���Լ�������������
 */
static inline uint32_t CRC32_Calc(const void *buf, uint32_t len)
{
    return CRC32_Final(CRC32_Update(CRC32_Init(), buf, len));
}

/**
  * @brief   ����Flash�й̼����ݵ�CRC32ֵ�������ʽ��
  */
uint32_t Calculate_Firmware_CRC32_SW(uint32_t flash_addr, uint32_t total_len);

#ifdef __cplusplus
}
#endif
#endif /* __SOFT_CRC32_H */

