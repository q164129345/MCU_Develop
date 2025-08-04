#include "retarget_rtt.h"

//! RTT缓存
//static uint8_t RTT_BufferUp1[1024] = {0,};

/**
  * @brief  重定向printf输出到RTT通道0
  * @param  ch  需要输出的字符
  * @param  f   文件指针，无实际用途
  * @retval     输出的字符
  */
int fputc(int ch, FILE *f)
{
    SEGGER_RTT_PutChar(0, ch);
    return ch;
}

/**
  * @brief  初始化SEGGER RTT重定向（上行缓冲区配置）
  * @note   该函数应在调用printf等重定向输出前调用，确保RTT缓冲区和通道配置正确。
  *         - 配置通道0为MCU到PC的数据上传缓冲区（上行缓冲区），
  *           使用自定义静态内存，并设置为非阻塞模式。
  *         - 设置RTT默认终端为通道0，保证printf等输出通过RTT通道0发送。
  * @retval 无
  */
void Retarget_RTT_Init(void)
{
    //! 配置MCU -> PC缓冲区（上行缓存区）
//    SEGGER_RTT_ConfigUpBuffer(1,                            //! 通道0
//                            "Buffer1Up",                    //! 通道名字
//                            (uint8_t*)&RTT_BufferUp1[0],    //! 缓存地址
//                            sizeof(RTT_BufferUp1),          //! 缓存大小
//                            SEGGER_RTT_MODE_NO_BLOCK_SKIP); //! 非阻塞
    SEGGER_RTT_SetTerminal(0); //! 设置终端0
}


