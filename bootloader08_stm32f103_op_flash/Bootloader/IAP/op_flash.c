/**
  ******************************************************************************
  * @file    op_flash.c
  * @brief   STM32F103 Flash����ģ��ʵ���ļ�
  ******************************************************************************
  */

#include "op_flash.h"
#include "stm32f1xx_hal.h"    //!< HAL��ӿ�

/**
  * @brief  �ж�Flash��ַ�Ƿ�Ϸ�
  * @param  addr Flash��ַ
  * @retval 1 �Ϸ���0 �Ƿ�
  */
static uint8_t OP_Flash_IsValidAddr(uint32_t addr)
{
    return ( (addr >= FLASH_BASE) && (addr < (FLASH_BASE + STM32_FLASH_SIZE)) ); // F103ZET6Ϊ512K
}

/**
  * @brief  Flash���������
  * @param  start_addr ��ʼ��ַ
  * @param  length �������ȣ��ֽڣ�
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

    uint32_t pageSize = FLASH_PAGE_SIZE; //!< 2K�ֽ�
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
  * @brief  Flashд�루��32λΪ��λ��Ҫ���ַ������4�ֽڶ��룩
  * @param  addr  Ŀ���ַ
  * @param  data  ����ָ��
  * @param  length д�볤�ȣ��ֽڣ���Ϊ4�ı�����
  * @retval OP_FlashStatus_t
  */
static OP_FlashStatus_t OP_Flash_Write(uint32_t addr, uint8_t *data, uint32_t length)
{
    if (!OP_Flash_IsValidAddr(addr) || !OP_Flash_IsValidAddr(addr + length - 1)) {
        return OP_FLASH_ADDR_INVALID;
    }
    if ((addr % 4) != 0 || (length % 4) != 0) {
        return OP_FLASH_ERROR; //!< ��4�ֽڶ���
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
  * @brief  Flash���򿽱���App������ -> App����
  * @param  src_addr    Դ����ʼ��ַ
  * @param  dest_addr   Ŀ������ʼ��ַ
  * @param  length      �������ȣ��ֽڣ���Ϊ4�ֽڶ��룩
  * @retval OP_FlashStatus_t  �������
  */
OP_FlashStatus_t OP_Flash_Copy(uint32_t src_addr, uint32_t dest_addr, uint32_t length)
{
    if ((length == 0) || ((src_addr % 4) != 0) || ((dest_addr % 4) != 0) || (length % 4) != 0) {
        return OP_FLASH_ERROR; //!< ������
    }
    if (!OP_Flash_IsValidAddr(src_addr) || !OP_Flash_IsValidAddr(dest_addr)) {
        return OP_FLASH_ADDR_INVALID;
    }

    //! 1. ����Ŀ����
    if (OP_Flash_Erase(dest_addr, length) != OP_FLASH_OK) {
        return OP_FLASH_ERROR;
    }

    //! 2. �𲽿���д�루�ֶλ��棬��ʡRAM����ֹ�����
    #define FLASH_COPY_BUFSIZE  256  //!< �������壬����64~256��4�ֽڶ��룩
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