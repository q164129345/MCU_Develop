/**
 * @file    ymodem.c
 * @brief   YModem协议下位机实现
 * @author  Wallace.zhang
 * @date    2025-06-20
 * @version 3.0.0
 * 
 * @note    YModem协议就像一个"纯粹的包裹处理专家"：
 *          1. 接收逐字节输入的数据（完全解耦数据来源）
 *          2. 检查包装是否完整（CRC校验）
 *          3. 按顺序整理内容（状态机管理）
 *          4. 存放到指定位置（写入Flash）
 *          5. 产生响应数据（供调用者发送给上位机）
 *          
 *          v3.0.0 更新：完全解耦通信接口，支持任意数据源
 *          - 移除所有通信接口依赖
 *          - 改为逐字节数据输入方式
 *          - 响应数据通过缓冲区输出
 */

#include "ymodem.h"
#include "retarget_rtt.h"
#include "op_flash.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

//! @defgroup 私有函数声明
//! @{
static YModem_Result_t YModem_Parse_Packet(YModem_Handler_t *handler);
static uint16_t YModem_Calculate_CRC16(const uint8_t *data, uint16_t length);
static YModem_Result_t YModem_Handle_FileInfo_Packet(YModem_Handler_t *handler);
static YModem_Result_t YModem_Handle_Data_Packet(YModem_Handler_t *handler);
static void YModem_Queue_Response(YModem_Handler_t *handler, uint8_t response);
static YModem_Result_t YModem_Write_To_Flash(YModem_Handler_t *handler, const uint8_t *data, uint16_t length);
//! @}

/**
 * @brief 初始化YModem协议处理器
 * @param handler: YModem处理器指针
 * @return YModem_Result_t: 初始化结果
 * 
 * @note 就像准备一个纯粹的"包裹处理工作台"，完全独立于数据来源
 *       不依赖任何通信接口，只专注于协议处理
 */
YModem_Result_t YModem_Init(YModem_Handler_t *handler)
{
    if (handler == NULL) {
        return YMODEM_RESULT_ERROR;
    }
    
    //! 初始化基本参数
    handler->state = YMODEM_STATE_IDLE;        //!< 设置协议状态为空闲
    handler->max_retry = 3;                    //!< 最大重试3次，防止无限重传
    
    //! 重置所有状态变量和缓冲区
    YModem_Reset(handler);
    
    log_printf("YModem: initialized successfully.\r\n");
    return YMODEM_RESULT_OK;
}

/**
 * @brief 重置YModem协议处理器
 * @param handler: YModem处理器指针
 * 
 * @note 就像清空收件箱，重新开始接收
 */
void YModem_Reset(YModem_Handler_t *handler)
{
    if (handler == NULL) return;
    
    //! 重要：强制重置为等待文件信息状态，确保能接收新传输
    handler->state = YMODEM_STATE_WAIT_FILE_INFO;     //!< 设置状态为等待文件信息
    handler->rx_index = 0;                            //!< 重置接收缓冲区索引
    handler->expected_packet_size = 0;                //!< 清空期望包大小
    handler->expected_packet_num = 0;                 //!< 清空期望包序号
    handler->file_size = 0;                           //!< 清空文件大小
    handler->received_size = 0;                       //!< 清空已接收大小
    handler->flash_write_addr = FLASH_DL_START_ADDR;  //!< 重置Flash写入地址
    handler->retry_count = 0;                         //!< 清空重试计数
    
    //! 重置响应缓冲区状态
    handler->response_length = 0;                     //!< 清空响应数据长度
    handler->response_ready = 0;                      //!< 清空响应就绪标志
    
    //! 清空所有缓冲区内容
    memset(handler->rx_buffer, 0, sizeof(handler->rx_buffer));
    memset(handler->file_name, 0, sizeof(handler->file_name));
    memset(&handler->current_packet, 0, sizeof(handler->current_packet));
    memset(handler->response_buffer, 0, sizeof(handler->response_buffer));
    
    log_printf("YModem: reset successfully, ready for new transmission.\r\n");
}

