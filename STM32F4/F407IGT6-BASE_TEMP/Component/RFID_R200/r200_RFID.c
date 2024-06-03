#include "r200_rfid.h"

#define DEBUG 1
#if DEBUG == 1
#define rfid_printf printf
#else
#define rfid_printf(format, ...)
#endif


static const uint8_t cmdFindCardOnce[7] = {0xBB, 0x00, 0x22, 0x00, 0x00, 0x22, 0x7E}; // ������ѯָ��
static const uint8_t cmdGetTransmittingPower[7] = {0xBB, 0x00, 0xB7, 0x00, 0x00, 0xB7, 0x7E}; // ��ȡ���书��
static uint8_t cmdSetTransmittingPower[9] = {0xBB, 0x00, 0xB6, 0x00, 0x02, 0x07, 0xD0, 0x8F, 0x7E}; // ���÷��书��

/**
 * @brief ִ��һ��RFID��ǩ����ѯ
 * 
 * @param me ָ��r200_rfid_reader�����ָ��
 */
static void R200_Find_The_RFID_Tag_Once(struct r200_rfid_reader* const me)
{
    if(me == NULL) return;
    
    // ����storedEPCs���� �� epcCount
    memset((uint8_t*)me->storedEPCs, 0, sizeof(me->storedEPCs));
    me->epcCount = 0;
    me->usartDrive->DMA_Sent(me->usartDrive, (uint8_t*)cmdFindCardOnce, sizeof(cmdFindCardOnce));
}

/**
 * @brief ��ȡ���书��
 * 
 * @param me ָ��r200_rfid_reader�����ָ��
 */
static void R200_Get_Transmitting_Power(struct r200_rfid_reader* const me)
{
    if(me == NULL) return;
    me->usartDrive->DMA_Sent(me->usartDrive, (uint8_t*)cmdGetTransmittingPower, sizeof(cmdGetTransmittingPower));
}

/**
 * @brief ���÷��书��
 * 
 * @param me ָ��r200_rfid_reader�����ָ��
 * @param power_dBm ���书�ʣ���λΪdBm����������20���൱��20dBm)
 */
static void R200_Set_Transmitting_Power(struct r200_rfid_reader* const me, uint16_t power)
{
    if(me == NULL) return;
    uint16_t powerX100 = power * 100; // ת��ΪЭ���ж�Ӧ�Ĺ���ֵ
    cmdSetTransmittingPower[5] = (powerX100 >> 8) & 0xFF;
    cmdSetTransmittingPower[6] = powerX100 & 0xFF;
    cmdSetTransmittingPower[7] = 0xB6 + cmdSetTransmittingPower[3] + cmdSetTransmittingPower[4] + cmdSetTransmittingPower[5] + cmdSetTransmittingPower[6];
    cmdSetTransmittingPower[8] = 0x7E;
    
    me->usartDrive->DMA_Sent(me->usartDrive, cmdSetTransmittingPower, sizeof(cmdSetTransmittingPower));
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
    if (me->CommTimeOut > 0) {
        me->CommTimeOut--;
    } else {
        me->state = STATE_WAIT_HEADER; // ��ʱ����״̬��
        me->frameIndex = 0;
    }
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
        rfid_printf("Checksum error\n");
        me->state = STATE_WAIT_HEADER;  // ����״̬��
        return;
    }
    
    me->CommTimeOut = R200_TIMEOUT_COUNT;
    uint8_t frame_type = data[1];
    uint8_t command = data[2];
    uint16_t parameter_length = (data[3] << 8) | data[4];

    /* ����ͬ��ָ����� */
    switch (frame_type) {
        case 0x01:
            if (command == 0xFF && parameter_length == 0x01) {
                uint8_t parameter = data[5];
                rfid_printf("No RFID tag found\n");
            } else if (command == 0xB7 && parameter_length == 0x02) {
                uint16_t power = (data[5] << 8) | data[6];
                me->current_Power = power / 100;
                rfid_printf("Transmitting power: %d dBm\n", me->current_Power); // ������ת��ΪdBm��ʾ
            } else if (command == 0xB6 && parameter_length == 0x01) {
                uint8_t status = data[5];
                rfid_printf("Set Power status: 0x%02X\n", status);
            }
            break;
        case 0x02:
            if (command == 0x22) {
                uint8_t rssi = data[5];
                uint16_t pc = (data[6] << 8) | data[7];
                uint8_t epc_length = parameter_length - 5;  // EPC����
                if (epc_length > EPC_LENGTH) {
                    rfid_printf("EPC length exceeds max length\n");
                    me->state = STATE_WAIT_HEADER;  // ����״̬��
                    return;
                }
                if (me->epcCount < MAX_RFID_CARDS) {
                    uint8_t epc[EPC_LENGTH] = {0,};
                    memcpy(epc, &data[8], epc_length);
                    memcpy((uint8_t*)me->storedEPCs[me->epcCount], epc, EPC_LENGTH);
                    me->epcCount++;
                    rfid_printf("Stored EPC: ");
                    for (uint8_t i = 0; i < epc_length; i++) {
                        rfid_printf("%02X ", epc[i]);
                    }
                    rfid_printf("\n");
                } else {
                    rfid_printf("RFID storage full\n");
                }
            }
            break;
        default:
            rfid_printf("Unknown frame received\n");
            break;
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
                memset((uint8_t*)me->frameBuffer, 0, FRAME_BUFFER_SIZE);
                me->paramterCount = 0;
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
            me->paramterCount++;
            uint16_t PL = me->frameBuffer[3] << 8 | me->frameBuffer[4]; // ����PL
            if (PL == me->paramterCount) {
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


static void R200_RFID_Run(struct r200_rfid_reader* const me)
{
    static uint32_t fre = 0;
    uint8_t byte;
    if (me == NULL) return;
    
    // 20ms
    if (0 == fre % 2U) {
        // ֻҪringbuffer�����ݣ���Ҫһֱ����FSM״̬�����
        while(me->usartDrive->Get_The_Number_Of_Data_In_Queue(me->usartDrive) > 0) {
            if (Queue_Pop(&me->usartDrive->queueHandler, &byte) == QUEUE_OK) {
                R200_FSM_Parse_Byte(me, byte);
            }
        }
    }
    // 1s
    if (0 == fre % 100U) {
        R200_Timeout_Counter_1S(me); // ����ͨѶ��ʱ
    }
    // 2s
    if (0 == fre % 200U) {
        R200_Find_The_RFID_Tag_Once(me); // ����һ��Ѱ��RFID��
    }
    // 5S
    if (0 == fre % 500U) {
        if (me->setting_Power != me->current_Power) {
            rfid_printf("R200:Transmitting power is not equial to setting power,Need to Set the power %d.\n", me->setting_Power);
            R200_Get_Transmitting_Power(me);     // ��ȡһ�η��书��
            R200_Set_Transmitting_Power(me,me->setting_Power); // �޸ķ��书��(28dBm)
        }
    }

    
    fre++;
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
    me->CommTimeOut = R200_TIMEOUT_COUNT;
    me->state = STATE_WAIT_HEADER;
    me->frameIndex = 0; // ��ʼ��֡����
    me->paramterCount = 0; // ��ʼ����������
    me->epcCount = 0;
    me->setting_Power = MAX_TRANSMITTING_POWER;
    
    /* ���󷽷���ʼ�� */
    me->Link_Usart_Drive = R200_Link_Usart_Drive;
    me->Find_The_RFID_Tag_Once = R200_Find_The_RFID_Tag_Once;
    me->Run_10ms = R200_RFID_Run;
}




