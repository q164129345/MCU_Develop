/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "bsp_usart_hal.h"
#include "app_jump.h"
#include "soft_crc32.h"
#include "op_flash.h"
#include "fw_verify.h"
#include "ymodem.h"
#include "boot_entry.h"
#include "param_storage.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
extern DMA_HandleTypeDef hdma_usart1_rx;
extern DMA_HandleTypeDef hdma_usart1_tx;

//! USART1缓存 RX方向
uint8_t gUsart1RXDMABuffer[2048];
uint8_t gUsart1RXRBBuffer[2048];
//! USART1缓存 TX方向
uint8_t gUsart1TXDMABuffer[2048];
uint8_t gUsart1TXRBBuffer[2048];
//! 实例化Usart1
USART_Driver_t gUsart1Drv = {
    .huart = &huart1,
    .hdma_rx = &hdma_usart1_rx,
    .hdma_tx = &hdma_usart1_tx,
};

//! YModem协议处理器实例
YModem_Handler_t gYModemHandler;

//! 添加 IAP 完成延迟计数器，确保最终 ACK 有时间发送
static uint32_t iap_complete_delay_counter = 0;
static bool iap_complete_pending = false;

//! 添加 IAP 超时机制 - 根据App有效性决定是否启用
#define IAP_TIMEOUT_SECONDS     5
#define IAP_TIMEOUT_MS          (IAP_TIMEOUT_SECONDS * 1000)
static uint32_t iap_timeout_counter = 0;
static bool iap_timeout_enabled = false;  // 默认禁用，根据启动原因决定
static bool iap_communication_detected = false;

//! 添加定期发送'C'字符的功能，告知上位机bootloader已就绪
#define IAP_HEARTBEAT_INTERVAL_MS   2000    // 每2秒发送一次'C'字符
static uint32_t iap_heartbeat_counter = 0;
static bool iap_heartbeat_enabled = true;  // 默认启用心跳

