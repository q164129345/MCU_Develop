#include "boot_entry.h"
#include "flash_map.h"
#include "app_jump.h"

/**
  * @brief  上电，系统早期初始化回调（main()前自动调用）
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
    
    /*! 检查复位更新标志 */
    if (flag == BOOTLOADER_RESET_MAGIC_WORD) {
        IAP_Ready_To_Jump_App(); //! 清理MCU环境，准备跳转App程序
    }

    /*! 后续添加升级模式判断 */
    
    
    /*! 清除固件更新标志 */
    IAP_SetUpdateFlag(0);
}

