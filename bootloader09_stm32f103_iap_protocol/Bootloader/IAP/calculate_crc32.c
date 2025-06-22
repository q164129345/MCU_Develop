#include "calculate_crc32.h"
#include "crc.h"

/**
 * @brief  计算固件缓冲区的CRC32（排除最后4字节）
 * @param  flash_addr     缓冲区起始地址（通常用FLASH_DL_START_ADDR）
 * @param  data_len       固件总长度（含末尾CRC32码）
 * @retval CRC32值（32位无符号整数）
 */
/**
  * @brief    使用 HAL 库计算固件 CRC32（排除末尾 4 B）
  * @param[in] flash_addr Flash 起始地址
  * @param[in] data_len   区域总长度（含 CRC32）
  * @retval   与 srec_cat -crc32-l-e 一致的小端 CRC32
  */
/**
  * @brief  计算固件 CRC32（排除尾部 4 B，结果与 srec_cat -crc32-l-e 一致）
  * @param  flash_addr  Flash 起始地址
  * @param  data_len    区域总长度（含 4 B CRC）
  * @return 小端 CRC32
  */
uint32_t Calculate_Firmware_CRC32(uint32_t flash_addr, uint32_t data_len)
{
    uint32_t calc_len = data_len - 4U;
    uint32_t words    = calc_len / 4U;
    uint32_t remain   = calc_len & 0x3U;

    __HAL_CRC_DR_RESET(&hcrc);

    const uint32_t *src = (uint32_t *)flash_addr;
    for (uint32_t i = 0; i < words; ++i) {
        uint32_t word = __RBIT(*src++);
        HAL_CRC_Accumulate(&hcrc, &word, 1);
    }

    if (remain) {
        uint32_t tail = 0xFFFFFFFFU;
        memcpy(&tail, (uint8_t *)src, remain);
        tail = __RBIT(tail);
        HAL_CRC_Accumulate(&hcrc, &tail, 1);
    }

    uint32_t crc_hw = hcrc.Instance->DR;      /* 旧版 HAL 无 GetValue() */
    return __RBIT(crc_hw) ^ 0xFFFFFFFFU;      /* RefOut + Final XOR */
}


