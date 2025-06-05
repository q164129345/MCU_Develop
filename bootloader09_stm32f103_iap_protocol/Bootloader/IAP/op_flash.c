/**
  ******************************************************************************
  * @file    op_flash.c
  * @brief   STM32F103 Flash����ģ��ʵ���ļ�
  ******************************************************************************
  */

#include "op_flash.h"

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
  * @brief  Flash�������������ҳΪ��λ����ָ������
  * @param  start_addr ��ʼ��ַ����������Ч��Flash��ַ������Ϊҳ�׵�ַ��
  * @param  length     �������ȣ��ֽڣ�������Ϊҳ��С��������������ʱ����ȡ��
  * @retval OP_FlashStatus_t ����������ɹ����� OP_FLASH_OK��ʧ�ܷ��ش�����
  *
  * @note
  *       - STM32F1ϵ�е�Flash������ҳ��Page��Ϊ��С��λ��F103ZET6һҳΪ2K�ֽ�
  *       - ����ǰ���Ƚ���Flash��HAL_FLASH_Unlock����������ɺ�������
  *       - HAL_FLASHEx_Erase() �ڲ���ȴ�������ɣ������û��ȴ�
  *       - ���������������Ҫ���ݣ��������ǰ���ñ���
  */
static OP_FlashStatus_t OP_Flash_Erase(uint32_t start_addr, uint32_t length)
{
    HAL_StatusTypeDef status;              //!< HAL�ⷵ��״̬
    uint32_t PageError = 0;                //!< ��¼���������ҳ�ţ���HAL��ά����
    FLASH_EraseInitTypeDef EraseInitStruct;//!< Flash�������ýṹ��

    //! 1. �ж���ʼ��ַ�ͽ�����ַ�Ƿ�Ϸ�����ֹ�����Ƿ�����
    if (!OP_Flash_IsValidAddr(start_addr) || !OP_Flash_IsValidAddr(start_addr + length - 1)) {
        return OP_FLASH_ADDR_INVALID;
    }

    uint32_t pageSize = FLASH_PAGE_SIZE; //!< ÿҳ��С��ͨ��Ϊ2K�ֽ�
    //! 2. �����������ҳҳ��
    uint32_t firstPage = (start_addr - FLASH_BASE) / pageSize;
    //! 3. ����Ҫ������ҳ��������һҳʱҲҪ��ҳ������
    uint32_t nbPages   = (length + pageSize - 1) / pageSize;

    //! 4. ����Flash������д��Ͳ���
    HAL_FLASH_Unlock();

    //! 5. ���ò�������
    EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES;  //!< ѡ��ҳ������ʽ
    EraseInitStruct.PageAddress = start_addr;             //!< ������ʼ��ַ��ҳ�׵�ַ��
    EraseInitStruct.NbPages     = nbPages;                //!< ����ҳ��

    //! 6. ����HAL�����ʵ�ʲ�������
    status = HAL_FLASHEx_Erase(&EraseInitStruct, &PageError);

    //! 7. ����������������������ֹ�����
    HAL_FLASH_Lock();

    //! 8. ���ؽ����HAL_OK��Ϊ�ɹ�������Ϊ����
    return (status == HAL_OK) ? OP_FLASH_OK : OP_FLASH_ERROR;
}

/**
  * @brief  Flashд�루��32λΪ��λ��Ҫ���ַ������4�ֽڶ��룩
  * @param  addr    Ŀ���ַ����������ЧFlash��ַ��4�ֽڶ���
  * @param  data    ����ָ�룬ָ���д������ݻ�����
  * @param  length  д�볤�ȣ���λ���ֽڣ�������Ϊ4��������
  * @retval OP_FlashStatus_t ����������ɹ�����OP_FLASH_OK��ʧ�ܷ��ش�����
  *
  * @note
  *       - STM32F1��Flashд����С��λΪ32λ����4�ֽڣ�һ��word��
  *       - д��ǰ�����Flash��HAL_FLASH_Unlock����д�������������������HAL_FLASH_Lock��
  *       - ��ַ�򳤶�δ4�ֽڶ���ʱ��д�������ֱ��ʧ��
  *       - Flashд��ֻ�ܽ�1��Ϊ0�����ܽ�0��Ϊ1������д�����������Ȳ���
  *       - ����һ����д�벻����һҳ���ݣ�2K�����������д��ɷֶ�ε���
  */
