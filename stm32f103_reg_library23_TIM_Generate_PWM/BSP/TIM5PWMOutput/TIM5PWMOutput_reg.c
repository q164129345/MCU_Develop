#include "TIM5PWMOutput/TIM5PWMOutput_reg.h"

/**
 * @brief  ���� TIM5 ͨ��1 �� PWM ���
 * @retval ��
 */
void TIM5_PWM_Start(void)
{
    /* ʹ�� CC1 ��� */
    TIM5->CCER |= TIM_CCER_CC1E;
    /* ʹ�ܶ�ʱ������ */
    TIM5->CR1 |= TIM_CR1_CEN;
    /* ������Ч�����������¼�������Ԥװ�ؼĴ��� */
    TIM5->EGR |= TIM_EGR_UG;
}

/**
 * @brief  ֹͣ TIM5 ͨ��1 �� PWM ���
 * @retval ��
 */
void TIM5_PWM_Stop(void)
{
    /* ���ö�ʱ������ */
    TIM5->CR1 &= ~TIM_CR1_CEN;
    /* ���� CC1 ��� */
    TIM5->CCER &= ~TIM_CCER_CC1E;
}

/**
 * @brief  ���� PWM ռ�ձȣ�ͨ��1��
 * @param  ccr: �ȽϼĴ���ֵ����Χ 0 ~ (ARR+1)
 * @retval ��
 */
void TIM5_PWM_SetDuty(uint16_t ccr)
{
    TIM5->CCR1 = ccr;
    /* ����������Ч����ȡ��ע�ʹ��������¼� */
    // TIM5->EGR |= TIM_EGR_UG;
}

/**
 * @brief  ���� PWM ���ڣ��Զ���װ�ؼĴ�����
 * @param  arr: �Զ���װ�ؼĴ���ֵ������ - 1��
 * @retval ��
 */
void TIM5_PWM_SetPeriod(uint16_t arr)
{
    TIM5->ARR = arr;
    /* ����ʹ�� ARR Ԥװ�أ�����Ҫ���������¼� */
    TIM5->EGR |= TIM_EGR_UG;
}

/**
 * @brief  ���ò���ʼ�� TIM5 ͨ��1 PWM �����PA0 = TIM5_CH1��
 * @param  arr: �Զ���װ�ؼĴ���ֵ������ - 1��
 * @param  psc: Ԥ��Ƶ��ֵ
 * @retval ��
 */
void TIM5_PWM_Init(uint16_t psc, uint16_t arr, uint16_t ccr)
{
    /* 1. ʹ��ʱ�ӣ�TIM5��APB1��, GPIOA��APB2�� */
    RCC->APB1ENR |= RCC_APB1ENR_TIM5EN;
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;

    /* 2. ���� PA0 Ϊ����������� */
    GPIOA->CRL &= ~(GPIO_CRL_MODE0 | GPIO_CRL_CNF0);
    GPIOA->CRL |=  (GPIO_CRL_MODE0_0   /* ���ģʽ��10 MHz */
                  | GPIO_CRL_CNF0_1);  /* �������� */

    /* 3. ���� TIM5 �������� */
    TIM5->PSC  = psc;    /* Ԥ��Ƶ */
    TIM5->ARR  = arr;    /* �Զ���װ�� */
    /* ʹ�� ARR Ԥװ�� */
    TIM5->CR1 |= TIM_CR1_ARPE;

    /* 4. ����ͨ��1 Ϊ PWM1 ģʽ */
    TIM5->CCMR1 &= ~TIM_CCMR1_OC1M; // ��0
    TIM5->CCMR1 |=  (TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1); /* PWM ģʽ1 */
    /* ʹ�� CCR1 Ԥװ�� */
    TIM5->CCMR1 |= TIM_CCMR1_OC1PE;
    /* Ĭ��ռ�ձ� 0 */
    TIM5->CCR1 = ccr;

    /* 5. ���ü��ԣ��ߵ�ƽ��Ч������ʱ������� */
    TIM5->CCER &= ~TIM_CCER_CC1P;
    TIM5->CCER &= ~TIM_CCER_CC1E;
}





