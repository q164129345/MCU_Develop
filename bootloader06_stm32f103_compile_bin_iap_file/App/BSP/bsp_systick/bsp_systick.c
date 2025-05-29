#include "bsp_systick/bsp_systick.h"

volatile uint64_t uwTick; //!< ȫ��Tick����

/**
  * @brief  ��ȡϵͳ��ǰTick����λms
  * @retval uint64_t ��ǰTick����
  */
uint64_t SysTick_GetTicks(void)
{
    return uwTick;
}

/**
  * @brief  SysTick��ʱ���жϷ�����
  * @note   ÿ1ms����һ�θ��жϣ�Tick��������
  * @retval ��
  */
void SysTick_Interrupt(void)
{
    uwTick++;
}

/**
  * @brief  ��ʼ��SysTick��ʱ����ʵ��1ms�����ж�
  * @note   ���ú�SysTick���Զ�ÿ1ms����һ���ж�
  * @retval ��
  */
void SysTick_Init(void)
{
    SysTick_Config(SystemCoreClock / 1000); //! ��ʼ��SysTick,1ms�ж�
}









