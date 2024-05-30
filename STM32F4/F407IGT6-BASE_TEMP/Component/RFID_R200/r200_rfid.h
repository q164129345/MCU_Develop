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

#define R200_TIMEOUT_COUNT 10U
#define EPC_MAX_LENGTH 12U // 卡号长度12bytes（其他长度的RFID卡，不要管）

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
    volatile uint8_t flagFoundRfidCard;
    volatile uint8_t CommTimeOut;
    
    volatile FSM_State state; // FSM当前状态
    volatile uint8_t frameBuffer[64]; // 存储接收到的帧数据
    volatile uint16_t frameIndex; // 当前帧的索引
    
    /* 方法 */
    void (*Link_Usart_Drive) (struct r200_rfid_reader* const me, Usart_Drive* const usartDrive);
    void (*Find_The_RFID_Tag_Once) (struct r200_rfid_reader* const me);
    void (*Parsing_Received_Data_Frame) (struct r200_rfid_reader* const me, uint8_t byte);
    void (*Timeout_Counter_1S) (struct r200_rfid_reader* const me);
}R200_Rfid_Reader;
    

void R200_RFID_Reader_Object_Init(R200_Rfid_Reader* const me);


#ifdef __cplusplus
} 
#endif



#endif /* __USART1_DRIVE_H */