//! 保存启动原因的快照，用于后续超时决策
volatile static uint64_t gUpdateFlag = 0;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static void Timeout_Handler_MS(void);
static void Timeout_Counter_Reset(void);
static void Timeout_Counter_Enable(void);
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  uint32_t fre = 0;
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  //! 初始化参数区
  Param_Init();
  //! USART1初始化
  USART_Config(&gUsart1Drv,
               gUsart1RXDMABuffer, gUsart1RXRBBuffer, sizeof(gUsart1RXDMABuffer),
               gUsart1TXDMABuffer, gUsart1TXRBBuffer, sizeof(gUsart1TXDMABuffer));
  
  //! YModem协议处理器初始化（完全解耦版本）
  YModem_Init(&gYModemHandler);
  
  //! 启动原因分析和处理(超时机制)
  Timeout_Counter_Enable();

  log_printf("Bootloader init successfully.\n");
  
  //! 初始化完成后立即发送第一次'C'字符心跳
  if (iap_heartbeat_enabled && gYModemHandler.state == YMODEM_STATE_WAIT_FILE_INFO) {
      uint8_t heartbeat_char = 0x43; // 'C'字符
      USART_Put_TxData_To_Ringbuffer(&gUsart1Drv, &heartbeat_char, 1);
      log_printf("IAP heartbeat: immediate first 'C' sent after bootloader init.\r\n");
  }
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    Timeout_Handler_MS(); //! 超时处理

    //! 30ms
    if (0 == fre % 30) {
        HAL_GPIO_TogglePin(LED0_GPIO_Port,LED0_Pin); //! 心跳灯快闪（在bootlaoder程序里，心跳灯快闪。App程序，心跳灯慢闪。肉眼区分当前跑什么程序）
    }
    
    //! 2ms
    if (0 == fre % 2) {
        //! YModem协议处理 - 逐字节从ringbuffer里拿出数据来解析
        while(USART_Get_The_Existing_Amount_Of_Data(&gUsart1Drv)) {
            uint8_t data;
            if (USART_Take_A_Piece_Of_Data(&gUsart1Drv, &data)) {
                YModem_Run(&gYModemHandler, data); //! 运行YModem协议
                Timeout_Counter_Reset(); // 重置超时计数器，保持通信活跃
                
                //! 一旦开始接收YModem数据，关闭心跳发送
                if (iap_heartbeat_enabled && gYModemHandler.state != YMODEM_STATE_WAIT_FILE_INFO) {
                    iap_heartbeat_enabled = false;
                    log_printf("IAP heartbeat disabled - YModem transmission started.\r\n");
                }
            }
        }

        //! 根据YModem协议的状态，处理IAP流程
        if (gYModemHandler.state == YMODEM_STATE_COMPLETE && !iap_complete_pending) {
            //! 第一次检测到完成状态，启动延迟计数器
            log_printf("YModem: IAP upgrade completed. Starting delay to ensure final ACK transmission...\r\n");
            iap_complete_pending = true;
            iap_complete_delay_counter = 0;
        } else if (iap_complete_pending) {
            //! 延迟期间，继续发送任何待发送的响应数据
            iap_complete_delay_counter++;
            
            //! 延迟足够时间确保最终ACK发送完成（大约500ms = 250次2ms周期）
            if (iap_complete_delay_counter >= 250) {
                log_printf("YModem: delay completed, starting firmware verification and copy...\r\n");
                uint32_t file_size = YMode_Get_File_Size(&gYModemHandler);
                log_printf("Firmware size: %lu bytes.\r\n", (unsigned long)file_size);
                if (HAL_OK == FW_Firmware_Verification(FLASH_DL_START_ADDR, file_size)) {
                    log_printf("CRC32 verification was successful\r\n");
                    
                    //! 第一步，首先将文件大小写入参数区
                    log_printf("Updating firmware metadata (intent to copy)...\r\n");
                    ParameterData_t temp_params = *Param_Get(); // 获取当前参数副本
                    temp_params.appFWInfo.totalSize = file_size; // 保存固件的长度
                    temp_params.copyRetryCount = 0; // 关键！每一次固件的更新都要清0。虽然，这个功能暂时不用。
                    Param_Save(&temp_params); //! 擦写并更新CRC
                    
                    //! 第二步，将固件从下载区拷贝到App区
                    if (OP_FLASH_OK == OP_Flash_Copy(FLASH_DL_START_ADDR, FLASH_APP_START_ADDR, FLASH_APP_SIZE)) { //! 将App下载缓存区的固件搬运到App区
                        log_printf("The firmware copy to the app area successfully.\r\n");
                        log_printf("Jump to the application.\r\n");
                        HAL_Delay(20);
                        IAP_Ready_To_Jump_App(); //! 跳转App
                    }
                } else {
                    log_printf("CRC32 verification failed\r\n");
                    //! 重要：重置状态和YModem处理器，准备下次传输
                    log_printf("YModem: resetting for next transmission...\r\n");
                    iap_complete_pending = false;
                    iap_complete_delay_counter = 0;
                    
                    //! 重置心跳功能，重新开始发送'C'字符
                    iap_heartbeat_enabled = true;
                    iap_heartbeat_counter = 0;
                    log_printf("IAP heartbeat re-enabled after CRC verification failed.\r\n");
                    
                    YModem_Reset(&gYModemHandler);
                    
                    //! 立即发送'C'字符，告知上位机可以重新开始传输
                    if (gYModemHandler.state == YMODEM_STATE_WAIT_FILE_INFO) {
                        uint8_t heartbeat_char = 0x43; // 'C'字符
                        USART_Put_TxData_To_Ringbuffer(&gUsart1Drv, &heartbeat_char, 1);
                        log_printf("IAP heartbeat: immediate 'C' sent after reset.\r\n");
                    }
                }
            }
        } else if (gYModemHandler.state == YMODEM_STATE_ERROR) {
            log_printf("YModem: transmission error, reset protocol processor.\r\n"); //! 传输出错，重置协议处理器
            //! 重置延迟状态
            iap_complete_pending = false;
            iap_complete_delay_counter = 0;
            iap_communication_detected = false; //! 重置通信标志
            iap_timeout_counter = 0; //! 重置超时计数器
            
            //! 重置心跳功能，重新开始发送'C'字符
            iap_heartbeat_enabled = true;
            iap_heartbeat_counter = 0;
            log_printf("IAP heartbeat re-enabled after error.\r\n");
            
            YModem_Reset(&gYModemHandler);
            
            //! 立即发送'C'字符，告知上位机可以重新开始传输
            if (gYModemHandler.state == YMODEM_STATE_WAIT_FILE_INFO) {
                uint8_t heartbeat_char = 0x43; // 'C'字符
                USART_Put_TxData_To_Ringbuffer(&gUsart1Drv, &heartbeat_char, 1);
                log_printf("IAP heartbeat: immediate 'C' sent after error recovery.\r\n");
            }
        }

        //! 检查是否有YModem响应数据需要发送
        if (YModem_Has_Response(&gYModemHandler)) {
            uint8_t response_buffer[16];
            uint8_t response_length = YModem_Get_Response(&gYModemHandler, response_buffer, sizeof(response_buffer));
            if (response_length > 0) {
                //! 将响应数据发送给上位机
                USART_Put_TxData_To_Ringbuffer(&gUsart1Drv, response_buffer, response_length);
            }
        }
        
        USART_Module_Run(&gUsart1Drv); //! Usart1模块运行
    }
    
    //! IAP心跳处理 - 每1ms执行
    if (iap_heartbeat_enabled) {
        iap_heartbeat_counter++;
        
        //! 每2秒发送一次'C'字符，告知上位机bootloader已就绪
        if (iap_heartbeat_counter >= IAP_HEARTBEAT_INTERVAL_MS) {
            //! 只在等待文件信息状态时发送心跳
            if (gYModemHandler.state == YMODEM_STATE_WAIT_FILE_INFO) {
                uint8_t heartbeat_char = 0x43; // 'C'字符
                USART_Put_TxData_To_Ringbuffer(&gUsart1Drv, &heartbeat_char, 1);
                log_printf("IAP heartbeat: sent 'C' to notify host - bootloader ready.\r\n");
            }
            iap_heartbeat_counter = 0; // 重置心跳计数器
        }
    }
    
    fre++;
    HAL_Delay(1);
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
/**
 * @brief   IAP超时处理函数
 * @note    每1ms调用一次，管理IAP模式下的超时倒计时
 * @details 
 *   - 持续监控通信状态，每秒显示剩余时间
 *   - 超时后根据启动原因智能处理：
 *     * App请求模式：返回App
 *     * 正常启动且App有效：跳转App
 *     * App无效：重置YModem等待新固件
 */
