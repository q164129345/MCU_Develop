#include "app_jump.h"
#include "stm32f1xx.h"      /**< ����оƬ�ͺ��޸� */
#include "flash_map.h"      /**< Flash�����궨�� */
#include "usart.h"
#include "retarget_rtt.h"
#include "bootloader_define.h"

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
void IAP_JumpToApp(uint32_t app_addr)
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

