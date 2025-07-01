#include "bootloader_define.h"
#include "flash_map.h"
#include "jump_boot.h"


#if defined(__IS_COMPILER_ARM_COMPILER_5__)
volatile uint64_t update_flag __attribute__((at(FIRMWARE_UPDATE_VAR_ADDR), zero_init));

#elif defined(__IS_COMPILER_ARM_COMPILER_6__)
    #define __INT_TO_STR(x)     #x
    #define INT_TO_STR(x)       __INT_TO_STR(x)
    volatile uint64_t update_flag __attribute__((section(".bss.ARM.__at_" INT_TO_STR(FIRMWARE_UPDATE_VAR_ADDR))));

#else
    #error "variable placement not supported for this compiler."
#endif

/**
  * @brief  获取固件升级标志位
  * @note   读取保存于指定RAM地址的升级标志变量（通常用于判断bootloader的运行状态）
  * @retval uint64_t 固件升级标志的当前值
  */
static uint64_t IAP_GetUpdateFlag(void)
{
    return update_flag;
}

/**
  * @brief  设置固件升级标志位
  * @param  flag 需要设置的标志值
  * @note   修改指定RAM地址的升级标志变量
  * @retval 无
  */
static void IAP_SetUpdateFlag(uint64_t flag)
{
    update_flag = flag;
}

/**
  * @brief  解析串口接收到的数据
  * @note   根据接收到的数据，判断是否需要跳转到Bootloader
  *         目标字符串: "A5A5A5A5" (8字节)
  *         ASCII: 0x41,0x35,0x41,0x35,0x41,0x35,0x41,0x35
  * @param  data 接收到的数据
  * @retval 无
  */
void IAP_Parse_Command(uint8_t data)
{
    // 目标字符串"A5A5A5A5"的ASCII码序列
    static const uint8_t target_sequence[8] = {
        0x41, 0x35, 0x41, 0x35,  // "A5A5"
        0x41, 0x35, 0x41, 0x35   // "A5A5"
    };
    
    // 静态变量：记录当前匹配的字节位置
    static uint8_t match_index = 0;
    
    // 检查当前字节是否与目标序列匹配
    if (data == target_sequence[match_index]) {
        match_index++;  // 匹配成功，移动到下一个位置
        
        // 检查是否接收完整的8字节序列
        if (match_index >= sizeof(target_sequence)) {
            // 完整匹配成功，跳转到Bootloader
            match_index = 0;  // 重置状态，为下次做准备
            //! 设置固件升级标志位
            IAP_SetUpdateFlag(FIRMWARE_UPDATE_MAGIC_WORD);
            //! 等待10ms，确保标志位设置成功
            LL_mDelay(10);
            //！此函数不会返回，MCU将复位
            NVIC_SystemReset();
        }
    } else {
        // 不匹配，重置状态机
        match_index = 0;
        
        // 特殊处理：如果当前字节恰好是序列的第一个字节，则开始新的匹配
        if (data == target_sequence[0]) {
            match_index = 1;
        }
    }
}
















