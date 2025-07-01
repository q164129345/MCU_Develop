#ifndef __BOOTLOADER_DEFINE_H__
#define __BOOTLOADER_DEFINE_H__

/**
  * @def   __IS_COMPILER_ARM_COMPILER_5__
  * @brief ARMCC V5 编译器检测宏
  * @note
  *   - 若当前编译器为 ARM Compiler 5（__ARMCC_VERSION >= 5000000 && < 6000000），则定义该宏为 1。
  */
#undef __IS_COMPILER_ARM_COMPILER_5__
#if ((__ARMCC_VERSION >= 5000000) && (__ARMCC_VERSION < 6000000))
#   define __IS_COMPILER_ARM_COMPILER_5__       1
#endif

/**
  * @def   __IS_COMPILER_ARM_COMPILER_6__
  * @brief ARMCC V6（armclang）编译器检测宏
  * @note
  *   - 若当前编译器为 ARM Compiler 6 及以上（armclang，__ARMCC_VERSION >= 6010050），则定义该宏为 1。
  */
#undef __IS_COMPILER_ARM_COMPILER_6__
#if defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
#   define __IS_COMPILER_ARM_COMPILER_6__       1
#endif

/**
 * 【固件更新标志变量存放的内存地址】
 *  固件标志位变量将作为 bootloader 和 APP 之间的沟通桥梁
 */
#define FIRMWARE_UPDATE_VAR_ADDR           0x20000000      /**< 一定要和 APP 保持一致 */



#define FIRMWARE_UPDATE_MAGIC_WORD         0xA5A5A5A5      /**< 固件需要更新的特殊标记（不建议修改，一定要和 APP 一致） */
#define BOOTLOADER_RESET_MAGIC_WORD        0xAAAAAAAA      /**< bootloader 复位的特殊标记（不建议修改，一定要和 APP 一致） */











#endif

