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
#define EPC_LENGTH 12U // ���ų���12bytes���������ȵ�RFID������Ҫ�ܣ�
#define FRAME_BUFFER_SIZE 40U // FSM����Ļ����С
#define MAX_RFID_CARDS 15U
#define MAX_TRANSMITTING_POWER 28U

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
    volatile uint8_t storedEPCs[MAX_RFID_CARDS][EPC_LENGTH]; // �洢��ǰ��ȡ��EPC��ά����
    volatile uint8_t lastStoredEPCs[MAX_RFID_CARDS][EPC_LENGTH]; // ��һ�ε��յ���EPS
    volatile uint8_t CommTimeOut;
    volatile uint32_t initStartTime; // ģ���ʼ��ʱ���
    volatile FSM_State state;        // FSM��ǰ״̬
    volatile uint8_t frameBuffer[FRAME_BUFFER_SIZE]; // �洢���յ���֡����
    volatile uint16_t frameIndex;    // ��ǰ֡������
    volatile uint16_t paramterCount; // �ۼƲ������ĳ���
    volatile uint8_t epcCount;       // ECP������
    volatile uint8_t current_Power;  // ��ǰ�ķ��书�ʣ���λdBm��
    volatile uint8_t setting_Power;  // ���õķ��书�ʣ���λdBm��
    
    /* ���� */
    void (*Link_Usart_Drive) (struct r200_rfid_reader* const me, Usart_Drive* const usartDrive); // ���Ӵ�������
    void (*Find_The_RFID_Tag_Once) (struct r200_rfid_reader* const me);                          // ����ѰRFID��ָ��
    void (*Run_10ms) (struct r200_rfid_reader* const me);                                        // ִ��ģ�飨������ѯ����10ms��
    
}R200_Rfid_Reader;

void R200_RFID_Reader_Object_Init(R200_Rfid_Reader* const me);


#ifdef __cplusplus
} 
#endif



#endif /* __USART1_DRIVE_H */
