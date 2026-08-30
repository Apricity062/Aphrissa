/*
 * backlight_service.h
 *
 *  LCD backlight control via software PWM on LCD_BL (PB10).
 *  Driven by the TIM3 1ms tick; no CubeMX/hardware changes needed.
 */

#ifndef INC_BACKLIGHT_SERVICE_H_
#define INC_BACKLIGHT_SERVICE_H_

#define BACKLIGHT_USER_MIN_PERCENT 20u // 最小亮度：20

#include <stdint.h>

/* ---- Public API ---- */

/** One-time init: pin low (backlight off), reset PWM state. */
void Backlight_Init(void);

/** Software-PWM step. Call once per millisecond from the TIM3 update ISR. */
void Backlight_Tick(void);

/** Set desired brightness, 0..100 (0 = off). Applies with a smooth ramp. */
void Backlight_SetBrightness(uint8_t percent);

/** Current (ramped) brightness in percent, 0..100. */
uint8_t Backlight_GetBrightness(void);

/** Target brightness in percent, 0..100. */
uint8_t Backlight_GetTargetBrightness(void);

void Backlight_SetUserBrightness(uint8_t percent);//用户亮度意图

#endif /* INC_BACKLIGHT_SERVICE_H_ */
