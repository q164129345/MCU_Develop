/**
 * @file    param_storage.h
 * @brief   Flash参数存储模块接口
 * @author  Wallace.zhang
 * @date    2025-07-31
 * @version 1.1.0
 * @copyright
 * (C) 2025 Wallace.zhang. 保留所有权利.
 * @license SPDX-License-Identifier: MIT
 */
#ifndef __PARAM_STORAGE_H
#define __PARAM_STORAGE_H

#include <stdint.h>
#include <stdbool.h>
#include "flash_map.h" // 引入Flash分区定义

/**
 * @brief 固件元数据结构体
 * @note  在您现有的代码中，此结构体似乎定义在IAP处理逻辑中。
 *        为了解耦，最好将其定义在一个公共的头文件中，或者在此处定义。
 */
typedef struct {
    uint32_t totalSize;
    uint32_t SingalSliceSize;
    uint32_t SliceCount;
} Firmware_Info_t;

/**
 * @brief 存储在Flash参数区的数据结构体
 * @note  此结构体只负责存储数据，不包含任何校验逻辑。
 */
typedef struct {
    uint32_t        structVersion;      /**< 参数结构体版本，用于未来扩展 */
    Firmware_Info_t appFWInfo;          /**< 当前App固件的信息（由IAP流程更新） */
    uint8_t         copyRetryCount;     /**< 固件拷贝重试计数器 */
    // ... 在此添加其他需要持久化的参数 ...
    
    // 我们在此增加一个针对本结构体自身的CRC，以确保参数读出的可靠性。
    // 这与固件CRC是两回事，强烈建议保留。
    uint32_t        paramsCRC32;        /**< 参数结构体自身的CRC32校验值 */
} ParameterData_t;


/**
 * @brief  初始化参数模块
 * @details 尝试从Flash加载参数。如果加载失败（如首次上电或数据损坏），
 *          则将参数恢复为默认值。
 * @return bool: true 表示成功加载有效参数, false 表示已恢复为默认值。
 */
bool Param_Init(void);

/**
 * @brief  获取当前系统参数的只读指针
 * @return const ParameterData_t*: 指向全局参数结构体的指针。
 */
const ParameterData_t* Param_Get(void);

/**
 * @brief  保存新的参数到Flash
 * @details 此函数会执行安全的“读-改-擦-写”操作，并自动更新自身CRC校验值。
 * @param  p_new_params 指向包含新数据的结构体指针。
 * @return bool: true 保存成功, false 保存失败。
 */
bool Param_Save(const ParameterData_t* p_new_params);

/**
 * @brief  更新Flash中的固件信息
 * @param  p_fw_info 指向新的固件信息结构体
 * @return bool: true 更新成功, false 更新失败
 */
bool Param_UpdateFirmwareInfo(const Firmware_Info_t* p_fw_info);

#endif /* __PARAM_STORAGE_H */

