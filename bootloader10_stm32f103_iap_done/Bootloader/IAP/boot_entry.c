#include "boot_entry.h"
#include "flash_map.h"
#include "app_jump.h"
#include "bootloader_define.h"

/**
 * @brief   更完善的App有效性检查
 */
static bool Is_App_Valid_Enhanced(void)
{
    uint32_t app_stack_ptr = *(volatile uint32_t*)FLASH_APP_START_ADDR; //! 获取栈指针
    uint32_t app_reset_vec = *(volatile uint32_t*)(FLASH_APP_START_ADDR + 4); //! 获取复位向量
    //! 检查1：栈指针必须在RAM范围内
    if ((app_stack_ptr < 0x20000000) || (app_stack_ptr > 0x20020000)) {
        return false; //! 栈指针不在RAM范围内
    }
    //! 检查2：复位向量必须是Thumb指令（奇数）
    if ((app_reset_vec & 1) == 0) {
        return false; //! 复位向量不是Thumb指令
    }
    //! 检查3：复位向量必须在Flash范围内
    if ((app_reset_vec < FLASH_APP_START_ADDR) || 
        (app_reset_vec > (FLASH_APP_START_ADDR + FLASH_APP_SIZE))) {
        return false; //! 复位向量不在Flash范围内
    }
    //! 检查4：不能是空Flash的特征值
    if ((app_stack_ptr == 0xFFFFFFFF) || (app_reset_vec == 0xFFFFFFFF)) {
        return false; //! 栈指针或复位向量是空Flash的特征值
    }
    //! 可选：检查5：CRC校验（最可靠但最慢）
    //! if (!FW_Verify_CRC32(FLASH_APP_START_ADDR, app_size)) {
    //!     return false;
    //! }
    return true;
}

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
  * @see    log_printf()
  */
__attribute__((constructor))
static void _SystemStart(void)
{
    //! 不要在构造函数中调用RTT重定向printf，例如调用Retarget_RTT_Init()。
    //! 只要在bootloader或者App的main()中调用Retarget_RTT_Init()。这个函数里的log_printf()会正常打印出来。
    uint64_t flag = IAP_GetUpdateFlag();

    log_printf("flag = 0x%08X%08X\n", (uint32_t)(flag >> 32), (uint32_t)flag);
    //! 决策逻辑:
    //! 1. App请求IAP?
    //! 2. Bootloader内部跳转?
    //! 3. 默认启动App?
    //! 4. 否则, 进入IAP模式
    if (flag == FIRMWARE_UPDATE_MAGIC_WORD) {
        log_printf("Upgrade mode.\r\n");
        IAP_SetUpdateFlag(0); //! 清除固件更新标志
        //! 不做任何事情，直接返回，让程序执行到main(),运行bootlaoder程序，进入IAP流程
        return;
    } else if (flag == BOOTLOADER_RESET_MAGIC_WORD) {
        //! --- Case 2: Bootloader 内部跳转流程 ---
        //! 这是 IAP_Ready_To_Jump_App 的第二步，会直接跳转
        IAP_Ready_To_Jump_App(); // 此函数不会返回
    } else {
        //! --- Case 3: 默认启动App ---
        IAP_SetUpdateFlag(0); // 清除可能存在的无效标志
        if (Is_App_Valid_Enhanced()) {
            //log_printf("App is valid, jump to app.\n");
            IAP_Ready_To_Jump_App(); // 此函数不会返回
        } else {
            log_printf("App is invalid, enter IAP mode.\r\n");
            //! 不做任何事，直接返回，让程序执行到main(),进入IAP流程
            return;
        }
    }
}

