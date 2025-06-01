#include "fw_verify.h"
#include "soft_crc32.h"

volatile unsigned long gCalc = 0;
volatile unsigned long gStored = 0;

/**
  * @brief  �̼�������CRC32У�麯��
  * @param  startAddr    �̼���ʼ��ַ��Flash��RAM���׵�ַ��
  * @param  firmwareLen  �̼��ܳ��ȣ���λ���ֽڣ���Ӧ����CRC32У���루���4�ֽڣ�
  * @retval HAL_StatusTypeDef
  *         - HAL_OK    ��У��ͨ�����̼���������
  *         - HAL_ERROR ��У��ʧ�ܣ��̼����ƻ������ݲ�һ��
  *
  * @note
  *       - �������ٶ��̼���������4���ֽ�ΪCRC32У��ֵ���������CRC32�㷨����̼����岿�֣��������4�ֽڣ���
  *         Ȼ����ĩβ�洢��CRC32���бȶԡ�
  *       - һ������Bootloader����ǰ/����ǰ�̼���������֤������
  *       - У��ʧ�ܽ����ֹ��ת��App���������ʧ����ʾ��
  *       - ��־�������У������CRC32�룬�����ڵ�������Դ��
  */
HAL_StatusTypeDef FW_Firmware_Verification(uint32_t startAddr, uint32_t firmwareLen)
{
    gCalc   = Calculate_Firmware_CRC32_SW(startAddr, firmwareLen - 4u); //! ���4���ֽ���CRC32�룬����Ҫ��CRC32����
    gStored = *(uint32_t *)(startAddr + firmwareLen - 4u);

    if (gCalc == gStored) {
        log_printf("CRC32 verification was successful. CRC:0x%08lX\r\n", gCalc);
        return HAL_OK; //!< У��ͨ��
    } else {
        log_printf("CRC32 verification was failure: calc=0x%08lX, stored=0x%08lX\r\n", gCalc, gStored);
        return HAL_ERROR;  //!< У��ʧ��
    }
}