/**
 * @brief YModem协议数据处理函数
 * @param handler: YModem处理器指针
 * @param data: 输入的数据字节
 * @return YModem_Result_t: 处理结果
 * 
 * @note 逐字节处理YModem协议数据，完全解耦数据来源
 *       数据可以来自串口、网口、蓝牙、文件等任何来源
 *       就像"包裹处理专家"逐个检查包裹内容
 */
YModem_Result_t YModem_Run(YModem_Handler_t *handler, uint8_t data)
{
    if (handler == NULL) {
        return YMODEM_RESULT_ERROR;
    }
    
    //! 状态机处理 - 根据当前状态执行相应操作
    switch (handler->state) {
        case YMODEM_STATE_WAIT_FILE_INFO:
        case YMODEM_STATE_WAIT_DATA:
        case YMODEM_STATE_COMPLETE:  //!< 完成状态也要能接收新的传输请求
            //! 检查是否是包头（只有在等待包头时才处理控制字符）
            if (handler->rx_index == 0) {
                if (data == YMODEM_STX) {
                    //! 如果是完成状态收到新的传输请求，自动重置并开始新传输
                    if (handler->state == YMODEM_STATE_COMPLETE) {
                        log_printf("YModem: new transmission request in complete state, auto reset.\r\n");
                        YModem_Reset(handler);
                    }
                    handler->expected_packet_size = YMODEM_PACKET_SIZE_1024;  //!< 1024字节数据包
                    handler->rx_buffer[handler->rx_index++] = data;
                } else if (data == YMODEM_SOH) {
                    //! 如果是完成状态收到新的传输请求，自动重置并开始新传输
                    if (handler->state == YMODEM_STATE_COMPLETE) {
                        log_printf("YModem: new transmission request in complete state, auto reset.\r\n");
                        YModem_Reset(handler);
                    }
                    handler->expected_packet_size = YMODEM_PACKET_SIZE_128;   //!< 128字节数据包
                    handler->rx_buffer[handler->rx_index++] = data;
                } else if (data == YMODEM_EOT) {
                    //! 在等待数据包时收到EOT，说明传输结束
                    log_printf("YModem: received EOT, transmission end.\r\n");
                    YModem_Queue_Response(handler, YMODEM_ACK);
                    //! 修复：EOT后发送C字符，请求下一个文件（或结束包）
                    YModem_Queue_Response(handler, YMODEM_C);
                    handler->state = YMODEM_STATE_WAIT_END;
                    return YMODEM_RESULT_OK;
                } else if (data == YMODEM_CAN) {
                    //! 在等待数据包时收到CAN，说明传输取消
                    log_printf("YModem: received CAN, transmission canceled.\r\n");
                    handler->state = YMODEM_STATE_ERROR;
                    return YMODEM_RESULT_ERROR;
                } else {
                    //! 不是有效包头或控制字符，忽略
                    return YMODEM_RESULT_CONTINUE;
                }
            } else {
                //! 正在接收数据包过程中，所有字节都当作数据处理，不检查控制字符
                if (handler->rx_index >= sizeof(handler->rx_buffer)) {
                    log_printf("YModem: rx buffer overflow, reset.\r\n");
                    handler->rx_index = 0;
                    return YMODEM_RESULT_ERROR;
                }
                
                handler->rx_buffer[handler->rx_index++] = data;
                
                //! 检查是否接收完整个包
                uint16_t total_size = YMODEM_PACKET_HEADER_SIZE + 
                                    handler->expected_packet_size + 
                                    YMODEM_PACKET_CRC_SIZE;
                
                if (handler->rx_index >= total_size) {
                    return YModem_Parse_Packet(handler);
                }
            }
            break;
            
        case YMODEM_STATE_WAIT_END:
            //! 等待结束包（空文件名包）或EOT
            if (handler->rx_index == 0) {
                if (data == YMODEM_EOT) {
                    log_printf("YModem: received EOT, transmission end.\r\n");
                    YModem_Queue_Response(handler, YMODEM_ACK);
                    //! 修复：EOT后发送C字符，请求下一个文件（或结束包）
                    YModem_Queue_Response(handler, YMODEM_C);
                    return YMODEM_RESULT_OK;
                } else if (data == YMODEM_STX) {
                    handler->expected_packet_size = YMODEM_PACKET_SIZE_1024;  //!< 1024字节结束包
                    handler->rx_buffer[handler->rx_index++] = data;
                } else if (data == YMODEM_CAN) {
                    log_printf("YModem: received CAN, transmission canceled.\r\n");
                    handler->state = YMODEM_STATE_ERROR;
                    return YMODEM_RESULT_ERROR;
                } else {
                    //! 忽略其他字符
                    return YMODEM_RESULT_CONTINUE;
                }
            } else {
                //! 正在接收结束包过程中，所有字节都当作数据处理
                if (handler->rx_index >= sizeof(handler->rx_buffer)) {
                    log_printf("YModem: rx buffer overflow in end state, reset.\r\n");
                    handler->rx_index = 0;
                    return YMODEM_RESULT_ERROR;
                }
                
                handler->rx_buffer[handler->rx_index++] = data;
                
                uint16_t total_size = YMODEM_PACKET_HEADER_SIZE + 
                                    handler->expected_packet_size + 
                                    YMODEM_PACKET_CRC_SIZE;
                
                if (handler->rx_index >= total_size) {
                    //! 解析结束包
                    YModem_Result_t result = YModem_Parse_Packet(handler);
                    if (result == YMODEM_RESULT_OK) {
                        log_printf("YModem: transmission completed! total received %u bytes.\r\n", handler->received_size);
                        handler->state = YMODEM_STATE_COMPLETE;
                        YModem_Queue_Response(handler, YMODEM_ACK);
                        return YMODEM_RESULT_COMPLETE;
                    }
                }
            }
            break;
            
        default:
            break;
    }
    
    return YMODEM_RESULT_CONTINUE;
}

