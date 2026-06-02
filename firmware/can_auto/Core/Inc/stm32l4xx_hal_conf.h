#ifndef STM32L4xx_HAL_CONF_H
#define STM32L4xx_HAL_CONF_H

#ifdef __cplusplus
 extern "C" {
#endif

/* ########################## Module Selection ############################## */
#define HAL_MODULE_ENABLED
#include "stm32l4xx_hal_def.h"
#define HAL_CAN_MODULE_ENABLED
#define HAL_SPI_MODULE_ENABLED
#define HAL_TIM_MODULE_ENABLED
#define HAL_UART_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_EXTI_MODULE_ENABLED
#define HAL_DMA_MODULE_ENABLED
#define HAL_RCC_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED
#define HAL_PWR_MODULE_ENABLED
#define HAL_CORTEX_MODULE_ENABLED

/* ########################## Oscillator Values ############################# */
#define HSE_VALUE    ((uint32_t)8000000U)
#define HSE_STARTUP_TIMEOUT    ((uint32_t)100U)
#define MSI_VALUE    ((uint32_t)4000000U)
#define HSI_VALUE    ((uint32_t)16000000U)
#define HSI48_VALUE   ((uint32_t)48000000U)
#define LSI_VALUE  32000U
#define LSE_VALUE    32768U
#define LSE_STARTUP_TIMEOUT    5000U
#define EXTERNAL_SAI1_CLOCK_VALUE    2097000U
#define EXTERNAL_SAI2_CLOCK_VALUE    2097000U

/* ########################### System Configuration ######################### */
#define  VDD_VALUE					  3300U
#define  TICK_INT_PRIORITY            0U
#define  USE_RTOS                     0U
#define  PREFETCH_ENABLE              1U
#define  INSTRUCTION_CACHE_ENABLE     1U
#define  DATA_CACHE_ENABLE            1U

/* ################## Register callback feature configuration ############### */
#define USE_HAL_CAN_REGISTER_CALLBACKS        0U
#define USE_HAL_SPI_REGISTER_CALLBACKS        0U
#define USE_HAL_TIM_REGISTER_CALLBACKS        0U
#define USE_HAL_UART_REGISTER_CALLBACKS       0U
#define USE_HAL_GPIO_REGISTER_CALLBACKS       0U
#define USE_HAL_DMA_REGISTER_CALLBACKS        0U
#define USE_HAL_EXTI_REGISTER_CALLBACKS       0U
#define USE_HAL_PWR_REGISTER_CALLBACKS        0U
#define USE_HAL_FLASH_REGISTER_CALLBACKS      0U
#define USE_HAL_RCC_REGISTER_CALLBACKS        0U

/* ################## SPI peripheral configuration ########################## */
#define USE_SPI_CRC                   0U

/* Includes ------------------------------------------------------------------*/
#ifdef HAL_RCC_MODULE_ENABLED
  #include "stm32l4xx_hal_rcc.h"
#endif

#ifdef HAL_GPIO_MODULE_ENABLED
  #include "stm32l4xx_hal_gpio.h"
#endif

#ifdef HAL_DMA_MODULE_ENABLED
  #include "stm32l4xx_hal_dma.h"
#endif

#ifdef HAL_CORTEX_MODULE_ENABLED
  #include "stm32l4xx_hal_cortex.h"
#endif

#ifdef HAL_CAN_MODULE_ENABLED
  #include "stm32l4xx_hal_can.h"
#endif

#ifdef HAL_EXTI_MODULE_ENABLED
  #include "stm32l4xx_hal_exti.h"
#endif

#ifdef HAL_FLASH_MODULE_ENABLED
  #include "stm32l4xx_hal_flash.h"
#endif

#ifdef HAL_PWR_MODULE_ENABLED
  #include "stm32l4xx_hal_pwr.h"
#endif

#ifdef HAL_SPI_MODULE_ENABLED
  #include "stm32l4xx_hal_spi.h"
#endif

#ifdef HAL_TIM_MODULE_ENABLED
  #include "stm32l4xx_hal_tim.h"
#endif

#ifdef HAL_UART_MODULE_ENABLED
  #include "stm32l4xx_hal_uart.h"
#endif

/* Exported macro ------------------------------------------------------------*/
#ifdef  USE_FULL_ASSERT
  #define assert_param(expr) ((expr) ? (void)0U : assert_failed((uint8_t *)__FILE__, __LINE__))
  void assert_failed(uint8_t *file, uint32_t line);
#else
  #define assert_param(expr) ((void)0U)
#endif

#ifdef __cplusplus
}
#endif

#endif /* STM32L4xx_HAL_CONF_H */
