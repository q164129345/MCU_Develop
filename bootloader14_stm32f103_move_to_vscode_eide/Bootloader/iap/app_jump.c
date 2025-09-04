#include "app_jump.h"
#include "stm32f1xx.h"      /**< 根据芯片型号修改 */
#include "flash_map.h"      /**< Flash分区宏定义 */
#include "usart.h"
#include "bootloader_define.h"


#if defined(__IS_COMPILER_ARM_COMPILER_5__)
volatile uint64_t update_flag __attribute__((at(FIRMWARE_UPDATE_VAR_ADDR), zero_init));

#elif defined(__IS_COMPILER_ARM_COMPILER_6__)
#define __INT_TO_STR(x)     #x
#define INT_TO_STR(x)       __INT_TO_STR(x)
volatile uint64_t update_flag __attribute__((section(".bss.ARM.__at_" INT_TO_STR(FIRMWARE_UPDATE_VAR_ADDR))));

#elif defined(__GNUC__)
/* GCC编译器: 使用.noinit段放置在RAM起始地址，程序初始化时不被清零 */
volatile uint64_t update_flag __attribute__((section(".noinit")));

#else
    #error "variable placement not supported for this compiler."
#endif

/**
  * @brief  验证APP是否有效
  * @param  base APP基地址
  * @retval 1=有效, 0=无效
  */
static int IAP_IsValidApp(uint32_t base)
{
    uint32_t stack = *(uint32_t *)base;        // 栈顶指针
    uint32_t reset = *(uint32_t *)(base + 4);  // Reset_Handler地址

    // 检查栈顶指针范围 (STM32F103ZET6: 64KB RAM)
    if (stack < 0x20000000U) {
        return 0;
    }
    
    // 检查栈顶是否8字节对齐
    if ((stack & 0x7U) != 0) {
        return 0;
    }
    
    // 检查Reset_Handler地址：必须是Thumb指令(LSB=1)且在FLASH范围内
    if ((reset & 0x1U) == 0) {
        return 0;
    }
    
    if (reset < base || reset > FLASH_APP_END_ADDR) {
        return 0;
    }
    
    return 1;
}

/**
  * @brief  跳转到指定应用程序入口（不返回）
  * @param  app_addr  应用程序向量表地址（通常为App分区起始地址）
  * @note
  *   - 跳转前自动去初始化所有已用外设与中断
  *   - 自动设置MSP，切换特权级模式
  *   - 重定位中断向量表SCB->VTOR到App起始地址
  *   - 跳转后本函数不返回，若跳转失败将死循环
  */
static void IAP_JumpToApp(uint32_t app_addr)
{   
    // 验证APP有效性
    if (!IAP_IsValidApp(app_addr)) {
        // APP无效，停止跳转
        while (1) {
            // 可在此处添加错误指示（LED闪烁等）
        }
    }
    
    /*! 关闭全局中断 */
    __disable_irq();

    // 防御式清 SysTick & NVIC
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL  = 0;
    for (uint32_t i = 0; i < 8; i++) {
        NVIC->ICER[i] = 0xFFFFFFFF;
        NVIC->ICPR[i] = 0xFFFFFFFF;
    }   

    void (*AppEntry)(void);           /**< 应用入口函数指针 */
    __IO uint32_t AppAddr = app_addr; /**< 跳转目标地址 */
  
    
    /*! 不需要deinit外设了，本实例使用"无deinit方案，核心是“软复位 + 标志再入跳转。一切资源全部归零，极大简化维护，提高可靠性
     *  这是现代Bootloader设计的推荐实践
     */
    /*! 跳转App前，将外设复位。为App提供一个干净的环境 */
    //IAP_DeInitHardware();

    /*! 获取App复位向量（入口地址） */
    AppEntry = (void (*)(void)) (*((uint32_t *) (AppAddr + 4)));

    /*! 设置主堆栈指针MSP */
    __set_MSP(*(uint32_t *)AppAddr);

    /*! 切换为特权级模式（使用MSP），RTOS场景下尤为重要 */
    __set_CONTROL(0);

    /*! 告诉CPU，App的中断向量表地址 */
    /* 在GCC编译器下，不能在bootloader设置App的中断向量表。否则会进入硬件中断回调，不会跳转App程序。这个问题排查了一整天！
     * 在Keil的编译器，没有这个问题，可能是编译器的差异导致的。
     * 经过测试，GCC下不设置VTOR也能跳转成功。但，必须在App的启动代码（例如system_stm32f1xx.c的函数SystemInit())设置VTOR。
     */
    //SCB->VTOR = AppAddr;
    
    /*! 跳转至应用程序入口，函数不再返回 */
    AppEntry();
    
    /*!
      @attention 若跳转失败，可在此增加错误处理（一般不会执行到这里）
    */
    while (1)
    {
        /*! 错误保护 */
    }
}

/**
  * @brief  获取固件升级标志位
  * @note   读取保存于指定RAM地址的升级标志变量（通常用于判断bootloader的运行状态）
  * @retval uint64_t 固件升级标志的当前值
  */
uint64_t IAP_GetUpdateFlag(void)
{
    return update_flag;
}

/**
  * @brief  设置固件升级标志位
  * @param  flag 需要设置的标志值
  * @note   修改指定RAM地址的升级标志变量
  * @retval 无
  */
void IAP_SetUpdateFlag(uint64_t flag)
{
    update_flag = flag;
}

/**
  * @brief  准备跳转到App程序
  * @note   判断当前升级标志，第一次调用时先设置标志并复位MCU，第二次调用时清除标志并跳转到App
  * @retval 无
  * @attention 
  *          - 此函数用于bootloader到应用程序的跳转流程控制。
  *          - 必须保证FLASH_APP_START_ADDR为合法的应用程序入口地址。
  *          - 跳转前应正确清理MCU相关外设状态，防止异常影响应用程序。
  */
void IAP_Ready_To_Jump_App(void)
{
    /*! 第二次进入，跳转至App */
    if (IAP_GetUpdateFlag() == BOOTLOADER_RESET_MAGIC_WORD) {
        //log_printf("The second time enter this function, clean the flag and then Jump to Application.\n");
        
        /*! 保证RTT打印完log(延迟约1ms) */
        Delay_MS_By_NOP(1);
        
        /*! 清除固件更新标志位 */
        IAP_SetUpdateFlag(0);
        
        /*! 跳转App */
        IAP_JumpToApp(FLASH_APP_START_ADDR);
    } else {
        //log_printf("The first time enter this function, clean up the MCU environment.\n");
        
        /*! 保证RTT打印完log(延迟约1ms) */
        Delay_MS_By_NOP(1);
        
        /*! 设置标志位 */
        IAP_SetUpdateFlag(BOOTLOADER_RESET_MAGIC_WORD);
        
        /*! 系统复位，重新进入bootloader */
        HAL_NVIC_SystemReset();
    }
}

