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
#define EPC_MAX_LENGTH 12U // ���ų���12bytes���������ȵ�RFID������Ҫ�ܣ�

// FSM���״̬��
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
    /* ��Ա */
    Usart_Drive* usartDrive;
    volatile uint8_t flagFoundRfidCard;
    volatile uint8_t CommTimeOut;
    
    volatile FSM_State state; // FSM��ǰ״̬
    volatile uint8_t frameBuffer[64]; // �洢���յ���֡����
    volatile uint16_t frameIndex; // ��ǰ֡������
    
    /* ���� */
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
