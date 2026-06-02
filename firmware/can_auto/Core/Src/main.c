#include "main.h"
#include "stm32l4xx_hal.h"
#include "lvgl.h"
#include "stm32l4xx_hal_tim.h"
#include "lv_conf.h"
#include "obd_data.h"
#include "lv_port_disp.h"
#include "lv_interface.h"
#include "ili9488.h"
#include "lv_port_indev.h"
#include "core_cm4.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#define MY_DISP_HOR_RES 480
#define MY_DISP_VER_RES 320

CAN_HandleTypeDef hcan1;
UART_HandleTypeDef huart2;
SPI_HandleTypeDef hspi2;
TIM_HandleTypeDef htim6;

volatile uint8_t pid_responses_received = 0;

/* --- latencija --- */
volatile uint32_t t_req_us       = 0;   /* vrijeme slanja */
volatile uint32_t lat_acc_us     = 0;   /* zbroj za prosjek */
volatile uint32_t lat_max_us     = 0;   /* najgora vrijednost */
volatile uint32_t lat_samples    = 0;   /* broj uzoraka */

/* --- brojač okvira za bus-load --- */
volatile uint32_t tx_cnt = 0;
volatile uint32_t rx_cnt = 0;
static uint32_t dbg_tick = 0;

static inline void DWT_Init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;  /* otključa DWT */
    DWT->CYCCNT = 0;
    DWT->CTRL  |= DWT_CTRL_CYCCNTENA_Msk;            /* start */
}

/* pretvori cikluse u µs (CLK = 80 MHz) */
static inline uint32_t dwt_us(void)
{
    return DWT->CYCCNT / 80u;
}



void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_CAN1_Init(void);
static void MX_SPI2_Init(void);
static void MX_TIM6_Init(void);

static const uint8_t pid_list[] = {
    0x0C,  // RPM
    0x0D,  // Speed
    0x05,  // Coolant Temp
    0x0F,  // Intake Air Temp
    0x11,  // Throttle Position
    0x10,  // MAF
    0x0B,  // MAP
    0x1C,  // Lambda Voltage
    0x1F   // Runtime
};

#define NUM_PIDS (sizeof(pid_list)/sizeof(pid_list[0]))

void send_OBD_Request(uint8_t pid) {
    uint8_t message[8] = {0x02, 0x01, pid, 0,0,0,0,0};
    CAN_TxHeaderTypeDef txHeader = {0};
    uint32_t txMailbox;

    txHeader.StdId = 0x7DF;
    txHeader.IDE = CAN_ID_STD;
    txHeader.RTR = CAN_RTR_DATA;
    txHeader.DLC = 8;
    t_req_us = dwt_us();
    if (HAL_CAN_AddTxMessage(&hcan1, &txHeader, message, &txMailbox) == HAL_OK)
        tx_cnt++;
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) {
    CAN_RxHeaderTypeDef rxH;
    uint8_t d[8];

    if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rxH, d) != HAL_OK)
        return;
    uint32_t delta = dwt_us() - t_req_us;    /* µs od slanja do prijema */

    lat_acc_us  += delta;
    if(delta > lat_max_us) lat_max_us = delta;
    lat_samples++;

    rx_cnt++;

    if (rxH.StdId == 0x7E8 && d[1] == 0x41) {
        uint8_t pid = d[2];

        switch (pid) {
            case 0x0C: obdData.rpm = (((uint16_t)d[3] << 8) | d[4]) / 4; break;
            case 0x0D: obdData.speed = d[3]; break;
            case 0x05: obdData.coolant = (int8_t)d[3] - 40; break;
            case 0x0F: obdData.intake = (int8_t)d[3] - 40; break;
            case 0x11: obdData.throttle = (d[3] * 100) / 255; break;
            case 0x10: obdData.maf = (((uint16_t)d[3] << 8) | d[4]) / 100.0f; break;
            case 0x0B: obdData.map = d[3]; break;
            case 0x1C: obdData.lambdaV = d[3] * 0.005f; break;
            case 0x1F: obdData.runtime = ((uint16_t)d[3] << 8) | d[4]; break;
            default: return;
        }

        pid_responses_received++;
    }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if(htim->Instance == TIM6)
        lv_tick_inc(1);
}

