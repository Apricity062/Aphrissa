/*
 * buzzer_service.c
 *
 *  PA1 低电平触发蜂鸣器: 高=静音, 低=响。
 *  响铃节奏: 响 500ms / 停 500ms 循环, 由 TIM3 1ms 中断调 Buzzer_Tick 驱动。
 *
 *  ⚠️ PA2 → PA1 迁移: 原 PA2 不响(硬件原因), 已改为 PA1
 *     (.ioc 中 PA1 配置为 GPIO_Output + PinState=SET 上电静音)。
 */

#include "buzzer_service.h"
#include "stm32u5xx.h"
#include "stm32u5xx_hal_gpio.h"
#include "motion_service.h" /* MotionService_GetTime() 返回 TIM3 hw_ms */

/* 节奏参数 (ms) */
#define BUZZ_ON_MS  500u
#define BUZZ_OFF_MS 500u

#define BUZZ_GPIO_PORT GPIOA
#define BUZZ_GPIO_PIN  GPIO_PIN_1

static volatile uint8_t  s_pattern_active = 0;  /* 节奏是否运行 */
static volatile uint32_t s_pattern_start  = 0;  /* 节奏起始时刻 */

void Buzzer_Init(void) { HAL_GPIO_WritePin(BUZZ_GPIO_PORT, BUZZ_GPIO_PIN, GPIO_PIN_SET); }

void Buzzer_Set(bool on){
  if (on) {
    HAL_GPIO_WritePin(BUZZ_GPIO_PORT, BUZZ_GPIO_PIN, GPIO_PIN_RESET);  /* 低=响 */
  }
  else{
    HAL_GPIO_WritePin(BUZZ_GPIO_PORT, BUZZ_GPIO_PIN, GPIO_PIN_SET);   /* 高=静音 */
  }
}

void Buzzer_Tick(void) {
  if (!s_pattern_active)
    return;

  uint32_t phase = MotionService_GetTime() - s_pattern_start;
  uint32_t period = BUZZ_ON_MS + BUZZ_OFF_MS;
  uint32_t in_period = phase % period;

  Buzzer_Set(in_period < BUZZ_ON_MS);   /* 前 500ms 响, 后 500ms 静音 */
}

void Buzzer_StartPattern(void) {
  s_pattern_active = 1;
  s_pattern_start  = MotionService_GetTime();
  Buzzer_Set(true);                     /* 立即开始响 */
}

void Buzzer_Stop(void) {
  s_pattern_active = 0;
  Buzzer_Set(false);  /* 立即静音 */
}