/**
 * @brief 解析YModem数据包
 * @param handler: YModem处理器指针
 * @return YModem_Result_t: 解析结果
 * 
 * @note 就像拆开快递包装，检查内容是否完整正确
 */
static YModem_Result_t YModem_Parse_Packet(YModem_Handler_t *handler)
{
    //! 提取包头信息
    handler->current_packet.header = handler->rx_buffer[0];         //!< 包头标识符（SOH/STX）
    handler->current_packet.packet_num = handler->rx_buffer[1];     //!< 包序号
    handler->current_packet.packet_num_inv = handler->rx_buffer[2]; //!< 包序号反码
    
    //! 检查包序号的完整性
    if ((handler->current_packet.packet_num + handler->current_packet.packet_num_inv) != 0xFF) {
        log_printf("YModem: packet number verification failed.\r\n");
        YModem_Queue_Response(handler, YMODEM_NAK);
        handler->rx_index = 0;
        
        //! 修复：增加重试计数和错误处理
        handler->retry_count++;
        if (handler->retry_count >= handler->max_retry) {
            log_printf("YModem: max retry count reached, abort.\r\n");
            handler->state = YMODEM_STATE_ERROR;
        }
        
        return YMODEM_RESULT_ERROR;
    }
    
    //! 提取数据部分
    memcpy(handler->current_packet.data, 
           &handler->rx_buffer[YMODEM_PACKET_HEADER_SIZE], 
           handler->expected_packet_size);
    
    //! 提取CRC校验值
    uint16_t crc_offset = YMODEM_PACKET_HEADER_SIZE + handler->expected_packet_size;
    handler->current_packet.crc = (handler->rx_buffer[crc_offset] << 8) | 
                                  handler->rx_buffer[crc_offset + 1];
    
    //! 计算并验证CRC
    uint16_t calculated_crc = YModem_Calculate_CRC16(handler->current_packet.data, 
                                                    handler->expected_packet_size);
    
    if (calculated_crc != handler->current_packet.crc) {
        log_printf("YModem: CRC verification failed, calculated value=0x%04X, received value=0x%04X\r\n", 
                  calculated_crc, handler->current_packet.crc);
        YModem_Queue_Response(handler, YMODEM_NAK);
        handler->rx_index = 0;
        
        //! 修复：增加重试计数和错误处理
        handler->retry_count++;
        if (handler->retry_count >= handler->max_retry) {
            log_printf("YModem: max retry count reached, abort.\r\n");
            handler->state = YMODEM_STATE_ERROR;
        }
        
        return YMODEM_RESULT_ERROR;
    }
    
    //! 重置接收缓冲区索引
    handler->rx_index = 0;
    
    //! 修复：成功处理包后重置重试计数
    handler->retry_count = 0;
    
    //! 根据包序号处理不同类型的包
    if (handler->current_packet.packet_num == 0) {
        //! 第0包：文件信息包
        return YModem_Handle_FileInfo_Packet(handler);
    } else {
        //! 数据包
        return YModem_Handle_Data_Packet(handler);
    }
}

