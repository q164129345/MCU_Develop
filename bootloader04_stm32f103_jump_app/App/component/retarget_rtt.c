#include "retarget_rtt.h"

//! RTT����
static uint8_t RTT_BufferUp0[256] = {0,};

/**
  * @brief  �ض���printf�����RTTͨ��0
  * @param  ch  ��Ҫ������ַ�
  * @param  f   �ļ�ָ�룬��ʵ����;
  * @retval     ������ַ�
  */
int fputc(int ch, FILE *f)
{
    SEGGER_RTT_PutChar(0, ch);
    return ch;
}

/**
  * @brief  ��ʼ��SEGGER RTT�ض������л��������ã�
  * @note   �ú���Ӧ�ڵ���printf���ض������ǰ���ã�ȷ��RTT��������ͨ��������ȷ��
  *         - ����ͨ��0ΪMCU��PC�������ϴ������������л���������
  *           ʹ���Զ��徲̬�ڴ棬������Ϊ������ģʽ��
  *         - ����RTTĬ���ն�Ϊͨ��0����֤printf�����ͨ��RTTͨ��0���͡�
  * @retval ��
  */
void Retarget_RTT_Init(void)
{
    //! ����MCU -> PC�����������л�������
    SEGGER_RTT_ConfigUpBuffer(1,                            //! ͨ��0
                            "Buffer1Up",                    //! ͨ������
                            (uint8_t*)&RTT_BufferUp0[0],    //! �����ַ
                            sizeof(RTT_BufferUp0),          //! �����С
                            SEGGER_RTT_MODE_NO_BLOCK_SKIP); //! ������
    SEGGER_RTT_SetTerminal(1); //! �����ն�0
}


