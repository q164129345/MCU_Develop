#include "dwt_timer.h"
    /*
    **********************************************************************
    *         时间戳相关寄存器定义
    **********************************************************************
    */
    /*
    在Cortex-M里面有一个外设叫DWT(Data Watchpoint and Trace)，
    该外设有一个32位的寄存器叫CYCCNT，它是一个向上的计数器，
    记录的是内核时钟运行的个数，最长能记录的时间为：
    59.6523s=2的32次方/72000000
    25.5652s=2的32次方/168000000
    (假设内核频率为72M，内核跳一次的时间大概为1/72M约等于13.8ns)
    当CYCCNT溢出之后，会清0重新开始向上计数。
    使能CYCCNT计数的操作步骤：
    1、先使能DWT外设，这个由另外内核调试寄存器DEMCR的位24控制，写1使能
    2、使能CYCCNT寄存器之前，先清0
    3、使能CYCCNT寄存器，这个由DWT_CTRL(代码上宏定义为DWT_CR)的位0控制，写1使能
    */
#define  DWT_CR      *(__IO uint32_t *)0xE0001000   //< 0xE0001000 DWT_CTRL RW The Debug Watchpoint and Trace (DWT) unit
#define  DWT_CYCCNT  *(__IO uint32_t *)0xE0001004   //< 0xE0001004 DWT_CYCCNT RW Cycle Count register, 
#define  DEM_CR      *(__IO uint32_t *)0xE000EDFC   //< 0xE000EDFC DEMCR RW Debug Exception and Monitor Control Register.
#define  DEM_CR_TRCENA                   (1 << 24)  // DEMCR的DWT使能位
#define  DWT_CR_CYCCNTENA                (1 <<  0)  // DWT的SYCCNT使能位

__IO static uint32_t g_Initialized = 0; // 初始化标志位
__IO static uint32_t g_Timestamp_us = 0;
__IO static uint32_t g_SysClock = 0; // 系统时钟
__IO static uint32_t g_UsCNT = 0;    // 1us所需的计数值

/*****************************************************************************************************
* describe: 初始化时间戳
* input parameter:无
* output parameter: 无
* retval: 当前时间戳，即DWT_CYCCNT寄存器的值
* note:使用延时函数前，必须调用本函数
*****************************************************************************************************/
HAL_StatusTypeDef DWT_Timer_Init(void)
{
    DEM_CR |= (uint32_t)DEM_CR_TRCENA;         // 使能DWT外设
    DWT_CYCCNT = (uint32_t)0u;                 // DWT CYCCNT寄存器清0
    DWT_CR |= (uint32_t)DWT_CR_CYCCNTENA;      // 使能Cortex-M DWT CYCCNT寄存器
    g_SysClock = HAL_RCC_GetSysClockFreq();    // 获取系统时钟频率
    g_UsCNT = g_SysClock / 1000000U;           // 计算1us所需的计数值(MCU每us的计数值，如果F4是168000000 / 1000000 = 168)
    g_Initialized = 1;                         // 完成初始化
    return HAL_OK;
}
/*****************************************************************************************************
* describe: 读取当前时间戳
* input parameter:无
* output parameter: 无
* retval: 当前时间戳，即DWT_CYCCNT寄存器的值
* note:
*****************************************************************************************************/
uint32_t DWT_Get_CNT(void)
{
    return (uint32_t)DWT_CYCCNT;
}

/*****************************************************************************************************
* describe: 获取系统时钟频率
* input parameter:无
* output parameter: 无
* retval: 系统时钟频率
* note:
*****************************************************************************************************/
uint32_t DWT_Get_System_Clock_Freq(void)
{
    return g_SysClock;
}
/*****************************************************************************************************
* describe: 获取1us所需的计数值
* input parameter:无
* output parameter: 无
* retval:
* note:
*****************************************************************************************************/
uint32_t DWT_Get_UsCNT(void)
{
    return g_UsCNT;
}

/*****************************************************************************************************
* describe: 采用CPU的内部计数实现精确延时，32位计数器
* input parameter:
@us -> 延迟的长度，单位1us
* output parameter: 无
* note:
*****************************************************************************************************/
void DWT_Delay_us(uint32_t us)
{
    uint32_t ticks = 0,told = 0,tnow = 0,tcnt = 0;
    if(0 == g_Initialized)  DWT_Timer_Init();  // 如果发现没有初始化，就启动初始化
    
    ticks = us * DWT_Get_UsCNT(); // 计算需要的节拍数
    told = DWT_Get_CNT();  // 获取刚进来时的计数值
    while(1){
        tnow = DWT_Get_CNT();  // 获取计数值
        if(tnow != told){
            /* 计算节拍数 */
            if(tnow > told){
               tcnt += tnow - told;
            }else{  // 计时器溢出
               tcnt += 0xFFFFFFFF - told + tnow;
            }
            
            if(tcnt >= ticks)  break;  // 时间超过/等于要延迟的时间，则退出

            told = tnow;  // 更新节拍数
        }
    }
}

/*****************************************************************************************************
* describe:提供us级的时间戳，本函数的溢出时间约4294.967秒，约71.58分）
* input parameter:无
* output parameter: 无
* note:
STM32F103（72 MHz） 的 DWT 计数器溢出时间约为 59.7 秒。
STM32F407（168 MHz） 的 DWT 计数器溢出时间约为 25.56 秒。
*****************************************************************************************************/
uint32_t DWT_Get_Microsecond(void)
{
    uint32_t tnow = DWT_Get_CNT(); // 获取当前计数值
    uint32_t us = DWT_Get_UsCNT();
    uint32_t tus = 0;
    static uint32_t told = 0,tcnt = 0;
    if(0 == g_Initialized)  DWT_Timer_Init();  // 如果发现没有初始化，就启动初始化
    
    if(told != tnow){ // 时间有变化
        if(tnow > told) 
            tcnt += tnow - told;
        else
            tcnt += 0xFFFFFFFF - told + tnow; // 溢出的计算方式
        told = tnow; // 更新时间
        tus = tcnt / us; // 看看经过多少us
        if(tus > 0){
            g_Timestamp_us += tus; // 积分us
            tcnt -= tus * us; // 减掉us
        }
    }
    return g_Timestamp_us;
}