/**
 * @brief 计算CRC16校验值
 * @param data: 数据指针
 * @param length: 数据长度
 * @return uint16_t: CRC16校验值
 * 
 * @note 使用CRC-16/XMODEM算法，就像给包裹贴上防伪标签
 */
static uint16_t YModem_Calculate_CRC16(const uint8_t *data, uint16_t length)
{
    uint16_t crc = 0x0000;  //!< CRC初始值
    
    for (uint16_t i = 0; i < length; i++) {
        crc ^= (data[i] << 8);
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                crc = ((crc << 1) ^ 0x1021) & 0xFFFF;  //!< CRC-16/XMODEM多项式
            } else {
                crc = (crc << 1) & 0xFFFF;
            }
        }
    }
    
    return crc;
}

/**
 * @brief 处理文件信息包（第0包）
 * @param handler: YModem处理器指针
 * @return YModem_Result_t: 处理结果
 * 
 * @note 就像查看快递单上的收件信息：文件名、大小等
 */
static YModem_Result_t YModem_Handle_FileInfo_Packet(YModem_Handler_t *handler)
{
    //! 检查是否是结束包（空文件名）
    if (handler->current_packet.data[0] == 0) {
        log_printf("YModem: received end packet.\r\n");
        handler->state = YMODEM_STATE_COMPLETE;
        YModem_Queue_Response(handler, YMODEM_ACK);
        log_printf("YModem: transmission completed, returning YMODEM_RESULT_COMPLETE.\r\n");
        return YMODEM_RESULT_COMPLETE;
    }
    
    //! 解析文件名
    char *filename_ptr = (char *)handler->current_packet.data;
    
    /*!
     * @brief 检查文件名长度，防止缓冲区溢出
     * @note 手动实现strnlen功能，因为某些编译器不支持strnlen
     */
    size_t filename_len = 0;
    const char *temp_ptr = filename_ptr;
    while (filename_len < handler->expected_packet_size && *temp_ptr != '\0') {
        filename_len++;
        temp_ptr++;
    }
    
    if (filename_len >= sizeof(handler->file_name)) {
        log_printf("YModem: filename too long (%u bytes), max allowed %u bytes.\r\n", 
                  (unsigned int)filename_len, (unsigned int)(sizeof(handler->file_name) - 1));
        YModem_Queue_Response(handler, YMODEM_CAN);
        handler->state = YMODEM_STATE_ERROR;
        return YMODEM_RESULT_ERROR;
    }
    
    strncpy(handler->file_name, filename_ptr, sizeof(handler->file_name) - 1);
    handler->file_name[sizeof(handler->file_name) - 1] = '\0';  //!< 确保字符串以NULL结尾
    
    //! 查找文件大小（在文件名后的第一个NULL之后）
    char *size_ptr = filename_ptr + strlen(filename_ptr) + 1;
    
    //! 修复：检查size_ptr是否在有效范围内
    if (size_ptr >= (char*)handler->current_packet.data + handler->expected_packet_size) {
        log_printf("YModem: invalid file size field position.\r\n");
        YModem_Queue_Response(handler, YMODEM_CAN);
        handler->state = YMODEM_STATE_ERROR;
        return YMODEM_RESULT_ERROR;
    }
    
    handler->file_size = (uint32_t)atol(size_ptr);  //!< 转换文件大小字符串为数值
    
    log_printf("YModem: file information - name: %s, size: %u bytes.\r\n", 
              handler->file_name, handler->file_size);
    
    //! 检查文件大小是否超出Flash缓存区容量
    if (handler->file_size > FLASH_DL_SIZE) {
        log_printf("YModem: error! file size %u exceeds cache size %u.\r\n", 
                  handler->file_size, (uint32_t)FLASH_DL_SIZE);
        YModem_Queue_Response(handler, YMODEM_CAN);
        handler->state = YMODEM_STATE_ERROR;
        return YMODEM_RESULT_ERROR;
    }
    
    //! 准备接收数据包
    handler->expected_packet_num = 1;                    //!< 期望接收第1个数据包
    handler->received_size = 0;                          //!< 重置已接收大小
    handler->flash_write_addr = FLASH_DL_START_ADDR;     //!< 重置Flash写入地址
    handler->state = YMODEM_STATE_WAIT_DATA;             //!< 切换到等待数据状态
    
    //! 发送ACK和'C'，表示准备接收数据
    YModem_Queue_Response(handler, YMODEM_ACK);
    YModem_Queue_Response(handler, YMODEM_C);
    
    return YMODEM_RESULT_OK;
}

