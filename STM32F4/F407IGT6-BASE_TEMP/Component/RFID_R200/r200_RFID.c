#include "r200_rfid.h"

static const uint8_t cmdFindCardOnce[7] = {0xBB,0x00,0x22,0x00,0x00,0x22,0x7E}; // ������ѯָ��
/**
 * @brief ִ��һ��RFID��ǩ����ѯ
 * 
 * @param me ָ��r200_rfid_reader�����ָ��
 */
static void R200_Find_The_RFID_Tag_Once(struct r200_rfid_reader* const me)
{
    if(me == NULL) return;
    me->usartDrive->DMA_Sent(me->usartDrive, (uint8_t*)cmdFindCardOnce, sizeof(cmdFindCardOnce));
}

/**
 * @brief ����USART��������
 * 
 * @param me ָ��r200_rfid_reader�����ָ��
 * @param usartDrive ָ��Usart_Drive�����ָ��
 */
static void R200_Link_Usart_Drive(struct r200_rfid_reader* const me, Usart_Drive* const usartDrive)
{
    if (me == NULL || usartDrive == NULL) return;
    me->usartDrive = usartDrive;
}

/**
 * @brief 1�볬ʱ������
 * 
 * @param me ָ��r200_rfid_reader�����ָ��
 */
static void R200_Timeout_Counter_1S(struct r200_rfid_reader* const me)
{
    if (me == NULL) return;
    if (me->CommTimeOut > 0) me->CommTimeOut--;
}

/**
 * @brief ����R200 RFID���������յ�����������֡
 * 
 * @param me ָ��r200_rfid_reader�����ָ��
 */
static void R200_Parse_Received_Data_Frame(struct r200_rfid_reader* const me)
{
    if (me == NULL) return;

    uint8_t* data = (uint8_t*)me->frameBuffer;
    uint16_t length = me->frameIndex;

    // У������֤
    uint8_t checksum = 0;
    for (uint16_t i = 1; i < length - 2; i++) {
        checksum += data[i];
    }
    if (checksum != data[length - 2]) {
        printf("Checksum error\n");
        me->state = STATE_WAIT_HEADER;  // ����״̬��
        return;
    }

    uint8_t frame_type = data[1];
    uint8_t command = data[2];
    uint16_t parameter_length = (data[3] << 8) | data[4];

    // ����ͬ��ָ�����
    if (frame_type == 0x01 && command == 0xFF && parameter_length == 0x01) {
        uint8_t parameter = data[5];
        printf("No RFID tag found, parameter: 0x%02X", parameter);
    } else if (frame_type == 0x02 && command == 0x22) {
        uint8_t rssi = data[5];
        uint16_t pc = (data[6] << 8) | data[7];
        uint8_t epc_length = parameter_length - 3;  // EPC����
        uint8_t epc[EPC_MAX_LENGTH];

        memcpy(epc, &data[8], epc_length);

        // ��ӡ�����������
        printf("RSSI: %d dBm", (int8_t)rssi);
        printf("PC: 0x%04X", pc);
        printf("EPC: ");
        for (uint8_t i = 0; i < epc_length; i++) {
            printf("%02X ", epc[i]);
        }
        printf("\n");

        me->flagFoundRfidCard = 1;
    } else {
        printf("Unknown frame received\n");
    }

    me->state = STATE_WAIT_HEADER;  // ����״̬��
}

/**
 * @brief FSM����R200 RFID���������յ�������
 * 
 * @param me ָ��r200_rfid_reader�����ָ��
 * @param byte ���յ����ֽ�
 */
static void R200_FSM_Parse_Byte(struct r200_rfid_reader* const me, uint8_t byte)
{
    switch (me->state) {
        case STATE_WAIT_HEADER:
            if (byte == 0xBB) {
                me->frameBuffer[0] = byte;
                me->frameIndex = 1;
                me->state = STATE_WAIT_TYPE;
            }
            break;
        case STATE_WAIT_TYPE:
            me->frameBuffer[me->frameIndex++] = byte;
            me->state = STATE_WAIT_COMMAND;
            break;
        case STATE_WAIT_COMMAND:
            me->frameBuffer[me->frameIndex++] = byte;
            me->state = STATE_WAIT_LENGTH_MSB;
            break;
        case STATE_WAIT_LENGTH_MSB:
            me->frameBuffer[me->frameIndex++] = byte;
            me->state = STATE_WAIT_LENGTH_LSB;
            break;
        case STATE_WAIT_LENGTH_LSB:
            me->frameBuffer[me->frameIndex++] = byte;
            me->state = STATE_WAIT_PARAMETER;
            break;
        case STATE_WAIT_PARAMETER:
            me->frameBuffer[me->frameIndex++] = byte;
            if (me->frameIndex == (5 + (me->frameBuffer[3] << 8 | me->frameBuffer[4]) + 2)) {
                me->state = STATE_WAIT_CHECKSUM;
            }
            break;
        case STATE_WAIT_CHECKSUM:
            me->frameBuffer[me->frameIndex++] = byte;
            me->state = STATE_WAIT_END;
            break;
        case STATE_WAIT_END:
            if (byte == 0x7E) {
                me->frameBuffer[me->frameIndex++] = byte;
                // ����֡������ϣ�����֡
                R200_Parse_Received_Data_Frame(me);
            } else {
                // ֡��������״̬��
                me->state = STATE_WAIT_HEADER;
            }
            break;
        default:
            me->state = STATE_WAIT_HEADER;
            break;
    }
}




















/**
 * @brief ��ʼ��R200 RFID����������
 * 
 * @param me ָ��r200_rfid_reader�����ָ��
 */
void R200_RFID_Reader_Object_Init(struct r200_rfid_reader* const me)
{
    if (me == NULL) return;
    /* ��Ա��ʼ�� */
    me->flagFoundRfidCard = 0;
    me->CommTimeOut = R200_TIMEOUT_COUNT;
    me->state = STATE_WAIT_HEADER;
    
    /* ���󷽷���ʼ�� */
    me->Link_Usart_Drive = R200_Link_Usart_Drive;
    me->Find_The_RFID_Tag_Once = R200_Find_The_RFID_Tag_Once;
    me->Timeout_Counter_1S = R200_Timeout_Counter_1S;
    
}




