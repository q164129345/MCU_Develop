#ifndef _R200_RFID_H
#define _R200_RFID_H

#include "main.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "usart_drive.h"

#ifdef __cplusplus
extern "C" {
#endif

#define R200_TIMEOUT_COUNT 12U
#define EPC_LENGTH 12U // 卡号长度12bytes（其他长度的RFID卡，不要管）
#define FRAME_BUFFER_SIZE 40U // FSM组包的缓存大小
#define MAX_RFID_CARDS 15U
#define MAX_TRANSMITTING_POWER 28U

// FSM解包状态机
typedef enum {
    STATE_WAIT_HEADER,
    STATE_WAIT_TYPE,
    STATE_WAIT_COMMAND,
    STATE_WAIT_LENGTH_MSB,
    STATE_WAIT_LENGTH_LSB,
    STATE_WAIT_PARAMETER,
    STATE_WAIT_CHECKSUM,
    STATE_WAIT_END,
}FSM_State;

typedef struct r200_rfid_reader {
    /* 成员 */
    Usart_Drive* usartDrive;
    volatile uint8_t storedEPCs[MAX_RFID_CARDS][EPC_LENGTH]; // 存储当前获取的EPC二维数组
    volatile uint8_t lastStoredEPCs[MAX_RFID_CARDS][EPC_LENGTH]; // 上一次的收到的EPS
    volatile uint8_t CommTimeOut;
    volatile uint32_t initStartTime; // 模块初始化时间戳
    volatile FSM_State state;        // FSM当前状态
    volatile uint8_t frameBuffer[FRAME_BUFFER_SIZE]; // 存储接收到的帧数据
    volatile uint16_t frameIndex;    // 当前帧的索引
    volatile uint16_t paramterCount; // 累计参数区的长度
    volatile uint8_t epcCount;       // ECP计数器
    volatile uint8_t current_Power;  // 当前的发射功率（单位dBm）
    volatile uint8_t setting_Power;  // 设置的发射功率（单位dBm）
    
    /* 方法 */
    void (*Link_Usart_Drive) (struct r200_rfid_reader* const me, Usart_Drive* const usartDrive); // 连接串口驱动
    void (*Find_The_RFID_Tag_Once) (struct r200_rfid_reader* const me);                          // 单次寻RFID卡指令
    void (*Run_10ms) (struct r200_rfid_reader* const me);                                        // 执行模块（建议轮询周期10ms）
    
}R200_Rfid_Reader;

void R200_RFID_Reader_Object_Init(R200_Rfid_Reader* const me);


#ifdef __cplusplus
} 
#endif



#endif /* __USART1_DRIVE_H */
