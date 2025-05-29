/**
 * @file    soft_crc32.h
 * @brief   纯软件 CRC-32/ISO-HDLC 计算（RefIn/RefOut = 1）
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
 * @brief   初始化 CRC 上下文
 * @return  初始值 0xFFFF?FFFF
 */
static inline uint32_t CRC32_Init(void) { return 0xFFFFFFFFu; }

/**
 * @brief   输入若干字节并更新 CRC
 */
uint32_t CRC32_Update(uint32_t crc, const void *buf, uint32_t len);

/**
 * @brief   计算完成后取反得到最终 CRC
 */
static inline uint32_t CRC32_Final(uint32_t crc) { return crc ^ 0xFFFFFFFFu; }

/**
 * @brief   便捷接口：一次性计算整个缓冲区
 */
static inline uint32_t CRC32_Calc(const void *buf, uint32_t len)
{
    return CRC32_Final(CRC32_Update(CRC32_Init(), buf, len));
}

/**
  * @brief   计算Flash中固件数据的CRC32值（软件方式）
  */
uint32_t Calculate_Firmware_CRC32_SW(uint32_t flash_addr, uint32_t total_len);

#ifdef __cplusplus
}
#endif
#endif /* __SOFT_CRC32_H */

