/**
 * @file    param_storage.c
 * @brief   Flash参数存储模块实现
 */
#include "param_storage.h"
#include "op_flash.h"       // 依赖底层的Flash驱动
#include "soft_crc32.h"     // 用于计算参数结构体自身的CRC
#include <string.h>

/**
 * @brief 全局参数缓存
 */
ParameterData_t g_params;

/**
 * @brief 计算并返回给定参数结构体的CRC32值
 */
static uint32_t calculate_params_crc(const ParameterData_t* p_params)
{
    // CRC计算不包含crc32字段本身
    return Calculate_Firmware_CRC32_SW((uint32_t)p_params, sizeof(ParameterData_t) - sizeof(uint32_t));
}

/**
 * @brief 安全地将全局参数g_params写入Flash
 * @note  这是一个内部函数，封装了擦写和CRC更新的完整流程。
 */
static bool flush_params_to_flash(void)
{
    // 1. 更新全局缓存中的CRC值
    g_params.paramsCRC32 = calculate_params_crc(&g_params);

    // 2. 擦除整个参数扇区
    if (OP_Flash_Erase(FLASH_PARAM_START_ADDR, FLASH_PARAM_SIZE) != OP_FLASH_OK) {
        // log_printf("Failed to erase parameter sector!\r\n");
        return false;
    }
    
    // 3. 将带有新CRC的整个结构体写回Flash
    uint32_t write_len = (sizeof(ParameterData_t) + 3) & ~3; // 4字节对齐
    if (OP_Flash_Write(FLASH_PARAM_START_ADDR, (uint8_t*)&g_params, write_len) != OP_FLASH_OK) {
        // log_printf("Failed to write parameters to Flash!\r\n");
        return false;
    }
    
    return true;
}

/**
 * @brief 初始化参数
 * @note  从Flash参数区读取数据到全局缓存 g_params
 * @return true 成功
 * @return false 失败
 */
bool Param_Init(void)
{
    // 1. 从Flash参数区读取数据到全局缓存 g_params
    memcpy(&g_params, (void*)FLASH_PARAM_START_ADDR, sizeof(ParameterData_t));

    // 2. 校验数据完整性 (检查参数结构体自身是否损坏)
    if (g_params.paramsCRC32 == calculate_params_crc(&g_params)) {
        log_printf("Parameters loaded successfully from Flash.\r\n");
        return true;
    } else {
        // 3. CRC校验失败，加载默认值
        log_printf("Parameter CRC check failed! Loading default parameters.\r\n");
        memset(&g_params, 0, sizeof(ParameterData_t));
        g_params.structVersion = 0x010000;
        g_params.appFWInfo.totalSize = 0; // 默认参数
        return false;
    }
}

/**
 * @brief 获取参数
 * @note  返回全局缓存 g_params 的指针
 * @return const ParameterData_t* 
 */
const ParameterData_t* Param_Get(void)
{
    return &g_params;
}

/**
 * @brief 保存参数
 * @note  将全局缓存 g_params 写入Flash
 * @param p_new_params 新的参数数据
 * @return true 成功
 * @return false 失败
 */
bool Param_Save(const ParameterData_t* p_new_params)
{
    // 1. 复制新数据到全局缓存
    // 不直接修改g_params是为了保持原子性，但在C中这样做已经足够
    memcpy(&g_params, p_new_params, sizeof(ParameterData_t));

    // 2. 将更新后的全局缓存写入Flash
    if (!flush_params_to_flash()) {
        log_printf("Failed to save parameters to Flash!\r\n");
        return false;
    }

    log_printf("Parameters saved to Flash successfully.\r\n");
    return true;
}

/**
 * @brief 更新固件信息
 * @note  将全局缓存 g_params 写入Flash
 * @param p_fw_info 新的固件信息
 * @return true 成功
 * @return false 失败
 */
bool Param_UpdateFirmwareInfo(const Firmware_Info_t* p_fw_info)
{
    // 1. 更新全局缓存中的固件信息部分
    memcpy(&g_params.appFWInfo, p_fw_info, sizeof(Firmware_Info_t));
    
    // 2. 将更新后的全局缓存写入Flash
    if (!flush_params_to_flash()) {
        // log_printf("Failed to update firmware info in Flash!\r\n");
        return false;
    }
    
    // log_printf("Firmware info updated successfully in parameters.\r\n");
    return true;
}