/**
 * @brief 处理数据包
 * @param handler: YModem处理器指针
 * @return YModem_Result_t: 处理结果
 * 
 * @note 就像把快递内容按顺序整理存放到仓库
 */
static YModem_Result_t YModem_Handle_Data_Packet(YModem_Handler_t *handler)
{
    //! 检查包序号是否正确
    if (handler->current_packet.packet_num != handler->expected_packet_num) {
        log_printf("YModem: packet number error, expected %u, received %u.\r\n", 
                  handler->expected_packet_num, handler->current_packet.packet_num);
        YModem_Queue_Response(handler, YMODEM_NAK);
        
        //! 修复：增加重试计数和错误处理
        handler->retry_count++;
        if (handler->retry_count >= handler->max_retry) {
            log_printf("YModem: max retry count reached, abort.\r\n");
            handler->state = YMODEM_STATE_ERROR;
        }
        
        return YMODEM_RESULT_ERROR;
    }
    
    //! 计算本包实际有效数据长度
    uint16_t valid_data_length = handler->expected_packet_size;    //!< 默认为完整包大小
    uint32_t remaining_size = handler->file_size - handler->received_size;
    
    if (remaining_size < handler->expected_packet_size) {
        valid_data_length = (uint16_t)remaining_size;   //!< 最后一包可能不满
    }
    
    //! 写入Flash存储
    YModem_Result_t result = YModem_Write_To_Flash(handler, 
                                                  handler->current_packet.data, 
                                                  valid_data_length);
    
    if (result != YMODEM_RESULT_OK) {
        log_printf("YModem: Flash write failed.\r\n");
        YModem_Queue_Response(handler, YMODEM_CAN);
        handler->state = YMODEM_STATE_ERROR;
        return YMODEM_RESULT_ERROR;
    }
    
    //! 更新传输状态
    handler->received_size += valid_data_length;       //!< 累加已接收大小
    handler->expected_packet_num++;                    //!< 期望下一个包序号

    //! 打印进度（每收到1个包打印一次）
    uint8_t progress = YModem_Get_Progress(handler);
    log_printf("YModem: progress %d%% (%u/%u bytes).\r\n", 
              progress, handler->received_size, handler->file_size);

    //! 发送ACK确认包接收成功
    YModem_Queue_Response(handler, YMODEM_ACK);
    
    //! 检查是否接收完成
    if (handler->received_size >= handler->file_size) {
        log_printf("YModem: data received completed, waiting for EOT.\r\n");
        handler->state = YMODEM_STATE_WAIT_END;
    }
    
    return YMODEM_RESULT_OK;
}

