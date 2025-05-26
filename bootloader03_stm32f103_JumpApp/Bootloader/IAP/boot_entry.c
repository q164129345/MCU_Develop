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
    /*! �ڶ��ν��룬��ת��App */
    if (IAP_GetUpdateFlag() == BOOTLOADER_RESET_MAGIC_WORD) {
        log_printf("The second time enter this function, clean the flag and then Jump to Application.\n");
        /*! ����̼����±�־λ */
        IAP_SetUpdateFlag(0);
        
        /*! ��תApp */
        IAP_JumpToApp(FLASH_APP_START_ADDR);
    } else {
        log_printf("The first time enter this function, clean up the MCU environment");
        /*! ���ñ�־λ */
        IAP_SetUpdateFlag(BOOTLOADER_RESET_MAGIC_WORD);
        
        /*! ϵͳ��λ�����½���bootloader */
        HAL_NVIC_SystemReset();
    }
}

/**
  * @brief  ϵͳ���ڳ�ʼ���ص���main()ǰ�Զ����ã�
  * @note
  *   - ������ͨ�� GCC/ARMCC �� @c __attribute__((constructor)) �������Σ�ϵͳ������main()ִ��ǰ�Զ����С�
  *   - �����ڽ���������־�ض��򡢻�����⡢�̼�������У�顢������־�жϵ�ȫ�ֳ�ʼ��������
  *   - ����Ŀ��չ���������Ʊ��������ݣ���������ò����������ʼ���Ĺؼ����롣
  *
  * @attention
  *   - ʹ�� @c __attribute__((constructor)) ����Ҫ�󹤳����ӽű�/�����ļ���ȷ֧�� .init_array �Ρ�
  *   - ���������������ű���֧�ָû��ƣ��뽫�ú��������ֶ����� main() ��ǰ����á�
  *
  * @see    Retarget_RTT_Init(), log_printf()
  */
__attribute__((constructor))
static void _SystemStart(void)
{
    uint64_t flag = IAP_GetUpdateFlag();
    Retarget_RTT_Init(); //! RTT�ض���printf
    
    log_printf("System Start!\n");
    
    /*! ���̼����±�־ */
    if (flag == BOOTLOADER_RESET_MAGIC_WORD) {
        IAP_Ready_To_Jump_App();
    }
    
    IAP_SetUpdateFlag(0);
}

