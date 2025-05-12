#include "myTIM6Drive/myTIM6Drive_reg.h"
#include "myUsartDrive/myUsartDrive_reg.h"

/**
  * @brief  TIM6 �Ĵ�������ʼ��
  * @note   - ʹ��ʱ�ӣ�RCC->APB1ENR.TIM6EN (bit4)  
  *         - ����Ԥ��Ƶ�� PSC = 71����Ӧ 72 ��Ƶ��ʹ�ö�ʱ������ʱ�� = Fpclk1/72  
  *         - �����Զ���װ�� ARR = 999���������Ƶ�� = ʱ��/1000 = 1 kHz  
  *         - ʹ�� ARR Ԥװ�أ�CR1.ARPE (bit7)  
  * @retval None
  */
void TIM6_RegInit(void)
{
    /* 1. ʹ�� TIM6 ʱ�� */
    RCC->APB1ENR |= RCC_APB1ENR_TIM6EN;  
    /* 2. ���� PSC �� ARR */
    TIM6->PSC = 71;      /* PSC �Ĵ���������ʱ�� = PCLK1/(PSC+1) */
    TIM6->ARR = 999;     /* ARR �Ĵ����������� ARR ����������¼� */
    /* 3. ʹ�� ARR Ԥװ�� */
    TIM6->CR1 |= TIM_CR1_ARPE;  

    /* 4. �������������ģʽ����Ĭ�ϣ�RESET/�رգ� */
    /*    TIM6->CR2 Ĭ�� TRGO=000��MSM=0 */
}

/**
  * @brief  ���� TIM6 ��ʹ�ܸ����ж�
  * @note   - ������ܵ� UIF ������־��SR.UIF (bit0) д 0 ���  
  *         - ʹ�ܸ����жϣ�DIER.UIE (bit0) =1  
  *         - ͨ�� NVIC ʹ�� TIM6/DAC �ж�  
  *         - ����������CR1.CEN (bit0)=1  
  * @retval None
  */
void TIM6_RegStartWithIRQ(void)
{
    /* 1. ��������жϱ�־����������ж��������� */
    TIM6->SR &= ~TIM_SR_UIF;
    /* 2. ʹ�ܸ����ж� */
    TIM6->DIER |= TIM_DIER_UIE;
    /* 3. NVIC ���ã�ʹ�� TIM6_DAC �жϣ����ȼ� PL=2, SP=0 */
    /*    CMSIS �ṩ�� NVIC_SetPriority �� NVIC_EnableIRQ Ҳ���� */
    NVIC_SetPriority(TIM6_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),2, 0));
    NVIC_EnableIRQ(TIM6_IRQn);
    /* 4. ������ʱ�� */
    TIM6->CR1 |= TIM_CR1_CEN;
}

/**
  * @brief  TIM6 �����жϷ�����
  * @note   ���ʹ�� DAC��TIM6 �� DAC �����жϣ���ͬʱ�ж�
  */
void TIM6_IRQHandler(void)
{
    /* ��� UIF ��־ (�����ж�) */
    if (TIM6->SR & TIM_SR_UIF) {
        /* ��� UIF ��־ */
        TIM6->SR &= ~TIM_SR_UIF;
        /* �ڴ���� 1 kHz ����ʱ�Ĵ������ */
        const char *msg = "Hello,World\n";
        uint16_t status = USART1_Put_TxData_To_Ringbuffer(msg, strlen(msg));
    }
}


