/**
 * @brief 将数据写入Flash（使用op_flash模块重构版本）
 * @param handler: YModem处理器指针
 * @param data: 要写入的数据
 * @param length: 数据长度
 * @return YModem_Result_t: 写入结果
 * 
 * @note 使用op_flash模块进行Flash操作，提高代码复用性和可维护性
 *       就像使用专业的"仓库管理系统"来存放货物，而不是直接操作货架
 */
static YModem_Result_t YModem_Write_To_Flash(YModem_Handler_t *handler, 
                                           const uint8_t *data, 
                                           uint16_t length)
{
    //! 检查Flash地址范围
    if (handler->flash_write_addr + length > FLASH_DL_END_ADDR) {
        log_printf("YModem: Flash address out of range.\r\n");
        return YMODEM_RESULT_ERROR;
    }
    
    //! 确保Flash写入地址4字节对齐
    if (handler->flash_write_addr % 4 != 0) {
        log_printf("YModem: Flash address not aligned to 4 bytes: 0x%08lX.\r\n", 
                  (unsigned long)handler->flash_write_addr);
        return YMODEM_RESULT_ERROR;
    }
    
    /*!
     * @brief 检查是否需要擦除Flash页面
     * @note 当写入地址是页面起始地址时，需要先擦除该页面
     */
    uint32_t page_start = (handler->flash_write_addr / STM32_FLASH_PAGE_SIZE) * STM32_FLASH_PAGE_SIZE;
    if (handler->flash_write_addr == page_start) {
        log_printf("YModem: erase Flash page 0x%08X.\r\n", page_start);
        
        //! 使用op_flash模块擦除页面
        OP_FlashStatus_t erase_result = OP_Flash_Erase(page_start, STM32_FLASH_PAGE_SIZE);
        if (erase_result != OP_FLASH_OK) {
            log_printf("YModem: Flash erase failed, op_flash error code=%d.\r\n", erase_result);
            return YMODEM_RESULT_ERROR;
        }
    }
    
    /*!
     * @brief 准备4字节对齐的数据缓冲区
     * @note 使用固定大小缓冲区避免VLA问题
     */
    uint16_t aligned_length = (length + 3) & ~3;  //!< 向上对齐到4字节边界
    static uint8_t aligned_buffer[YMODEM_PACKET_SIZE_1024 + 4];  //!< 静态缓冲区，足够大
    
    //! 检查缓冲区大小
    if (aligned_length > sizeof(aligned_buffer)) {
        log_printf("YModem: aligned data too large for buffer.\r\n");
        return YMODEM_RESULT_ERROR;
    }
    
    //! 复制数据到对齐缓冲区，不足部分填充0xFF
    memcpy(aligned_buffer, data, length);
    if (aligned_length > length) {
        memset(aligned_buffer + length, 0xFF, aligned_length - length);
    }
    
    //! 使用op_flash模块写入数据
    OP_FlashStatus_t write_result = OP_Flash_Write(handler->flash_write_addr, 
                                                  aligned_buffer, 
                                                  aligned_length);
    
    if (write_result != OP_FLASH_OK) {
        log_printf("YModem: Flash write failed, op_flash error code=%d.\r\n", write_result);
        return YMODEM_RESULT_ERROR;
    }
    
    log_printf("YModem: successfully written %d bytes to address 0x%04X%04X.\r\n", 
              length, (uint16_t)(handler->flash_write_addr >> 16), (uint16_t)(handler->flash_write_addr & 0xFFFF));
    
    //! 更新Flash写入地址
    handler->flash_write_addr += aligned_length;
    
    return YMODEM_RESULT_OK;
}