void print_all_data(void)
{
    float afr = 14.7f, dens = 736.0f;
    obdData.fuelRate = (obdData.maf / afr / dens) * 3600.0f;

    ui_set_rpm(obdData.rpm);
    ui_set_speed(obdData.speed);
    ui_set_temp(obdData.coolant, obdData.intake);
    ui_set_throttle(obdData.throttle);
    ui_set_fuel_rate(obdData.fuelRate);
    ui_set_map(obdData.map);
    ui_set_lambda(obdData.lambdaV);
    ui_set_runtime(obdData.runtime);

    char buf[256];
    int len = sprintf(buf,
        "RPM:%u,S:%ukm/h,C:%d°C,I:%d°C,Thr:%u%%,MAF:%.2fg/s,MAP:%ukPa,λ:%.2fV,Run:%lus,FR:%.2fL/h\r\n",
        obdData.rpm,
        obdData.speed,
        obdData.coolant,
        obdData.intake,
        obdData.throttle,
        obdData.maf,
        obdData.map,
        obdData.lambdaV,
        obdData.runtime,
        obdData.fuelRate
    );

    HAL_UART_Transmit(&huart2, (uint8_t*)buf, len, HAL_MAX_DELAY);
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    DWT_Init();
    MX_GPIO_Init();
    MX_SPI2_Init();
    MX_USART2_UART_Init();
    MX_CAN1_Init();
    MX_TIM6_Init();
    HAL_TIM_Base_Start_IT(&htim6);

    HAL_CAN_Start(&hcan1);
    HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING);

    lv_init();
    lv_port_disp_init();
    lv_port_indev_init();

    ux_init();

    for (int i = 0; i < 10; i++)     /* 10 × 1 ms ≈ dovoljno da flush-a 8 blokova */
    {
        lv_timer_handler();          /* nennt se u v9 */
        HAL_Delay(1);
    }

    HAL_UART_Transmit(&huart2, (uint8_t*)"Init...\r\n", 9, HAL_MAX_DELAY);

    uint8_t idx = 0;

    while (1)
    {
        pid_responses_received = 0;

        for (idx = 0; idx < NUM_PIDS; idx++) {
            send_OBD_Request(pid_list[idx]);
            HAL_Delay(30);
        }

        // Čekaj da dobijemo sve odgovore
        uint32_t timeout = HAL_GetTick();
        while (pid_responses_received < NUM_PIDS && (HAL_GetTick() - timeout < 500));

        print_all_data();

        /* ---------------- Debug svake 2 s ---------------- */
        if(HAL_GetTick() - dbg_tick >= 2000)
        {
            uint32_t avg = (lat_samples) ? lat_acc_us / lat_samples : 0;

            char dbg[64];
            int n = sprintf(dbg, "LAT,%lu,%lu,%lu\r\n",
                            (unsigned long)lat_samples,
                            (unsigned long)avg,
                            (unsigned long)lat_max_us);
            HAL_UART_Transmit(&huart2, (uint8_t*)dbg, n, HAL_MAX_DELAY);

            n = sprintf(dbg, "BUS,%lu,%lu\r\n",
                        (unsigned long)tx_cnt, (unsigned long)rx_cnt);
            HAL_UART_Transmit(&huart2, (uint8_t*)dbg, n, HAL_MAX_DELAY);

            /* resetiraj brojače za sljedeći ciklus */
            lat_acc_us = lat_max_us = lat_samples = 0;
            tx_cnt = rx_cnt = 0;
            dbg_tick = HAL_GetTick();
        }


        lv_timer_handler();
        HAL_Delay(100);  // update otprilike svakih 0.5 sek
    }
}



