/**
 * @file    ymodem_example.c
 * @brief   YModem协议使用示例
 * @author  Wallace.zhang
 * @date    2025-01-20
 * @version 1.0.0
 * 
 * @note    这个文件展示了如何在main函数中正确使用YModem协议
 *          进行固件升级的完整流程
 */

#include "ymodem.h"
#include "retarget_rtt.h"

/* 全局变量示例 */
extern USART_Driver_t gUsart1Drv;  // 串口驱动实例
YModem_Handler_t gYModemHandler;    // YModem处理器实例

/**
 * @brief YModem协议使用示例
 * @note 这个函数展示了完整的YModem使用流程
 */
void YModem_Usage_Example(void)
{
    // 1. 初始化YModem协议处理器
    YModem_Result_t init_result = YModem_Init(&gYModemHandler, &gUsart1Drv);
    if (init_result != YMODEM_RESULT_OK) {
        log_printf("YModem: 初始化失败\r\n");
        return;
    }
    
    log_printf("YModem: 协议处理器初始化成功，等待上位机发送固件...\r\n");
    
    // 2. 在主循环中处理YModem协议
    while (1) {
        // 运行YModem协议处理
        YModem_Result_t result = YModem_Run(&gYModemHandler);
        
        switch (result) {
            case YMODEM_RESULT_COMPLETE:
                log_printf("YModem: 固件接收完成！\r\n");
                log_printf("文件名: %s\r\n", gYModemHandler.file_name);
                log_printf("文件大小: %lu 字节\r\n", gYModemHandler.file_size);
                log_printf("接收大小: %lu 字节\r\n", gYModemHandler.received_size);
                
                // 这里可以添加固件校验和复制逻辑
                // 例如：
                // 1. 计算接收到的固件CRC32
                // 2. 与预期的CRC32比较
                // 3. 如果校验通过，复制到App区域
                // 4. 重启跳转到新固件
                
                // 重置YModem处理器，准备下次传输
                YModem_Reset(&gYModemHandler);
                break;
                
            case YMODEM_RESULT_ERROR:
                log_printf("YModem: 传输出错，重置协议处理器\r\n");
                YModem_Reset(&gYModemHandler);
                break;
                
            case YMODEM_RESULT_CONTINUE:
            case YMODEM_RESULT_NEED_MORE_DATA:
            case YMODEM_RESULT_OK:
            default:
                // 正常处理中，继续循环
                break;
        }
        
        // 其他系统任务...
        HAL_Delay(1);
    }
}

/**
 * @brief 获取YModem传输进度并显示
 * @note 可以在定时器中调用此函数，定期显示传输进度
 */
void YModem_Display_Progress(void)
{
    static uint8_t last_progress = 0;
    uint8_t current_progress = YModem_Get_Progress(&gYModemHandler);
    
    // 只有进度变化时才打印，避免频繁输出
    if (current_progress != last_progress && current_progress > 0) {
        log_printf("YModem: 传输进度 %d%% (%lu/%lu 字节)\r\n", 
                  current_progress, 
                  gYModemHandler.received_size, 
                  gYModemHandler.file_size);
        last_progress = current_progress;
    }
}

/**
 * @brief 完整的固件升级流程示例
 * @note 这个函数展示了从接收固件到升级完成的完整流程
 */
void Firmware_Upgrade_Complete_Example(void)
{
    log_printf("开始固件升级流程...\r\n");
    
    // 1. 初始化YModem
    if (YModem_Init(&gYModemHandler, &gUsart1Drv) != YMODEM_RESULT_OK) {
        log_printf("YModem初始化失败\r\n");
        return;
    }
    
    // 2. 等待并接收固件
    YModem_Result_t result;
    do {
        result = YModem_Run(&gYModemHandler);
        
        // 定期显示进度
        YModem_Display_Progress();
        
        HAL_Delay(1);
    } while (result != YMODEM_RESULT_COMPLETE && result != YMODEM_RESULT_ERROR);
    
    // 3. 检查接收结果
    if (result == YMODEM_RESULT_COMPLETE) {
        log_printf("固件接收成功！开始校验...\r\n");
        
        // 4. 校验固件（使用项目中的CRC32函数）
        uint32_t calculated_crc = Calculate_Firmware_CRC32_SW(FLASH_DL_START_ADDR, 
                                                             gYModemHandler.file_size);
        
        log_printf("固件CRC32校验值: 0x%08lX\r\n", calculated_crc);
        
        // 5. 复制固件到App区域（如果校验通过）
        // 注意：这里应该根据实际需求添加CRC32验证逻辑
        log_printf("开始复制固件到App区域...\r\n");
        
        OP_FlashStatus_t copy_result = OP_Flash_Copy(FLASH_DL_START_ADDR, 
                                                    FLASH_APP_START_ADDR, 
                                                    gYModemHandler.file_size);
        
        if (copy_result == OP_FLASH_OK) {
            log_printf("固件升级成功！准备重启...\r\n");
            HAL_Delay(1000);  // 等待日志输出完成
            
            // 6. 跳转到新固件
            // IAP_Ready_To_Jump_App();  // 取消注释以启用跳转
        } else {
            log_printf("固件复制失败，错误码: %d\r\n", copy_result);
        }
    } else {
        log_printf("固件接收失败\r\n");
    }
    
    // 重置YModem处理器
    YModem_Reset(&gYModemHandler);
} 