static void Timeout_Counter_Reset(void)
{
    if (iap_timeout_enabled) {
        iap_timeout_counter = 0; // 重置计数器
        if (!iap_communication_detected) {
            iap_communication_detected = true;
            log_printf("IAP communication established, timeout counter reset.\n");
        }
    }
}

/**
 * @brief   检测通信活跃性并重置超时计数器
 * @note    当检测到IAP通信时调用，重置超时机制确保持续通信
 * @details 
 *   - 仅在超时机制启用时生效
 *   - 首次检测到通信会标记通信状态并记录日志
 *   - 每次调用都会重置超时计数器到0
 */
static void Timeout_Counter_Enable(void)
{
    gUpdateFlag = IAP_GetUpdateFlag();
    const ParameterData_t* params = Param_Get();
    bool app_valid = Is_App_Valid_Enhanced(FLASH_APP_START_ADDR);
    log_printf("=== Bootloader Boot Analysis ===\n");
    log_printf("Boot flag: 0x%08lX%08lX\n", (unsigned long)(gUpdateFlag >> 32), (unsigned long)gUpdateFlag);
    log_printf("App valid: %s\n", app_valid ? "YES" : "NO");
    
    if (gUpdateFlag == FIRMWARE_UPDATE_MAGIC_WORD) {
        //! App请求进入IAP模式
        IAP_SetUpdateFlag(0); // 清除RAM中的标志，避免重复触发
        log_printf("=== IAP Mode: App Request ===\n");
        log_printf("Timeout: ENABLED (%d seconds)\n", IAP_TIMEOUT_SECONDS);
        log_printf("Will return to App if no communication detected.\n");
        iap_timeout_enabled = true;
    } else {
        //! 正常启动
        IAP_SetUpdateFlag(0); // 清除可能存在的无效标志
        if (app_valid) {
            //! 正常启动，启用超时
            log_printf("=== IAP Mode: Normal Entry (App Valid) ===\n");
            log_printf("Timeout: ENABLED (%d seconds)\n", IAP_TIMEOUT_SECONDS);
            log_printf("Will return to App if no communication detected.\n");
            iap_timeout_enabled = true;
        } else {
            //! App无效，必须等待固件 或 看看下载区是否有完整的App
            log_printf("=== IAP Mode: App Invalid ===\n");
            log_printf("Timeout: DISABLED\n");
            iap_timeout_enabled = false;
            //！检查下载区是否有完整的App
            if (params->appFWInfo.totalSize > 0 && FW_Firmware_Verification(FLASH_DL_START_ADDR, params->appFWInfo.totalSize) == HAL_OK) {
                log_printf("There is a complete firmware in the download area.\r\n");
                //! 将完整的App从下载区搬运到App区
                if (OP_Flash_Copy(FLASH_DL_START_ADDR, FLASH_APP_START_ADDR, params->appFWInfo.totalSize) == OP_FLASH_OK) {
                    log_printf("Re-copy successful! System will now reboot.");
                    Delay_MS_By_NOP(50);
                    HAL_NVIC_SystemReset(); //! 复位MCU
                } else {
                    log_printf("Re-copy not successful! System will now reboot.");
                    Delay_MS_By_NOP(50);
                    HAL_NVIC_SystemReset(); //! 复位MCU
                }
            } else {
                log_printf("There is not a complete firmware in the download area.\r\n");
            }
            log_printf("Will wait indefinitely for firmware download.\n");
        }
    }
}

