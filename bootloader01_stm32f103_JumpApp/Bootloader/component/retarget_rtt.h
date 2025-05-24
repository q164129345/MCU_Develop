// retarget_rtt.h
#ifndef __RETARGET_RTT_H
#define __RETARGET_RTT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

int fputc(int ch, FILE *f);
void Retarget_RTT_Init(void);


#ifdef __cplusplus
}
#endif

#endif
