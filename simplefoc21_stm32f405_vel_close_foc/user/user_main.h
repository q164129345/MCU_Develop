#ifndef USER_MAIN_H
#define USER_MAIN_H

#include "main.h"
#include "time_utils.h"

// 为 C++ 和 C 代码之间的混合使用提供接口
// 将 C++ 的函数导出供 C 调用时，使用 extern "C"
#ifdef __cplusplus
extern "C"
{
#endif

void main_Cpp(void);
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);

#ifdef __cplusplus
}
#endif




#endif