/**
 * @brief   超时处理函数 - 管理IAP模式下的超时机制
 * @note    
 *   - 此函数每1ms被调用一次，用于实现IAP模式的倒计时和超时处理
 *   - 持续监控通信活跃状态，即使已建立通信也会继续计时
 *   - 超时时间由IAP_TIMEOUT_MS宏定义（默认10秒）
 *   - 每秒会打印一次剩余倒计时，帮助用户了解超时状态
 *
 * @details
 *   改进的超时机制工作流程：
 *   1. 检查是否启用超时（iap_timeout_enabled）
 *   2. 持续递增计数器，不管是否检测到通信
 *   3. 每1000ms（1秒）打印一次剩余时间提示
 *   4. 达到超时时间后，根据App有效性智能处理：
 *      - App有效：跳转到App程序
 *      - App无效：重置YModem协议，重新等待升级
 */
static void Timeout_Handler_MS(void)
{
    //! === IAP 超时检查逻辑（每1ms执行） ===
    if (iap_timeout_enabled) {
        iap_timeout_counter++; // 每1ms累加1
        
        // 每秒打印一次倒计时
        if (iap_timeout_counter % 1000 == 0) {
#if LOG_ENABLE
            uint32_t remaining_seconds = (IAP_TIMEOUT_MS - iap_timeout_counter) / 1000;
            const char* status = iap_communication_detected ? "Active" : "Waiting";
            log_printf("IAP timeout (%s): %lu seconds remaining...\n", status, (unsigned long)remaining_seconds);
#endif
        }
        
        // 超时检查
        if (iap_timeout_counter >= IAP_TIMEOUT_MS) {
            log_printf("IAP timeout reached! Handling based on boot reason...\n");
            log_printf("Original boot flag: 0x%08lX%08lX\n", 
                      (unsigned long)(gUpdateFlag >> 32), (unsigned long)gUpdateFlag);
            
            // 智能超时处理
            if (gUpdateFlag == FIRMWARE_UPDATE_MAGIC_WORD) {
                //! App请求进入IAP模式，App肯定有效
                log_printf("Boot reason: App request -> Jumping back to App...\n");
                HAL_Delay(100);
                IAP_Ready_To_Jump_App();
            } else {
                bool app_valid = Is_App_Valid_Enhanced(FLASH_APP_START_ADDR);
                if (app_valid) {
                    log_printf("Boot reason: Other, App valid -> Jumping to App...\n");
                    HAL_Delay(100);
                    IAP_Ready_To_Jump_App();
                } else {
                    log_printf("Boot reason: Other, App invalid -> Resetting YModem for retry...\n");
                    iap_complete_pending = false;
                    iap_complete_delay_counter = 0;
                    iap_communication_detected = false;
                    iap_timeout_counter = 0;
                    
                    //! 重置心跳功能，重新开始发送'C'字符
                    iap_heartbeat_enabled = true;
                    iap_heartbeat_counter = 0;
                    
                    YModem_Reset(&gYModemHandler);
                    
                    //! 立即发送'C'字符，告知上位机可以重新开始传输
                    if (gYModemHandler.state == YMODEM_STATE_WAIT_FILE_INFO) {
                        uint8_t heartbeat_char = 0x43; // 'C'字符
                        USART_Put_TxData_To_Ringbuffer(&gUsart1Drv, &heartbeat_char, 1);
                        log_printf("IAP heartbeat: immediate 'C' sent after timeout reset.\r\n");
                    }
                    
                    log_printf("YModem reset completed, waiting for new firmware...\n");
                }
            }
        }
    }
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
