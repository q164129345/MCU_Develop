#include "retarget_printf.h"
#include "usart.h"

extern UART_HandleTypeDef huart2;

/**
  * @brief  重定向printf输出到USART2
  * @param  ch  需要输出的字符
  * @param  f   文件指针，无实际用途
  * @retval     输出的字符
  */
int _write(int file, char *ptr, int len)
{
    HAL_UART_Transmit(&huart2, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}