static OP_FlashStatus_t OP_Flash_Write(uint32_t addr, uint8_t *data, uint32_t length)
{
    //! 1. ���Ŀ���ַ�Ƿ�Ϸ�����ֹ�����
    if (!OP_Flash_IsValidAddr(addr) || !OP_Flash_IsValidAddr(addr + length - 1)) {
        return OP_FLASH_ADDR_INVALID;
    }
    //! 2. ����ַ�ͳ����Ƿ�4�ֽڶ��루Ӳ��Ҫ��
    if ((addr % 4) != 0 || (length % 4) != 0) {
        return OP_FLASH_ERROR; //!< ��4�ֽڶ���
    }

    HAL_StatusTypeDef status = HAL_OK; //!< HAL�⺯������״̬

    //! 3. ����Flash������д����
    HAL_FLASH_Unlock();

    //! 4. ��word��32λ��4�ֽڣ�Ϊ��λ��д��
    for (uint32_t i = 0; i < length; i += 4) {
        uint32_t word;
        //! ��data[i~i+3]����Ϊ32λword����ֹ�ֽ�������
        memcpy(&word, data + i, 4);
        //! ִ��ʵ��д�������һ��д��4�ֽڣ�
        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr + i, word);
        if (status != HAL_OK) {
            //! ����д�������������������
            HAL_FLASH_Lock();
            return OP_FLASH_ERROR;
        }
    }

    //! 5. д�����������������Flash�������
    HAL_FLASH_Lock();
    return OP_FLASH_OK;
}

/**
  * @brief  Flash���򿽱�������Ӧ�ã���App�������Ĺ̼����˵�App����
  * @param  src_addr    Դ����ʼ��ַ���磺��������ʼ��ַ FLASH_APP_CACHE_ADDR��
  * @param  dest_addr   Ŀ������ʼ��ַ���磺App����ʼ��ַ FLASH_APP_ADDR��
  * @param  length      �������ȣ���λ���ֽڣ�����Ϊ4�ֽڶ��룩
  * @retval OP_FlashStatus_t  ����������ɹ�����OP_FLASH_OK��ʧ�ܷ��ش�����
  *
  * @note
  *       - �ú������Զ�����Ŀ������Ȼ��ֶΰ������ݣ���ʡRAM�������ڴ������̼�����
  *       - һ������Bootloader�����ػ���������App�ĳ���
  *       - �ڲ����÷ֿ鿽������ֹһ�η�����󻺳��������ڴ����
  *       - ����д�����������4�ֽڶ��룬STM32F1ϵ��Flash��֧�ַǶ���д��
  *       - ����ǰ��ȷ��Դ����������д����У��ͨ������CRC32У�飩
  */
OP_FlashStatus_t OP_Flash_Copy(uint32_t src_addr, uint32_t dest_addr, uint32_t length)
{
    //! 1. ��������Ч�ԣ�����Ϊ0��δ������Ƿ�
    if ((length == 0) || ((src_addr % 4) != 0) || ((dest_addr % 4) != 0) || (length % 4) != 0) {
        return OP_FLASH_ERROR; //!< ������
    }
    //! 2. ���Դ����Ŀ��������ʼ��ַ�Ƿ�Ϸ�
    if (!OP_Flash_IsValidAddr(src_addr) || !OP_Flash_IsValidAddr(dest_addr)) {
        return OP_FLASH_ADDR_INVALID;
    }

    //! 3. ����Ŀ������ȷ��д��ǰĿ����ȫ��Ϊ0xFF
    if (OP_Flash_Erase(dest_addr, length) != OP_FLASH_OK) {
        return OP_FLASH_ERROR;
    }

    #define FLASH_COPY_BUFSIZE  256  //!< �ֿ���˻�������С����λ�ֽڣ�����Ϊ4�ı���
    uint8_t buffer[FLASH_COPY_BUFSIZE];
    uint32_t copied = 0;  //!< ����ɿ������ֽ���

    while (copied < length) {
        //! 4. ���㱾�ΰ��������������һ�ο��ܲ���һ����
        uint32_t chunk = ((length - copied) > FLASH_COPY_BUFSIZE) ? FLASH_COPY_BUFSIZE : (length - copied);

        //! 5. ��Դ����ȡchunk�ֽڵ���ʱbuffer
        memcpy(buffer, (uint8_t*)(src_addr + copied), chunk);

        //! 6. д�뵽Ŀ��Flash����
        if (OP_Flash_Write(dest_addr + copied, buffer, chunk) != OP_FLASH_OK) {
            return OP_FLASH_ERROR;
        }
        copied += chunk;  //!< �ۼӰ����ֽ���
    }

    return OP_FLASH_OK;
}