/**
 * @brief 将响应数据加入队列
 * @param handler: YModem处理器指针
 * @param response: 响应字节
 * 
 * @note 就像把要寄出的回执信放进信箱，等待邮递员来取
 *       完全解耦发送方式，由调用者决定如何发送
 */
static void YModem_Queue_Response(YModem_Handler_t *handler, uint8_t response)
{
    if (handler == NULL) return;
    
    //! 检查响应缓冲区是否还有空间
    if (handler->response_length < sizeof(handler->response_buffer)) {
        handler->response_buffer[handler->response_length++] = response;  //!< 添加响应字节
        handler->response_ready = 1;                                      //!< 设置响应就绪标志
        
        log_printf("YModem: response data added to queue: 0x%02X.\r\n", response);
    } else {
        log_printf("YModem: warning! response buffer is full, discard response: 0x%02X.\r\n", response);
    }
}

/**
 * @brief 获取当前传输进度
 * @param handler: YModem处理器指针
 * @return uint8_t: 传输进度百分比(0-100)
 * 
 * @note 就像查看快递配送进度
 */
uint8_t YModem_Get_Progress(YModem_Handler_t *handler)
{
    if (handler == NULL || handler->file_size == 0) {
        return 0;
    }
    
    return (uint8_t)((handler->received_size * 100) / handler->file_size);
}

/**
 * @brief 检查是否有响应数据需要发送
 * @param handler: YModem处理器指针
 * @return uint8_t: 1-有响应数据，0-无响应数据
 * 
 * @note 检查信箱里是否有待发送的回执信
 */
uint8_t YModem_Has_Response(YModem_Handler_t *handler)
{
    if (handler == NULL) {
        return 0;
    }
    
    return handler->response_ready;
}

/**
 * @brief 获取响应数据
 * @param handler: YModem处理器指针
 * @param buffer: 存储响应数据的缓冲区
 * @param max_length: 缓冲区最大长度
 * @return uint8_t: 实际获取的响应数据长度
 * 
 * @note 从信箱取出所有待发送的回执信，取出后信箱就空了
 */
uint8_t YModem_Get_Response(YModem_Handler_t *handler, uint8_t *buffer, uint8_t max_length)
{
    if (handler == NULL || buffer == NULL || max_length == 0) {
        return 0;
    }
    
    if (!handler->response_ready || handler->response_length == 0) {
        return 0;
    }
    
    //! 计算实际复制的长度
    uint8_t copy_length = (handler->response_length > max_length) ? 
                         max_length : handler->response_length;
    
    //! 复制响应数据到调用者缓冲区
    memcpy(buffer, handler->response_buffer, copy_length);
    
    //! 清空响应缓冲区，为下次响应做准备
    handler->response_length = 0;                                       //!< 重置响应数据长度
    handler->response_ready = 0;                                        //!< 清除响应就绪标志
    memset(handler->response_buffer, 0, sizeof(handler->response_buffer));
    
    log_printf("YModem: successfully obtained %d bytes of response data.\r\n", copy_length);
    
    return copy_length;
}

/**
 * @brief 获取当前文件大小
 * @param handler: YModem处理器指针
 * @return uint32_t: 当前正在传输的文件大小（字节）
 * 
 * @note 就像查看包裹上的"重量标签"，告诉你这个文件总共有多大
 *       - 在接收到文件信息包后，这个值才会有效
 *       - 返回0表示还没有接收到文件信息或文件大小为0
 *       - 常用于显示传输进度或验证传输完整性
 */
uint32_t YMode_Get_File_Size(YModem_Handler_t *handler)
{
    return handler->file_size;
}




