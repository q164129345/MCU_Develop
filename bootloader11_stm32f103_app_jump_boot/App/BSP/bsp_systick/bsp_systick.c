#include "bsp_systick/bsp_systick.h"

volatile uint64_t uwTick; //!< 全局Tick计数

/**
  * @brief  获取系统当前Tick，单位ms
  * @retval uint64_t 当前Tick计数
  */
uint64_t SysTick_GetTicks(void)
{
    return uwTick;
}

/**
  * @brief  SysTick定时器中断服务函数
  * @note   每1ms进入一次该中断，Tick计数自增
  * @retval 无
  */
void SysTick_Interrupt(void)
{
    uwTick++;
}

/**
  * @brief  初始化SysTick定时器，实现1ms周期中断
  * @note   调用后SysTick将自动每1ms触发一次中断
  * @retval 无
  */
void SysTick_Init(void)
{
    SysTick_Config(SystemCoreClock / 1000); //! 初始化SysTick,1ms中断
}









