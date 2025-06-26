#include "common.h"

/**
  * @brief  粗略延迟指定的毫秒数，自动适配主频
  * @param  ms 延迟的毫秒数
  * @note   只适合非严格实时需求，易受优化影响
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



