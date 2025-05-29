#ifndef __CALCULATE_CRC32_H
#define __CALCULATE_CRC32_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"


uint32_t Calculate_Firmware_CRC32(uint32_t flash_addr, uint32_t data_len);


#ifdef __cplusplus
}
#endif

#endif /* __CALCULATE_CRC32_H */
