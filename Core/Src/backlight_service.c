/*
 * backlight_service.c
 *
 *  Software PWM for the LCD backlight (LCD_BL = PB10).
 *
 *  The pin is a plain GPIO, so brightness is emulated in software:
 *  a 10 ms period (100 Hz) is divided into 11 levels (0..10 -> 0..100%).
 *  Backlight_Tick() must be called once per millisecond from the TIM3
 *  update interrupt (kept light-weight: counter compare + one GPIO write).
 *
 *  The ramp moves the duty one level every 100 ms, so a full 0-100% sweep
 *  takes about one second and avoids harsh brightness jumps.
 */

#include "backlight_service.h"
#include "main.h"
#include "stm32u5xx_hal_gpio.h"

#define BL_PERIOD_MS 10u /* PWM period = 10 ms -> 100 Hz */
#define BL_LEVELS 10u    /* duty range 0..10 -> 0..100% */
#define BL_RAMP_MS 100u  /* one level per 100 ms */


static uint8_t m_duty = 0;   /* current duty 0..10 */
static uint8_t m_target = 0; /* target duty 0..10 */
static uint8_t m_level = 0;  /* current brightness in percent */
static uint8_t m_phase = 0;  /* PWM phase counter 0..9 */
static uint32_t m_tick = 0;  /* total tick count */

static inline void bl_pin(uint8_t on) {
  HAL_GPIO_WritePin(LCD_BL_GPIO_Port, LCD_BL_Pin,
                    on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void Backlight_Init(void) {
  m_duty = 0;
  m_target = 0;
  m_level = 0;
  m_phase = 0;
  m_tick = 0;
  bl_pin(0);
}

void Backlight_SetBrightness(uint8_t percent) {
  if (percent > 100)
    percent = 100;
  m_target = (uint8_t)(((uint16_t)percent * BL_LEVELS + 50u) / 100u);
}

uint8_t Backlight_GetBrightness(void) { return m_level; }

uint8_t Backlight_GetTargetBrightness(void) { return m_target; }

void Backlight_Tick(void) {
  m_tick++;

  /* smooth ramp: one level per BL_RAMP_MS */
  if ((m_tick % BL_RAMP_MS) == 0u) {
    if (m_duty < m_target)
      m_duty++;
    else if (m_duty > m_target)
      m_duty--;
    m_level = (uint8_t)(((uint16_t)m_duty * 100u + (uint16_t)BL_LEVELS / 2u) /
                        (uint16_t)BL_LEVELS);
  }

  /* PWM phase: on for the first `m_duty` ms of every 10 ms period */
  m_phase = (m_phase + 1u) % BL_PERIOD_MS;
  bl_pin((m_phase < m_duty) ? 1u : 0u);
}

void Backlight_SetUserBrightness(uint8_t percent) {//对亮度进行了+20处理和消除了100以上的部分
  if (percent > 100)
    percent = 100;
  /* 整体 +20 抬升, 保证最低也可读, 封顶 100 */
  uint8_t out = (percent >= (100u - BACKLIGHT_USER_MIN_PERCENT))
                    ? 100u
                    : (uint8_t)(percent + BACKLIGHT_USER_MIN_PERCENT);
  Backlight_SetBrightness(out);
}
