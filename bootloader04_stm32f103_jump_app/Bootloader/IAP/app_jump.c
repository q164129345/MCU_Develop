#include "app_jump.h"
#include "stm32f1xx.h"      /**< ����оƬ�ͺ��޸� */
#include "flash_map.h"      /**< Flash�����궨�� */
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
  * @brief  ȥ��ʼ��Bootloader�������裬����Ӱ��App����
  * @note
  *   - ��λϵͳʱ��ΪĬ��HSI
  *   - �ر�ȫ���ж���SysTick
  *   - ȥ��ʼ��USART1����DMA
  *   - �������NVIC�жϺ͹����־
  *   - ��תAppǰ������ñ�����
  */
//void IAP_DeInitHardware(void)
//{
//    /*! ϵͳʱ�Ӹ�λΪĬ��״̬��ʹ��HSI�� */
//    HAL_RCC_DeInit();
//   
//    /*! �ر�SysTick��ʱ�� */
//    SysTick->CTRL = 0;
//    SysTick->LOAD = 0;
//    SysTick->VAL  = 0;

//    /*! bootloader�����ʼ�������裬ȫ����ҪDeInit�������Ӱ��App������ */
//    /*! ��ʵ����bootloaderֻ�õ�DMA��USART1 */
//    /*! ����ȥ��ʼ�� */
//    HAL_UART_DeInit(&huart1);

//    /*! DMAȥ��ʼ�� */
//    HAL_DMA_DeInit(huart1.hdmarx);
//    HAL_DMA_DeInit(huart1.hdmatx);

//    /*! NVIC�жϳ��׹رպ������ */
//    for (uint8_t i = 0; i < 8; ++i) {
//        NVIC->ICER[i] = 0xFFFFFFFF;
//        NVIC->ICPR[i] = 0xFFFFFFFF;
//    }

//    //__enable_irq();   /*!< ���´��жϣ���תǰ�ɲ�ʹ�ܣ� */
//}

/**
  * @brief  ��ת��ָ��Ӧ�ó�����ڣ������أ�
  * @param  app_addr  Ӧ�ó����������ַ��ͨ��ΪApp������ʼ��ַ��
  * @note
  *   - ��תǰ�Զ�ȥ��ʼ�����������������ж�
  *   - �Զ�����MSP���л���Ȩ��ģʽ
  *   - �ض�λ�ж�������SCB->VTOR��App��ʼ��ַ
  *   - ��ת�󱾺��������أ�����תʧ�ܽ���ѭ��
  */
static void IAP_JumpToApp(uint32_t app_addr)
{
    void (*AppEntry)(void);           /**< Ӧ����ں���ָ�� */
    __IO uint32_t AppAddr = app_addr; /**< ��תĿ���ַ */
    
    /*! �ر�ȫ���ж� */
    __disable_irq();
    
    /*! ����Ҫdeinit�����ˣ���ʵ��ʹ��"��deinit�����������ǡ���λ + ��־������ת��һ����Դȫ�����㣬�����ά������߿ɿ���
     *  �����ִ�Bootloader��Ƶ��Ƽ�ʵ��
     */
    /*! ��תAppǰ�������踴λ��ΪApp�ṩһ���ɾ��Ļ��� */
    //IAP_DeInitHardware();

    /*! ��ȡApp��λ��������ڵ�ַ�� */
    AppEntry = (void (*)(void)) (*((uint32_t *) (AppAddr + 4)));

    /*! ��������ջָ��MSP */
    __set_MSP(*(uint32_t *)AppAddr);

    /*! �л�Ϊ��Ȩ��ģʽ��ʹ��MSP����RTOS��������Ϊ��Ҫ */
    __set_CONTROL(0);

    /*! ����CPU��App���ж��������ַ */
    SCB->VTOR = AppAddr;
    
    log_printf("Jump to App, MSP and VTOR = %d, AppEntry = %d\n", AppAddr, (AppAddr + 4));
    
    /*! ��ת��Ӧ�ó�����ڣ��������ٷ��� */
    AppEntry();
    
    /*!
      @attention ����תʧ�ܣ����ڴ����Ӵ�����һ�㲻��ִ�е����
    */
    while (1)
    {
        /*! ���󱣻� */
    }
}

/**
  * @brief  ��ȡ�̼�������־λ
  * @note   ��ȡ������ָ��RAM��ַ��������־������ͨ�������ж�bootloader������״̬��
  * @retval uint64_t �̼�������־�ĵ�ǰֵ
  */
uint64_t IAP_GetUpdateFlag(void)
{
    return update_flag;
}

/**
  * @brief  ���ù̼�������־λ
  * @param  flag ��Ҫ���õı�־ֵ
  * @note   �޸�ָ��RAM��ַ��������־����
  * @retval ��
  */
void IAP_SetUpdateFlag(uint64_t flag)
{
    update_flag = flag;
}

/**
  * @brief  ׼����ת��App����
  * @note   �жϵ�ǰ������־����һ�ε���ʱ�����ñ�־����λMCU���ڶ��ε���ʱ�����־����ת��App
  * @retval ��
  * @attention 
  *          - �˺�������bootloader��Ӧ�ó������ת���̿��ơ�
  *          - ���뱣֤FLASH_APP_START_ADDRΪ�Ϸ���Ӧ�ó�����ڵ�ַ��
  *          - ��תǰӦ��ȷ����MCU�������״̬����ֹ�쳣Ӱ��Ӧ�ó���
  */
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