static void MX_SPI2_Init(void) {
    hspi2.Instance = SPI2;
    hspi2.Init.Mode = SPI_MODE_MASTER;
    hspi2.Init.Direction = SPI_DIRECTION_2LINES;
    hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi2.Init.NSS = SPI_NSS_SOFT;
    hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
    hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi2.Init.CRCPolynomial = 10;

    if (HAL_SPI_Init(&hspi2) != HAL_OK) {
        Error_Handler();
    }
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 10;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief CAN1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_CAN1_Init(void)
{

  /* USER CODE BEGIN CAN1_Init 0 */

  /* USER CODE END CAN1_Init 0 */

  /* USER CODE BEGIN CAN1_Init 1 */

  /* USER CODE END CAN1_Init 1 */
  hcan1.Instance = CAN1;
  hcan1.Init.Prescaler = 16;
  hcan1.Init.Mode = CAN_MODE_NORMAL;
  hcan1.Init.SyncJumpWidth = CAN_SJW_1TQ;
  hcan1.Init.TimeSeg1 = CAN_BS1_8TQ;
  hcan1.Init.TimeSeg2 = CAN_BS2_1TQ;
  hcan1.Init.TimeTriggeredMode = DISABLE;
  hcan1.Init.AutoBusOff = DISABLE;
  hcan1.Init.AutoWakeUp = DISABLE;
  hcan1.Init.AutoRetransmission = DISABLE;
  hcan1.Init.ReceiveFifoLocked = DISABLE;
  hcan1.Init.TransmitFifoPriority = DISABLE;
  if (HAL_CAN_Init(&hcan1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CAN1_Init 2 */
  CAN_FilterTypeDef canFilterConfig;
  canFilterConfig.FilterActivation = CAN_FILTER_ENABLE;
  canFilterConfig.FilterBank = 0;  // Prvi bank filtera
  canFilterConfig.FilterFIFOAssignment = CAN_FILTER_FIFO0;
  canFilterConfig.FilterIdHigh = 0x0000;     // Propusti sve poruke
  canFilterConfig.FilterIdLow = 0x0000;
  canFilterConfig.FilterMaskIdHigh = 0x0000;
  canFilterConfig.FilterMaskIdLow = 0x0000;
  canFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
  canFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
  canFilterConfig.FilterIdHigh = 0x07E8 << 5;
  canFilterConfig.FilterMaskIdHigh = 0x07F8 << 5;


  if (HAL_CAN_ConfigFilter(&hcan1, &canFilterConfig) != HAL_OK)
  {
      Error_Handler();
  }

  /* USER CODE END CAN1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* ----------------------- Onboard LED (LD2) ----------------------- */
    /* PA5 */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin   = GPIO_PIN_5;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* ------------------------ User button (B1) ----------------------- */
    /* obično PC13 */
    GPIO_InitStruct.Pin  = B1_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

    /* ------------------ TFT Display Control Pins -------------------- */
    /* PA8 = TFT_CS, PA9 = TFT_DC/RS, PA10 = TFT_RST, PA11 = TFT_LED */
    /* INIT: CS/DC/RST/LED = LOW */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin   = GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* Reset TFT */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_RESET);
    HAL_Delay(20);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_SET);
    HAL_Delay(120);

    /* Back-light ON */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_SET);

    /* ----------------------- Touch Controller ------------------------ */
    /* PA12 = TOUCH_CS */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_SET);  // idle = HIGH
    GPIO_InitStruct.Pin   = GPIO_PIN_12;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* PC0 = TOUCH_IRQ (EXTI on falling edge) */
    GPIO_InitStruct.Pin  = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    /* EXTI0 interrupt init */
    HAL_NVIC_SetPriority(EXTI0_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);

    /* ------------------------- SPI2 Pins ----------------------------- */
    /* PB13 = SPI2_SCK, PB14 = SPI2_MISO, PB15 = SPI2_MOSI */
    GPIO_InitStruct.Pin       = GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* ----------------------- USART2 Pins ----------------------------- */
    /* PA2 = USART2_TX, PA3 = USART2_RX */
    GPIO_InitStruct.Pin       = GPIO_PIN_2|GPIO_PIN_3;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* ------------------------ CAN1 Pins ------------------------------ */
    /* PB8 = CAN1_RX, PB9 = CAN1_TX */
    GPIO_InitStruct.Pin       = GPIO_PIN_8|GPIO_PIN_9;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF9_CAN1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* Note: PA13/PA14 left as SWD (TMS/TCK), do not reconfigure */
}


void MX_TIM6_Init(void)
{
    __HAL_RCC_TIM6_CLK_ENABLE();

    htim6.Instance = TIM6;
    htim6.Init.Prescaler = 7999;       // (80 MHz / (7999+1)) = 10 kHz
    htim6.Init.Period = 9;             // 10 kHz / (9+1) = 1 kHz → 1ms perioda
    htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    if (HAL_TIM_Base_Init(&htim6) != HAL_OK) {
        Error_Handler();
    }

    HAL_NVIC_SetPriority(TIM6_DAC_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(TIM6_DAC_IRQn);
}




/* USER CODE BEGIN 4 */

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
