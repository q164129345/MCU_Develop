// retarget_rtt.h
#ifndef __RETARGET_RTT_H
#define __RETARGET_RTT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/**
  * @brief  ��ʽ����ӡ������Ϣ�������������кţ�
  * @note   �÷���printfһ�£�����Զ�������ǰ���������к�
  */
#define log_printf(fmt, ...) \
printf("%s-%s(%d):" fmt, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)

/**
  * @brief  �ض���printf�����RTTͨ��0
  */
int fputc(int ch, FILE *f);

/**
  * @brief  ��ʼ��SEGGER RTT�ض������л��������ã�
  */
void Retarget_RTT_Init(void);


#ifdef __cplusplus
}
#endif

#endif
