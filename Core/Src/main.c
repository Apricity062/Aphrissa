/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
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
#include "app_touchgfx.h"
#include "crc.h"
#include "gpdma.h"
#include "gpio.h"
#include "i2c.h"
#include "rng.h"
#include "rtc.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"


/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "alarm_service.h"
#include "backlight_service.h"
#include "brightness_mode.h"
#include "bsp_ft6336.h"
#include "bsp_ili9341_4line.h"
#include "max30102_for_stm32_hal.h"
#include "max30102_service.h"
#include "motion_service.h"
#include "time_service.h"
#include "veml7700.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
static veml7700_t g_veml;
static uint8_t g_als_valid = 0;
static uint16_t g_als_raw = 0;
static float g_als_lux = 0.0f;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* 扫描指定 I2C 总线: 打印所有有应答的从机地址(7位地址)。 */
static void i2c_scan(const char *tag, I2C_HandleTypeDef *hi2c) {
  printf("[%s] scanning 0x01..0x7F ...\r\n", tag);
  uint8_t found = 0;
  for (uint16_t addr = 0x01; addr <= 0x7F; addr++) {
    if (HAL_I2C_IsDeviceReady(hi2c, (uint16_t)(addr << 1), 2, 10) == HAL_OK) {
      printf("[%s] device @ 0x%02X (write 0x%02X)\r\n", tag, addr, addr << 1);
      found++;
    }
  }
  if (found == 0) {
    printf("[%s] NO device responded (check wiring/power/pull-ups)\r\n", tag);
  } else {
    printf("[%s] %u device(s) found\r\n", tag, (unsigned)found);
  }
}

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick.
   */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_GPDMA1_Init();
  MX_I2C1_Init();
  MX_SPI1_Init();
  MX_CRC_Init();
  MX_USART1_UART_Init();
  MX_TIM3_Init();
  MX_RNG_Init();
  MX_I2C2_Init();
  MX_RTC_Init();
  MX_TouchGFX_Init();
  /* USER CODE BEGIN 2 */
  Backlight_Init(); /* 背光软件PWM接管 PB10 (随后由环境光控制) */
  ILI9341_Init();   /* 显示屏初始化 (2.8寸 ILI9341, 4线SPI) */
  FT6336_init();    /* 触摸屏初始化 (FT6336, TP_RST=PA11) */
  MotionService_Init(&hi2c1);
  USART1_StartRx(); /* 开启串口接收中断, 等待校时帧 */
  HAL_TIM_Base_Start_IT(
      &htim3); /* 启动 TIM3 硬件时基(1ms): VSYNC/心跳/全局走时 */
  /* VEML7700 临时挂到 I2C2 排查: 排除 I2C3 接线问题, 验证传感器好坏 */
  if (veml7700_init(&g_veml, &hi2c2)) {
    g_als_valid = 1;
  } else {
    printf("[LUX] init FAILED (I2C2) err=%d\r\n", (int)g_veml.last_status);
  }
  i2c_scan(
      "I2C2",
      &hi2c2); /* 调试: 扫描 I2C2, 应看到 0x57(MAX30102) 和 0x10(VEML7700) */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1) {
    /* 传感器/熄屏判断: 跟随 33ms 节拍运行, 避免主循环空转时全速读 I2C */
    static uint32_t last_update = 0;
    uint32_t now = MotionService_GetTime();
    if ((uint32_t)(now - last_update) >= 33u) {
      last_update = now;
      MotionService_Update(); /* MPU 抬手/熄屏判断 (时间源 = TIM3 hw_ms) */
    }
    Max30102_Update();              /* 读心率血氧 FIFO (内部 20ms 节流) */
    TimeService_PollPendingFrame(); /* 串口校时: 有完整行则解析写 RTC */
    /* 光照传感器 VEML7700 (I2C3): 500ms 周期读一次 */
    static uint32_t last_lux = 0;
    if (g_als_valid && (uint32_t)(now - last_lux) >= 500u) {
      last_lux = now;
      uint16_t raw = veml7700_read_als(&g_veml);
      if (g_veml.last_status == HAL_OK) {
        g_als_raw = raw;
        g_als_lux = veml7700_compute_lux(&g_veml, raw);
        MotionService_SetAmbientLux(
            g_als_lux); /* 环境光 -> 背光亮度 (自动模式生效) */
        veml7700_debug_print(&g_veml, raw, g_als_lux); /* 串口调试输出 */
      } else {
        printf("[LUX] read FAILED err=%d\r\n", (int)g_veml.last_status);
      }
    }
    /* 手动模式: 主循环直接把滑条值同步到背光 (自动模式由环境光驱动, 不覆盖) */
    if (Brightness_GetMode() == BRIGHT_MODE_MANUAL &&
        MotionService_IsScreenOn()) {
      Backlight_SetUserBrightness(Brightness_GetManualValue());
    }
    AlarmService_Tick(); /* 响铃: 强制亮屏 + 超时自动停止 */
    /* USER CODE END WHILE */

    MX_TouchGFX_Process();
    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
   */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK) {
    Error_Handler();
  }

  /** Configure LSE Drive Capability
   */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48 |
                                     RCC_OSCILLATORTYPE_LSE |
                                     RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_0;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLMBOOST = RCC_PLLMBOOST_DIV4;
  RCC_OscInitStruct.PLL.PLLM = 3;
  RCC_OscInitStruct.PLL.PLLN = 10;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 1;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLLVCIRANGE_1;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2 |
                                RCC_CLOCKTYPE_PCLK3;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK) {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
int __io_putchar(int ch) {
  HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
  return ch;
}

/* ── 触摸中断 (TP_INT = PB5, EXTI5) ── */
static uint8_t tp_pressed = 0;

void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin) {
  /* 触摸按下: 记录状态 + 刷新亮屏计时(操作时不熄屏) */
  if (GPIO_Pin == TP_INT_Pin) {
    /* 防误触: 屏幕熄灭时, 本次触摸只用于唤醒, 不产生 UI 触摸事件
       (tp_pressed 不置位 → 松手时不会调 FT6336_irq_fuc → TouchGFX 收不到) */
    if (MotionService_IsScreenOn()) {
      tp_pressed = 1;
    }
    MotionService_OnTouchActivity(); /* 触摸活动始终记录: 用于唤醒/刷新熄屏计时
                                      */
  }
}

void HAL_GPIO_EXTI_Rising_Callback(uint16_t GPIO_Pin) {
  /* 触摸释放: 通知 FT6336 产生触摸事件 + 刷新亮屏计时 */
  if (GPIO_Pin == TP_INT_Pin) {
    if (tp_pressed) {
      FT6336_irq_fuc();
      tp_pressed = 0;
    }
    MotionService_OnTouchActivity();
  }
}
/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1) {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line) {
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line
     number, ex: printf("Wrong parameters value: file %s on line %d\r\n", file,
     line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
