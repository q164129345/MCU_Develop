#include "app_jump.h"
#include "stm32f1xx.h"      /**< 根据芯片型号修改 */
#include "flash_map.h"      /**< Flash分区宏定义 */
#include "usart.h"
#include "retarget_rtt.h"
#include "bootloader_define.h"


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
  * @brief  去初始化Bootloader已用外设，避免影响App运行
  * @note
  *   - 复位系统时钟为默认HSI
  *   - 关闭全局中断与SysTick
  *   - 去初始化USART1及其DMA
  *   - 清除所有NVIC中断和挂起标志
  *   - 跳转App前必须调用本函数
  */
//void IAP_DeInitHardware(void)
//{
//    /*! 系统时钟复位为默认状态（使用HSI） */
//    HAL_RCC_DeInit();
//   
//    /*! 关闭SysTick定时器 */
//    SysTick->CTRL = 0;
//    SysTick->LOAD = 0;
//    SysTick->VAL  = 0;

//    /*! bootloader程序初始化的外设，全部都要DeInit，否则会影响App的运行 */
//    /*! 本实例的bootloader只用到DMA与USART1 */
//    /*! 串口去初始化 */
//    HAL_UART_DeInit(&huart1);

//    /*! DMA去初始化 */
//    HAL_DMA_DeInit(huart1.hdmarx);
//    HAL_DMA_DeInit(huart1.hdmatx);

//    /*! NVIC中断彻底关闭和清挂起 */
//    for (uint8_t i = 0; i < 8; ++i) {
//        NVIC->ICER[i] = 0xFFFFFFFF;
//        NVIC->ICPR[i] = 0xFFFFFFFF;
//    }

//    //__enable_irq();   /*!< 重新打开中断（跳转前可不使能） */
//}

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
    void (*AppEntry)(void);           /**< 应用入口函数指针 */
    __IO uint32_t AppAddr = app_addr; /**< 跳转目标地址 */
    
    /*! 关闭全局中断 */
    __disable_irq();
    
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
    SCB->VTOR = AppAddr;
    
    log_printf("Jump to App, MSP and VTOR = %d, AppEntry = %d\n", AppAddr, (AppAddr + 4));
    
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

