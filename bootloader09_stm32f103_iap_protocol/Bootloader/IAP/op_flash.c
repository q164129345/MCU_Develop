/**
  ******************************************************************************
  * @file    op_flash.c
  * @brief   STM32F103 Flash操作模块实现文件
  ******************************************************************************
  */

#include "op_flash.h"

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
  * @brief  Flash整区域擦除（按页为单位擦除指定区域）
  * @param  start_addr 起始地址（必须是有效的Flash地址，并且为页首地址）
  * @param  length     擦除长度（字节），建议为页大小的整数倍，不足时向上取整
  * @retval OP_FlashStatus_t 操作结果，成功返回 OP_FLASH_OK，失败返回错误码
  *
  * @note
  *       - STM32F1系列的Flash擦除以页（Page）为最小单位，F103ZET6一页为2K字节
  *       - 擦除前需先解锁Flash（HAL_FLASH_Unlock），擦除完成后再上锁
  *       - HAL_FLASHEx_Erase() 内部会等待擦除完成，不需用户等待
  *       - 若擦除区域包含重要数据，请务必提前做好备份
  */
static OP_FlashStatus_t OP_Flash_Erase(uint32_t start_addr, uint32_t length)
{
    HAL_StatusTypeDef status;              //!< HAL库返回状态
    uint32_t PageError = 0;                //!< 记录擦除出错的页号（由HAL库维护）
    FLASH_EraseInitTypeDef EraseInitStruct;//!< Flash擦除配置结构体

    //! 1. 判断起始地址和结束地址是否合法，防止操作非法区域
    if (!OP_Flash_IsValidAddr(start_addr) || !OP_Flash_IsValidAddr(start_addr + length - 1)) {
        return OP_FLASH_ADDR_INVALID;
    }

    uint32_t pageSize = FLASH_PAGE_SIZE; //!< 每页大小，通常为2K字节
    //! 2. 计算擦除的首页页号
    uint32_t firstPage = (start_addr - FLASH_BASE) / pageSize;
    //! 3. 计算要擦除的页数（不足一页时也要整页擦除）
    uint32_t nbPages   = (length + pageSize - 1) / pageSize;

    //! 4. 解锁Flash，允许写入和擦除
    HAL_FLASH_Unlock();

    //! 5. 配置擦除参数
    EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES;  //!< 选择页擦除方式
    EraseInitStruct.PageAddress = start_addr;             //!< 擦除起始地址（页首地址）
    EraseInitStruct.NbPages     = nbPages;                //!< 擦除页数

    //! 6. 调用HAL库进行实际擦除操作
    status = HAL_FLASHEx_Erase(&EraseInitStruct, &PageError);

    //! 7. 操作结束后立即上锁，防止误操作
    HAL_FLASH_Lock();

    //! 8. 返回结果，HAL_OK即为成功，否则为错误
    return (status == HAL_OK) ? OP_FLASH_OK : OP_FLASH_ERROR;
}

/**
  * @brief  Flash写入（以32位为单位，要求地址和数据4字节对齐）
  * @param  addr    目标地址，必须是有效Flash地址且4字节对齐
  * @param  data    数据指针，指向待写入的数据缓冲区
  * @param  length  写入长度（单位：字节），必须为4的整数倍
  * @retval OP_FlashStatus_t 操作结果，成功返回OP_FLASH_OK，失败返回错误码
  *
  * @note
  *       - STM32F1的Flash写入最小单位为32位（即4字节，一个word）
  *       - 写入前需解锁Flash（HAL_FLASH_Unlock），写入结束后需重新上锁（HAL_FLASH_Lock）
  *       - 地址或长度未4字节对齐时，写入操作会直接失败
  *       - Flash写入只能将1变为0，不能将0变为1，如需写入新内容需先擦除
  *       - 建议一次性写入不超过一页数据（2K），如需大量写入可分多次调用
  */
