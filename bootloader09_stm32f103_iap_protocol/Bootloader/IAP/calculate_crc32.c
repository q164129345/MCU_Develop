#include "calculate_crc32.h"
#include "crc.h"

/**
 * @brief  ����̼���������CRC32���ų����4�ֽڣ�
 * @param  flash_addr     ��������ʼ��ַ��ͨ����FLASH_DL_START_ADDR��
 * @param  data_len       �̼��ܳ��ȣ���ĩβCRC32�룩
 * @retval CRC32ֵ��32λ�޷���������
 */
/**
  * @brief    ʹ�� HAL �����̼� CRC32���ų�ĩβ 4 B��
  * @param[in] flash_addr Flash ��ʼ��ַ
  * @param[in] data_len   �����ܳ��ȣ��� CRC32��
  * @retval   �� srec_cat -crc32-l-e һ�µ�С�� CRC32
  */
/**
  * @brief  ����̼� CRC32���ų�β�� 4 B������� srec_cat -crc32-l-e һ�£�
  * @param  flash_addr  Flash ��ʼ��ַ
  * @param  data_len    �����ܳ��ȣ��� 4 B CRC��
  * @return С�� CRC32
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

    uint32_t crc_hw = hcrc.Instance->DR;      /* �ɰ� HAL �� GetValue() */
    return __RBIT(crc_hw) ^ 0xFFFFFFFFU;      /* RefOut + Final XOR */
}


