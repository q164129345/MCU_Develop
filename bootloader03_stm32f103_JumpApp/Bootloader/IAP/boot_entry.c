#include "boot_entry.h"
#include "bootloader_define.h"
#include "flash_map.h"
#include "app_jump.h"


#if defined(__IS_COMPILER_ARM_COMPILER_5__)
volatile uint64_t update_flag __attribute__((at(FIRMWARE_UPDATE_VAR_ADDR), zero_init));

#elif defined(__IS_COMPILER_ARM_COMPILER_6__)
    #define __INT_TO_STR(x)     #x
    #define INT_TO_STR(x)       __INT_TO_STR(x)
    volatile uint64_t update_flag __attribute__((section(".bss.ARM.__at_" INT_TO_STR(FIRMWARE_UPDATE_VAR_ADDR))));

#else
    #error "variable placement not supported for this compiler."
#endif

uint64_t IAP_GetUpdateFlag(void)
{
    return update_flag;
}

void IAP_SetUpdateFlag(uint64_t flag)
{
    update_flag = flag;
}


void IAP_Ready_To_Jump_App(void)
{
    /*! 第二次进入，跳转至App */
    if (IAP_GetUpdateFlag() == BOOTLOADER_RESET_MAGIC_WORD) {
        log_printf("The second time enter this function, clean the flag and then Jump to Application.\n");
        /*! 清除固件更新标志位 */
        IAP_SetUpdateFlag(0);
        
        /*! 跳转App */
        IAP_JumpToApp(FLASH_APP_START_ADDR);
    } else {
        log_printf("The first time enter this function, clean up the MCU environment");
        /*! 设置标志位 */
        IAP_SetUpdateFlag(BOOTLOADER_RESET_MAGIC_WORD);
        
        /*! 系统复位，重新进入bootloader */
        HAL_NVIC_SystemReset();
    }
}

/**
  * @brief  系统早期初始化回调（main()前自动调用）
  * @note
  *   - 本函数通过 GCC/ARMCC 的 @c __attribute__((constructor)) 属性修饰，系统启动后、main()执行前自动运行。
  *   - 适用于进行早期日志重定向、环境检测、固件完整性校验、启动标志判断等全局初始化操作。
  *   - 随项目进展，可逐步完善本函数内容，建议仅放置不依赖外设初始化的关键代码。
  *
  * @attention
  *   - 使用 @c __attribute__((constructor)) 机制要求工程链接脚本/启动文件正确支持 .init_array 段。
  *   - 若编译器或启动脚本不支持该机制，请将该函数内容手动放入 main() 最前面调用。
  *
  * @see    Retarget_RTT_Init(), log_printf()
  */
__attribute__((constructor))
static void _SystemStart(void)
{
    uint64_t flag = IAP_GetUpdateFlag();
    Retarget_RTT_Init(); //! RTT重定向printf
    
    log_printf("System Start!\n");
    
    /*! 检查固件更新标志 */
    if (flag == BOOTLOADER_RESET_MAGIC_WORD) {
        IAP_Ready_To_Jump_App();
    }
    
    IAP_SetUpdateFlag(0);
}