static OP_FlashStatus_t OP_Flash_Write(uint32_t addr, uint8_t *data, uint32_t length)
{
    //! 1. 检查目标地址是否合法，防止误操作
    if (!OP_Flash_IsValidAddr(addr) || !OP_Flash_IsValidAddr(addr + length - 1)) {
        return OP_FLASH_ADDR_INVALID;
    }
    //! 2. 检查地址和长度是否4字节对齐（硬件要求）
    if ((addr % 4) != 0 || (length % 4) != 0) {
        return OP_FLASH_ERROR; //!< 非4字节对齐
    }

    HAL_StatusTypeDef status = HAL_OK; //!< HAL库函数返回状态

    //! 3. 解锁Flash，允许写操作
    HAL_FLASH_Unlock();

    //! 4. 按word（32位，4字节）为单位逐步写入
    for (uint32_t i = 0; i < length; i += 4) {
        uint32_t word;
        //! 将data[i~i+3]拷贝为32位word，防止字节序问题
        memcpy(&word, data + i, 4);
        //! 执行实际写入操作（一次写入4字节）
        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr + i, word);
        if (status != HAL_OK) {
            //! 遇到写入错误，立即上锁并返回
            HAL_FLASH_Lock();
            return OP_FLASH_ERROR;
        }
    }

    //! 5. 写入结束后上锁，避免Flash被误操作
    HAL_FLASH_Lock();
    return OP_FLASH_OK;
}

/**
  * @brief  Flash区域拷贝（典型应用：将App缓存区的固件搬运到App区）
  * @param  src_addr    源区起始地址（如：缓存区起始地址 FLASH_APP_CACHE_ADDR）
  * @param  dest_addr   目标区起始地址（如：App区起始地址 FLASH_APP_ADDR）
  * @param  length      拷贝长度（单位：字节，必须为4字节对齐）
  * @retval OP_FlashStatus_t  操作结果，成功返回OP_FLASH_OK，失败返回错误码
  *
  * @note
  *       - 该函数会自动擦除目标区域，然后分段搬运数据，节省RAM，适用于大容量固件升级
  *       - 一般用于Bootloader从下载缓存区升级App的场景
  *       - 内部采用分块拷贝，防止一次分配过大缓冲区导致内存溢出
  *       - 所有写入操作都基于4字节对齐，STM32F1系列Flash不支持非对齐写入
  *       - 操作前请确保源数据已完整写入且校验通过（如CRC32校验）
  */
OP_FlashStatus_t OP_Flash_Copy(uint32_t src_addr, uint32_t dest_addr, uint32_t length)
{
    //! 1. 检查参数有效性：长度为0、未对齐均非法
    if ((length == 0) || ((src_addr % 4) != 0) || ((dest_addr % 4) != 0) || (length % 4) != 0) {
        return OP_FLASH_ERROR; //!< 对齐检查
    }
    //! 2. 检查源区和目标区的起始地址是否合法
    if (!OP_Flash_IsValidAddr(src_addr) || !OP_Flash_IsValidAddr(dest_addr)) {
        return OP_FLASH_ADDR_INVALID;
    }

    //! 3. 擦除目标区域，确保写入前目标区全部为0xFF
    if (OP_Flash_Erase(dest_addr, length) != OP_FLASH_OK) {
        return OP_FLASH_ERROR;
    }

    #define FLASH_COPY_BUFSIZE  256  //!< 分块搬运缓冲区大小，单位字节，必须为4的倍数
    uint8_t buffer[FLASH_COPY_BUFSIZE];
    uint32_t copied = 0;  //!< 已完成拷贝的字节数

    while (copied < length) {
        //! 4. 计算本次搬运数据量，最后一次可能不足一整块
        uint32_t chunk = ((length - copied) > FLASH_COPY_BUFSIZE) ? FLASH_COPY_BUFSIZE : (length - copied);

        //! 5. 从源区读取chunk字节到临时buffer
        memcpy(buffer, (uint8_t*)(src_addr + copied), chunk);

        //! 6. 写入到目标Flash区域
        if (OP_Flash_Write(dest_addr + copied, buffer, chunk) != OP_FLASH_OK) {
            return OP_FLASH_ERROR;
        }
        copied += chunk;  //!< 累加搬运字节数
    }

    return OP_FLASH_OK;
}
