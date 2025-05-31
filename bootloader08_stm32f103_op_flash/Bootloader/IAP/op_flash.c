/**
  ******************************************************************************
  * @file    op_flash.c
  * @brief   STM32F103 Flash操作模块实现文件
  ******************************************************************************
  */

#include "op_flash.h"
#include "stm32f1xx_hal.h"    //!< HAL库接口

/**
  * @brief  判断Flash地址是否合法
  * @param  addr Flash地址
  * @retval 1 合法，0 非法
  */
static uint8_t OP_Flash_IsValidAddr(uint32_t addr)
{
    return ( (addr >= FLASH_BASE) && (addr < (FLASH_BASE + STM32_FLASH_SIZE)) ); // F103ZET6为512K
}

/**
  * @brief  Flash整区域擦除
  * @param  start_addr 起始地址
  * @param  length 擦除长度（字节）
  * @retval OP_FlashStatus_t
  */
static OP_FlashStatus_t OP_Flash_Erase(uint32_t start_addr, uint32_t length)
{
    HAL_StatusTypeDef status;
    uint32_t PageError = 0;
    FLASH_EraseInitTypeDef EraseInitStruct;

    if (!OP_Flash_IsValidAddr(start_addr) || !OP_Flash_IsValidAddr(start_addr + length - 1)) {
        return OP_FLASH_ADDR_INVALID;
    }

    uint32_t pageSize = FLASH_PAGE_SIZE; //!< 2K字节
    uint32_t firstPage = (start_addr - FLASH_BASE) / pageSize;
    uint32_t nbPages   = (length + pageSize - 1) / pageSize;

    HAL_FLASH_Unlock();

    EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.PageAddress = start_addr;
    EraseInitStruct.NbPages     = nbPages;

    status = HAL_FLASHEx_Erase(&EraseInitStruct, &PageError);

    HAL_FLASH_Lock();

    return (status == HAL_OK) ? OP_FLASH_OK : OP_FLASH_ERROR;
}

/**
  * @brief  Flash写入（以32位为单位，要求地址和数据4字节对齐）
  * @param  addr  目标地址
  * @param  data  数据指针
  * @param  length 写入长度（字节，需为4的倍数）
  * @retval OP_FlashStatus_t
  */
static OP_FlashStatus_t OP_Flash_Write(uint32_t addr, uint8_t *data, uint32_t length)
{
    if (!OP_Flash_IsValidAddr(addr) || !OP_Flash_IsValidAddr(addr + length - 1)) {
        return OP_FLASH_ADDR_INVALID;
    }
    if ((addr % 4) != 0 || (length % 4) != 0) {
        return OP_FLASH_ERROR; //!< 非4字节对齐
    }

    HAL_StatusTypeDef status = HAL_OK;

    HAL_FLASH_Unlock();

    for (uint32_t i = 0; i < length; i += 4) {
        uint32_t word;
        memcpy(&word, data + i, 4);
        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr + i, word);
        if (status != HAL_OK) {
            HAL_FLASH_Lock();
            return OP_FLASH_ERROR;
        }
    }

    HAL_FLASH_Lock();
    return OP_FLASH_OK;
}

/**
  * @brief  Flash区域拷贝（App缓存区 -> App区）
  * @param  src_addr    源区起始地址
  * @param  dest_addr   目标区起始地址
  * @param  length      拷贝长度（字节，需为4字节对齐）
  * @retval OP_FlashStatus_t  操作结果
  */
OP_FlashStatus_t OP_Flash_Copy(uint32_t src_addr, uint32_t dest_addr, uint32_t length)
{
    if ((length == 0) || ((src_addr % 4) != 0) || ((dest_addr % 4) != 0) || (length % 4) != 0) {
        return OP_FLASH_ERROR; //!< 对齐检查
    }
    if (!OP_Flash_IsValidAddr(src_addr) || !OP_Flash_IsValidAddr(dest_addr)) {
        return OP_FLASH_ADDR_INVALID;
    }

    //! 1. 擦除目标区
    if (OP_Flash_Erase(dest_addr, length) != OP_FLASH_OK) {
        return OP_FLASH_ERROR;
    }

    //! 2. 逐步拷贝写入（分段缓存，节省RAM，防止溢出）
    #define FLASH_COPY_BUFSIZE  256  //!< 拷贝缓冲，建议64~256（4字节对齐）
    uint8_t buffer[FLASH_COPY_BUFSIZE];
    uint32_t copied = 0;
    while (copied < length) {
        uint32_t chunk = ((length - copied) > FLASH_COPY_BUFSIZE) ? FLASH_COPY_BUFSIZE : (length - copied);
        memcpy(buffer, (uint8_t*)(src_addr + copied), chunk);

        if (OP_Flash_Write(dest_addr + copied, buffer, chunk) != OP_FLASH_OK) {
            return OP_FLASH_ERROR;
        }
        copied += chunk;
    }

    return OP_FLASH_OK;
}