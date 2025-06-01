#include "fw_verify.h"
#include "soft_crc32.h"

volatile unsigned long gCalc = 0;
volatile unsigned long gStored = 0;

/**
  * @brief  固件完整性CRC32校验函数
  * @param  startAddr    固件起始地址（Flash或RAM区首地址）
  * @param  firmwareLen  固件总长度（单位：字节），应包含CRC32校验码（最后4字节）
  * @retval HAL_StatusTypeDef
  *         - HAL_OK    ：校验通过，固件数据完整
  *         - HAL_ERROR ：校验失败，固件被破坏或数据不一致
  *
  * @note
  *       - 本函数假定固件镜像的最后4个字节为CRC32校验值，采用软件CRC32算法计算固件主体部分（不含最后4字节）；
  *         然后与末尾存储的CRC32进行比对。
  *       - 一般用于Bootloader升级前/启动前固件完整性验证场景。
  *       - 校验失败建议禁止跳转到App或给出升级失败提示。
  *       - 日志输出包含校验结果与CRC32码，有助于调试与溯源。
  */
HAL_StatusTypeDef FW_Firmware_Verification(uint32_t startAddr, uint32_t firmwareLen)
{
    gCalc   = Calculate_Firmware_CRC32_SW(startAddr, firmwareLen - 4u); //! 最后4个字节是CRC32码，不需要做CRC32运算
    gStored = *(uint32_t *)(startAddr + firmwareLen - 4u);

    if (gCalc == gStored) {
        log_printf("CRC32 verification was successful. CRC:0x%08lX\r\n", gCalc);
        return HAL_OK; //!< 校验通过
    } else {
        log_printf("CRC32 verification was failure: calc=0x%08lX, stored=0x%08lX\r\n", gCalc, gStored);
        return HAL_ERROR;  //!< 校验失败
    }
}

