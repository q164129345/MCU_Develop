#include "common.h"

/**
  * @brief  �����ӳ�ָ���ĺ��������Զ�������Ƶ
  * @param  ms �ӳٵĺ�����
  * @note   ֻ�ʺϷ��ϸ�ʵʱ���������Ż�Ӱ��
  */
void Delay_MS_By_NOP(uint32_t ms)
{
    uint32_t nops_per_ms = SystemCoreClock / 1000U;
    while (ms--)
    {
        for (volatile uint32_t i = 0; i < nops_per_ms; i++)
        {
            __NOP();
        }
    }
}



