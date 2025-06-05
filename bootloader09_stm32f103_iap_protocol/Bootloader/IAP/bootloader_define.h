#ifndef __BOOTLOADER_DEFINE_H__
#define __BOOTLOADER_DEFINE_H__

/**
  * @def   __IS_COMPILER_ARM_COMPILER_5__
  * @brief ARMCC V5 ����������
  * @note
  *   - ����ǰ������Ϊ ARM Compiler 5��__ARMCC_VERSION >= 5000000 && < 6000000��������ú�Ϊ 1��
  */
#undef __IS_COMPILER_ARM_COMPILER_5__
#if ((__ARMCC_VERSION >= 5000000) && (__ARMCC_VERSION < 6000000))
#   define __IS_COMPILER_ARM_COMPILER_5__       1
#endif

/**
  * @def   __IS_COMPILER_ARM_COMPILER_6__
  * @brief ARMCC V6��armclang������������
  * @note
  *   - ����ǰ������Ϊ ARM Compiler 6 �����ϣ�armclang��__ARMCC_VERSION >= 6010050��������ú�Ϊ 1��
  */
#undef __IS_COMPILER_ARM_COMPILER_6__
#if defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
#   define __IS_COMPILER_ARM_COMPILER_6__       1
#endif

/**
 * ���̼����±�־������ŵ��ڴ��ַ��
 *  �̼���־λ��������Ϊ bootloader �� APP ֮��Ĺ�ͨ����
 */
#define FIRMWARE_UPDATE_VAR_ADDR           0x20000000      /**< һ��Ҫ�� APP ����һ�� */



#define FIRMWARE_UPDATE_MAGIC_WORD         0xA5A5A5A5      /**< �̼���Ҫ���µ������ǣ��������޸ģ�һ��Ҫ�� APP һ�£� */
#define BOOTLOADER_RESET_MAGIC_WORD        0xAAAAAAAA      /**< bootloader ��λ�������ǣ��������޸ģ�һ��Ҫ�� APP һ�£� */











#endif

