/**
 * @file    ymodem.h
 * @brief   YModem协议下位机实现头文件
 * @author  Wallace.zhang
 * @date    2025-06-20
 * @version 3.0.0
 * 
 * @note    这就像一个"纯粹的包裹处理专家"，完全独立于通信方式，
 *          只专注于解析YModem格式的数据包和管理固件写入流程
 *          
 *          v3.0.0 更新：完全解耦通信接口，支持任意数据源
 *          - YModem_Init() 不再需要通信接口参数
 *          - YModem_Run() 改为逐字节数据输入方式
 *          - 支持串口、网口、蓝牙、文件等任何数据源
 */

#ifndef __YMODEM_H
#define __YMODEM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "flash_map.h"
#include "op_flash.h"
#include "soft_crc32.h"

/**
 * @brief YModem协议常量定义
 */
#define YMODEM_SOH              0x01    /**< 128字节数据包开始标记 */
#define YMODEM_STX              0x02    /**< 1024字节数据包开始标记 */
#define YMODEM_EOT              0x04    /**< 传输结束标记 */
#define YMODEM_ACK              0x06    /**< 应答，接收正确 */
#define YMODEM_NAK              0x15    /**< 否定应答，接收错误 */
#define YMODEM_CAN              0x18    /**< 取消传输 */
#define YMODEM_C                0x43    /**< ASCII 'C'，下位机告诉上位机，表示开始接收固件数据 */

#define YMODEM_PACKET_SIZE_128  128     /**< 128字节数据包大小 */
#define YMODEM_PACKET_SIZE_1024 1024    /**< 1024字节数据包大小 */
#define YMODEM_PACKET_HEADER_SIZE 3     /**< 包头大小：起始标记+包序号+包序号取反 */
#define YMODEM_PACKET_CRC_SIZE  2       /**< CRC校验大小 */

/**
 * @brief YModem协议状态机枚举
 */
typedef enum {
    YMODEM_STATE_IDLE = 0,              /**< 空闲状态，等待开始 */
    YMODEM_STATE_WAIT_START,            /**< 等待起始包 */
    YMODEM_STATE_WAIT_FILE_INFO,        /**< 等待文件信息包(第0包) */
    YMODEM_STATE_WAIT_DATA,             /**< 等待数据包 */
    YMODEM_STATE_RECEIVING_DATA,        /**< 正在接收数据 */
    YMODEM_STATE_WAIT_END,              /**< 等待结束包 */
    YMODEM_STATE_COMPLETE,              /**< 传输完成 */
    YMODEM_STATE_ERROR,                 /**< 错误状态 */
} YModem_State_t;

/**
 * @brief YModem数据包结构
 */
typedef struct {
    uint8_t header;                     /**< 包头标记(SOH/STX) */
    uint8_t packet_num;                 /**< 包序号 */
    uint8_t packet_num_inv;             /**< 包序号取反 */
    uint8_t data[YMODEM_PACKET_SIZE_1024]; /**< 数据部分 */
    uint16_t crc;                       /**< CRC校验值 */
} YModem_Packet_t;

/**
 * @brief YModem处理结果枚举
 */
typedef enum {
    YMODEM_RESULT_OK = 0,               /**< 处理成功 */
    YMODEM_RESULT_CONTINUE,             /**< 继续处理 */
    YMODEM_RESULT_NEED_MORE_DATA,       /**< 需要更多数据 */
    YMODEM_RESULT_ERROR,                /**< 处理错误 */
    YMODEM_RESULT_COMPLETE,             /**< 传输完成 */
} YModem_Result_t;

/**
 * @brief YModem协议处理器结构体
 * @note 完全独立的协议处理器，不依赖任何特定的通信接口
 */
typedef struct YModem_Handler {
    YModem_State_t state;               /**< 当前状态 */
    
    /* 接收缓冲区管理 */
    uint8_t rx_buffer[YMODEM_PACKET_SIZE_1024 + 5]; /**< 接收缓冲区：最大包+头尾 */
    uint16_t rx_index;                  /**< 接收缓冲区索引 */
    uint16_t expected_packet_size;      /**< 期望的包大小 */
    
    /* 数据包解析 */
    YModem_Packet_t current_packet;     /**< 当前解析的数据包 */
    uint16_t expected_packet_num;        /**< 期望的包序号 (修复：改为uint16_t支持更大包序号) */
    
    /* 文件信息 */
    char file_name[256];                /**< 文件名 */
    uint32_t file_size;                 /**< 文件大小 */
    uint32_t received_size;             /**< 已接收大小 */
    
    /* Flash写入管理 */
    uint32_t flash_write_addr;          /**< 当前Flash写入地址 */
    
    /* 错误处理 */
    uint8_t retry_count;                /**< 重试计数 */
    uint8_t max_retry;                  /**< 最大重试次数 */
    
    /* 响应数据缓冲区 */
    uint8_t response_buffer[16];        /**< 响应数据缓冲区 */
    uint8_t response_length;            /**< 响应数据长度 */
    uint8_t response_ready;             /**< 响应数据就绪标志 */
    
} YModem_Handler_t;

/* 函数声明 */

/**
 * @brief 初始化YModem协议处理器
 * @param handler: YModem处理器指针
 * @return YModem_Result_t: 初始化结果
 * 
 * @note 完全独立的初始化，不依赖任何通信接口
 *       就像准备一个纯粹的"包裹处理工作台"
 */
YModem_Result_t YModem_Init(YModem_Handler_t *handler);

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
YModem_Result_t YModem_Run(YModem_Handler_t *handler, uint8_t data);

/**
 * @brief 重置YModem协议处理器
 * @param handler: YModem处理器指针
 */
void YModem_Reset(YModem_Handler_t *handler);

/**
 * @brief 获取当前传输进度
 * @param handler: YModem处理器指针
 * @return uint8_t: 传输进度百分比(0-100)
 */
uint8_t YModem_Get_Progress(YModem_Handler_t *handler);

/**
 * @brief 检查是否有响应数据需要发送
 * @param handler: YModem处理器指针
 * @return uint8_t: 1-有响应数据，0-无响应数据
 * 
 * @note 检查YModem处理器是否产生了需要发送给上位机的响应数据
 */
uint8_t YModem_Has_Response(YModem_Handler_t *handler);

/**
 * @brief 获取响应数据
 * @param handler: YModem处理器指针
 * @param buffer: 存储响应数据的缓冲区
 * @param max_length: 缓冲区最大长度
 * @return uint8_t: 实际获取的响应数据长度
 * 
 * @note 获取YModem处理器产生的响应数据，用于发送给上位机
 *       获取后会自动清除响应数据缓冲区
 */
uint8_t YModem_Get_Response(YModem_Handler_t *handler, uint8_t *buffer, uint8_t max_length);

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
uint32_t YMode_Get_File_Size(YModem_Handler_t *handler);


#ifdef __cplusplus
}
#endif

#endif /* __YMODEM_H */
