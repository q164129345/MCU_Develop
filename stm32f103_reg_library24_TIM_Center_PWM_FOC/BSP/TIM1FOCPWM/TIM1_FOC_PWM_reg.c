#include "TIM1FOCPWM/TIM1_FOC_PWM_reg.h"

/**
  * @brief  �� TIM1 ���� remap �� PE7/PE8~PE15�������� PE8~PE13 Ϊ
  *         TIM1_CH1N/CH1, CH2N/CH2, CH3N/CH3
  */
static void TIM1_GPIO_Init(void)
{
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_GPIOE);
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_AFIO);
    /**TIM1 GPIO Configuration
        PE8     ------> TIM1_CH1N
        PE9     ------> TIM1_CH1
        PE10     ------> TIM1_CH2N
        PE11     ------> TIM1_CH2
        PE12     ------> TIM1_CH3N
        PE13     ------> TIM1_CH3
        */
    GPIO_InitStruct.Pin = LL_GPIO_PIN_8|LL_GPIO_PIN_9|LL_GPIO_PIN_10|LL_GPIO_PIN_11
                              |LL_GPIO_PIN_12|LL_GPIO_PIN_13;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_MEDIUM;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    LL_GPIO_Init(GPIOE, &GPIO_InitStruct);
    LL_GPIO_AF_EnableRemap_TIM1();
}

/**
  * @brief  ���� TIM1 ������ʱ�䣨Dead-Time Generator��
  * @param  dtg ����������ֵ��0��255������Ӧ BDTR.DTG[7:0]
  *            - �� dtg < 128 ʱ��DeadTime �� dtg �� T_DTS
  *            - ���ڵ��� 128 �����ģʽ2/3�����賬����������
  * @note   ʹ��ǰ��ȷ�� TIM1_BDTR.MOE = 1���Ҷ�ʱ����ʹ��ʱ��
  */
static void TIM1_SetDeadTime(uint8_t dtg)
{
    /* 1. ����ɵ� DTG �ֶ� */
    TIM1->BDTR &= ~TIM_BDTR_DTG;
    /* 2. д���µ�����ֵ */
    TIM1->BDTR |= (uint32_t)dtg;
}

/**
  * @brief  ������ʱ���ĸ����¼����ȼ����� EGR �Ĵ���д�� UG λ��
  * @note   EGR �Ĵ���д 1 �����¼���д 0 ��Ч�������ֱ�Ӹ�ֵ�� SET_BIT ����
  */
__STATIC_INLINE void TIM1_GenerateEvent_UPDATE(void)
{
    /* ��ʽһ��ֱ�Ӹ�ֵ������ UG */
    TIM1->EGR = TIM_EGR_UG;
    
    /*  
    // ���߷�ʽ����ʹ�� CMSIS ����λ
    SET_BIT(TIMx->EGR, TIM_EGR_UG);
    */
}

/**
  * @brief      ���� TIM1 ͨ�� 1/2/3 �ıȽ�ֵ�������������¼�ʹ��ֵ������Ч
  * @param[in]  CH1CompareValue  ͨ�� 1 �ıȽ�ֵ��д�� CCR1��
  * @param[in]  CH2CompareValue  ͨ�� 2 �ıȽ�ֵ��д�� CCR2��
  * @param[in]  CH3CompareValue  ͨ�� 3 �ıȽ�ֵ��д�� CCR3��
  * @note       д�� CCRx ����Ҫ���� TIM1_GenerateEvent_UPDATE()  
  *             �� EGR �Ĵ���д�� UG λ�����ܽ�Ԥװ�صıȽ�ֵˢ�µ�ʵ�ʼ����Ƚϵ�Ԫ
  */
void TIM1_Set_Compare_Value(uint32_t CH1CompareValue, uint32_t CH2CompareValue, uint32_t CH3CompareValue)
{
    TIM1->CCR1 = CH1CompareValue;
    TIM1->CCR2 = CH2CompareValue;
    TIM1->CCR3 = CH3CompareValue;
    TIM1_GenerateEvent_UPDATE(); // ��ʱ���±Ƚ�ֵ
}

/**
  * @brief  ���� TIM1����ʼ��� PWM
  * @note   �����´���һ�θ����¼���ȷ��Ԥװ��ֵд��������Ч��  
  *         �����´��������ʹ PWM �˿���Ч��
  */
