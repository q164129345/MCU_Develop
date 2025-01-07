#include "dwt_timer.h"
    /*
    **********************************************************************
    *         ʱ�����ؼĴ�������
    **********************************************************************
    */
    /*
    ��Cortex-M������һ�������DWT(Data Watchpoint and Trace)��
    ��������һ��32λ�ļĴ�����CYCCNT������һ�����ϵļ�������
    ��¼�����ں�ʱ�����еĸ�������ܼ�¼��ʱ��Ϊ��
    59.6523s=2��32�η�/72000000
    25.5652s=2��32�η�/168000000
    (�����ں�Ƶ��Ϊ72M���ں���һ�ε�ʱ����Ϊ1/72MԼ����13.8ns)
    ��CYCCNT���֮�󣬻���0���¿�ʼ���ϼ�����
    ʹ��CYCCNT�����Ĳ������裺
    1����ʹ��DWT���裬����������ں˵��ԼĴ���DEMCR��λ24���ƣ�д1ʹ��
    2��ʹ��CYCCNT�Ĵ���֮ǰ������0
    3��ʹ��CYCCNT�Ĵ����������DWT_CTRL(�����Ϻ궨��ΪDWT_CR)��λ0���ƣ�д1ʹ��
    */
#define  DWT_CR      *(__IO uint32_t *)0xE0001000   //< 0xE0001000 DWT_CTRL RW The Debug Watchpoint and Trace (DWT) unit
#define  DWT_CYCCNT  *(__IO uint32_t *)0xE0001004   //< 0xE0001004 DWT_CYCCNT RW Cycle Count register, 
#define  DEM_CR      *(__IO uint32_t *)0xE000EDFC   //< 0xE000EDFC DEMCR RW Debug Exception and Monitor Control Register.
#define  DEM_CR_TRCENA                   (1 << 24)  // DEMCR��DWTʹ��λ
#define  DWT_CR_CYCCNTENA                (1 <<  0)  // DWT��SYCCNTʹ��λ

__IO static uint32_t g_Initialized = 0; // ��ʼ����־λ
__IO static uint32_t g_Timestamp_us = 0;
__IO static uint32_t g_SysClock = 0; // ϵͳʱ��
__IO static uint32_t g_UsCNT = 0;    // 1us����ļ���ֵ

/*****************************************************************************************************
* describe: ��ʼ��ʱ���
* input parameter:��
* output parameter: ��
* retval: ��ǰʱ�������DWT_CYCCNT�Ĵ�����ֵ
* note:ʹ����ʱ����ǰ��������ñ�����
*****************************************************************************************************/
HAL_StatusTypeDef DWT_Timer_Init(void)
{
    DEM_CR |= (uint32_t)DEM_CR_TRCENA;         // ʹ��DWT����
    DWT_CYCCNT = (uint32_t)0u;                 // DWT CYCCNT�Ĵ�����0
    DWT_CR |= (uint32_t)DWT_CR_CYCCNTENA;      // ʹ��Cortex-M DWT CYCCNT�Ĵ���
    g_SysClock = HAL_RCC_GetSysClockFreq();    // ��ȡϵͳʱ��Ƶ��
    g_UsCNT = g_SysClock / 1000000U;           // ����1us����ļ���ֵ(MCUÿus�ļ���ֵ�����F4��168000000 / 1000000 = 168)
    g_Initialized = 1;                         // ��ɳ�ʼ��
    return HAL_OK;
}
/*****************************************************************************************************
* describe: ��ȡ��ǰʱ���
* input parameter:��
* output parameter: ��
* retval: ��ǰʱ�������DWT_CYCCNT�Ĵ�����ֵ
* note:
*****************************************************************************************************/
uint32_t DWT_Get_CNT(void)
{
    return (uint32_t)DWT_CYCCNT;
}

/*****************************************************************************************************
* describe: ��ȡϵͳʱ��Ƶ��
* input parameter:��
* output parameter: ��
* retval: ϵͳʱ��Ƶ��
* note:
*****************************************************************************************************/
uint32_t DWT_Get_System_Clock_Freq(void)
{
    return g_SysClock;
}
/*****************************************************************************************************
* describe: ��ȡ1us����ļ���ֵ
* input parameter:��
* output parameter: ��
* retval:
* note:
*****************************************************************************************************/
uint32_t DWT_Get_UsCNT(void)
{
    return g_UsCNT;
}

/*****************************************************************************************************
* describe: ����CPU���ڲ�����ʵ�־�ȷ��ʱ��32λ������
* input parameter:
@us -> �ӳٵĳ��ȣ���λ1us
* output parameter: ��
* note:
*****************************************************************************************************/
void DWT_Delay_us(uint32_t us)
{
    uint32_t ticks = 0,told = 0,tnow = 0,tcnt = 0;
    if(0 == g_Initialized)  DWT_Timer_Init();  // �������û�г�ʼ������������ʼ��
    
    ticks = us * DWT_Get_UsCNT(); // ������Ҫ�Ľ�����
    told = DWT_Get_CNT();  // ��ȡ�ս���ʱ�ļ���ֵ
    while(1){
        tnow = DWT_Get_CNT();  // ��ȡ����ֵ
        if(tnow != told){
            /* ��������� */
            if(tnow > told){
               tcnt += tnow - told;
            }else{  // ��ʱ�����
               tcnt += 0xFFFFFFFF - told + tnow;
            }
            
            if(tcnt >= ticks)  break;  // ʱ�䳬��/����Ҫ�ӳٵ�ʱ�䣬���˳�

            told = tnow;  // ���½�����
        }
    }
}

/*****************************************************************************************************
* describe:�ṩus����ʱ����������������ʱ��Լ4294.967�룬Լ71.58�֣�
* input parameter:��
* output parameter: ��
* note:
STM32F103��72 MHz�� �� DWT ���������ʱ��ԼΪ 59.7 �롣
STM32F407��168 MHz�� �� DWT ���������ʱ��ԼΪ 25.56 �롣
*****************************************************************************************************/
uint32_t DWT_Get_Microsecond(void)
{
    uint32_t tnow = DWT_Get_CNT(); // ��ȡ��ǰ����ֵ
    uint32_t us = DWT_Get_UsCNT();
    uint32_t tus = 0;
    static uint32_t told = 0,tcnt = 0;
    if(0 == g_Initialized)  DWT_Timer_Init();  // �������û�г�ʼ������������ʼ��
    
    if(told != tnow){ // ʱ���б仯
        if(tnow > told) 
            tcnt += tnow - told;
        else
            tcnt += 0xFFFFFFFF - told + tnow; // ����ļ��㷽ʽ
        told = tnow; // ����ʱ��
        tus = tcnt / us; // ������������us
        if(tus > 0){
            g_Timestamp_us += tus; // ����us
            tcnt -= tus * us; // ����us
        }
    }
    return g_Timestamp_us;
}




