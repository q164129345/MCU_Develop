#include "r200_rfid.h"

#define DEBUG 1
#if DEBUG == 1
#define rfid_printf printf
#else
#define rfid_printf(format, ...)
#endif


static const uint8_t cmdFindCardOnce[7] = {0xBB,0x00,0x22,0x00,0x00,0x22,0x7E}; // 单次轮询指令
/**
 * @brief 执行一次RFID标签的轮询
 * 
 * @param me 指向r200_rfid_reader对象的指针
 */
static void R200_Find_The_RFID_Tag_Once(struct r200_rfid_reader* const me)
{
    if(me == NULL) return;
    
    // 清零storedEPCs数组 和 epcCount
    memset((uint8_t*)me->storedEPCs, 0, sizeof(me->storedEPCs));
    me->epcCount = 0;
    me->usartDrive->DMA_Sent(me->usartDrive, (uint8_t*)cmdFindCardOnce, sizeof(cmdFindCardOnce));
}

/**
 * @brief 关联USART驱动对象
 * 
 * @param me 指向r200_rfid_reader对象的指针
 * @param usartDrive 指向Usart_Drive对象的指针
 */
static void R200_Link_Usart_Drive(struct r200_rfid_reader* const me, Usart_Drive* const usartDrive)
{
    if (me == NULL || usartDrive == NULL) return;
    me->usartDrive = usartDrive;
}

/**
 * @brief 1秒超时计数器
 * 
 * @param me 指向r200_rfid_reader对象的指针
 */
static void R200_Timeout_Counter_1S(struct r200_rfid_reader* const me)
{
    if (me == NULL) return;
    if (me->CommTimeOut > 0) {
        me->CommTimeOut--;
    } else {
        me->state = STATE_WAIT_HEADER; // 超时重置状态机
        me->frameIndex = 0;
    }
}

/**
 * @brief 解析R200 RFID读卡器接收到的完整数据帧
 * 
 * @param me 指向r200_rfid_reader对象的指针
 */
static void R200_Parse_Received_Data_Frame(struct r200_rfid_reader* const me)
{
    if (me == NULL) return;

    uint8_t* data = (uint8_t*)me->frameBuffer;
    uint16_t length = me->frameIndex;

    // 校验码验证
    uint8_t checksum = 0;
    for (uint16_t i = 1; i < length - 2; i++) {
        checksum += data[i];
    }
    if (checksum != data[length - 2]) {
        rfid_printf("Checksum error\n");
        me->state = STATE_WAIT_HEADER;  // 重置状态机
        return;
    }

    uint8_t frame_type = data[1];
    uint8_t command = data[2];
    uint16_t parameter_length = (data[3] << 8) | data[4];

    /* 处理不同的指令代码 */
    switch (frame_type) {
        case 0x01:
            if (command == 0xFF && parameter_length == 0x01) {
                uint8_t parameter = data[5];
                rfid_printf("No RFID tag found\n");
            }
            break;
        case 0x02:
            if (command == 0x22) {
                uint8_t rssi = data[5];
                uint16_t pc = (data[6] << 8) | data[7];
                uint8_t epc_length = parameter_length - 5;  // EPC长度
                if (epc_length > EPC_LENGTH) {
                    rfid_printf("EPC length exceeds max length\n");
                    me->state = STATE_WAIT_HEADER;  // 重置状态机
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

    me->state = STATE_WAIT_HEADER;  // 重置状态机
}

/**
 * @brief FSM解析R200 RFID读卡器接收到的数据
 * 
 * @param me 指向r200_rfid_reader对象的指针
 * @param byte 接收到的字节
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
            uint16_t PL = me->frameBuffer[3] << 8 | me->frameBuffer[4]; // 计算PL
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
                
                // 完整帧接收完毕，解析帧
                R200_Parse_Received_Data_Frame(me);
            } else {
                // 帧错误，重置状态机
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
    if (me == NULL) return;
    uint8_t byte;
    // 只要ringbuffer有数据，就要一直运行FSM状态机解包
    while(me->usartDrive->Get_The_Number_Of_Data_In_Queue(me->usartDrive) > 0) {
        if (Queue_Pop(&me->usartDrive->queueHandler, &byte) == QUEUE_OK) {
            R200_FSM_Parse_Byte(me, byte);
        }
    }
}


/**
 * @brief 初始化R200 RFID读卡器对象
 * 
 * @param me 指向r200_rfid_reader对象的指针
 */
void R200_RFID_Reader_Object_Init(struct r200_rfid_reader* const me)
{
    if (me == NULL) return;
    /* 成员初始化 */
    me->CommTimeOut = R200_TIMEOUT_COUNT;
    me->state = STATE_WAIT_HEADER;
    me->frameIndex = 0; // 初始化帧索引
    me->paramterCount = 0; // 初始化参数长度
    me->epcCount = 0;
    
    /* 对象方法初始化 */
    me->Link_Usart_Drive = R200_Link_Usart_Drive;
    me->Find_The_RFID_Tag_Once = R200_Find_The_RFID_Tag_Once;
    me->Timeout_Counter_1S = R200_Timeout_Counter_1S;
    me->Parsing_Received_Data_Frame = R200_Parse_Received_Data_Frame;
    me->Run = R200_RFID_Run;
}