void TIM1_PWM_Start(void)
{
    /* 1. ���������¼���ˢ�� ARR/CCR */
    TIM1->EGR = TIM_EGR_UG;
    /* 2. ʹ������� */
    TIM1->BDTR |= TIM_BDTR_MOE;
    /* 3. ʹ�ܶ�ʱ������ */
    TIM1->CR1 |= TIM_CR1_CEN;
}

/**
  * @brief  ֹͣ TIM1 PWM ���
  * @note   �رռ�������ֹ�������ȷ�� PWM ����˿�ʧЧ
  */
void TIM1_PWM_Stop(void)
{
    /* 1. ��ֹ��ʱ������ */
    TIM1->CR1 &= ~TIM_CR1_CEN;
    /* 2. ��������� */
    TIM1->BDTR &= ~TIM_BDTR_MOE;
}

/**
  * @brief  ���ڼĴ�����ʼ�� TIM1 ���� 3 · PWM ��������
  * @param  period   ��ʱ����װ��ֵ (ARR)������ PWM ����
  * @param  duty1    ͨ�� 1 �ıȽ�ֵ (CCR1)������ռ�ձ�
  * @param  duty2    ͨ�� 2 �ıȽ�ֵ (CCR2)
  * @param  duty3    ͨ�� 3 �ıȽ�ֵ (CCR3)
  * @note   ���� PWM ģʽ 3��������������Զ����ء��ȽϼĴ�������Ԥװ�أ��ߵ�ƽ��Ч
  */
void TIM1_PWM_Init(uint16_t period,
                   uint16_t duty1,
                   uint16_t duty2,
                   uint16_t duty3)
{
    /* 1. ʹ��ʱ�ӣ�TIM1��GPIOE��AFIO */
    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;
    
    /* 2. ��ʼ��GPIO */
    TIM1_GPIO_Init();
    
    /* 3. ʱ���׼���ã��޷�Ƶ���������� */
    TIM1->PSC = 0;               /* PSC = 0������Ƶ 1 */
    TIM1->ARR = period;          /* �Զ�����ֵ */
    TIM1->CR1 |= TIM_CR1_ARPE    /* ARR Ԥװ�� */
               | TIM_CR1_CMS_0;  /* CMS = 01�����Ķ���ģʽ1 */

    /* 4. PWM ģʽ2 + Ԥװ�� (OCxM = 111, OCxPE = 1) */
    TIM1->CCMR1 &= ~(TIM_CCMR1_OC1M | TIM_CCMR1_OC1PE
                   | TIM_CCMR1_OC2M | TIM_CCMR1_OC2PE);
    TIM1->CCMR1 |=  TIM_CCMR1_OC1M_0 | TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1PE
                 | TIM_CCMR1_OC2M_0 | TIM_CCMR1_OC2M_1 | TIM_CCMR1_OC2M_2 | TIM_CCMR1_OC2PE;

    TIM1->CCMR2 &= ~(TIM_CCMR2_OC3M | TIM_CCMR2_OC3PE);
    TIM1->CCMR2 |=  TIM_CCMR2_OC3M_0 | TIM_CCMR2_OC3M_1 | TIM_CCMR2_OC3M_2 | TIM_CCMR2_OC3PE;

    /* 5. д��Ƚ�ֵ */
    TIM1->CCR1 = duty1;
    TIM1->CCR2 = duty2;
    TIM1->CCR3 = duty3;
    
    /* 6. ������м���λ���ߵ�ƽ��Ч */
    TIM1->CCER &= ~(
         TIM_CCER_CC1P | TIM_CCER_CC1NP
       | TIM_CCER_CC2P | TIM_CCER_CC2NP
       | TIM_CCER_CC3P | TIM_CCER_CC3NP
    );
    
    /* 7. ʹ��ͨ�������������� */
    TIM1->CCER |= TIM_CCER_CC1E  | TIM_CCER_CC1NE
                | TIM_CCER_CC2E  | TIM_CCER_CC2NE
                | TIM_CCER_CC3E  | TIM_CCER_CC3NE;
    
    /* 8. ��������ʱ�䣨100 ����ʱ��ʱ�����ڣ���Լ 1.39��s�� */
    TIM1_SetDeadTime(100);
    
    /* 9. �����ʹ�ܣ�BDTR �Ĵ����� MOE λ��*/
    TIM1->BDTR |= TIM_BDTR_MOE;

    /* 10. ����һ�θ����¼��������� ARR/CCR/CCMR1 ��Ԥװ�ؼĴ���д�빤���Ĵ��� */
    TIM1_GenerateEvent_UPDATE();
}


