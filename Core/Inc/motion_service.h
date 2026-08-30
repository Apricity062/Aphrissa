/*
 * motion_service.h
 *
 *  Model-layer service: MPU6050 motion detection + backlight control.
 *  Owns all sensor-processing logic — callers just init once and update each frame.
 */

#ifndef INC_MOTION_SERVICE_H_
#define INC_MOTION_SERVICE_H_

#include <stdint.h>
#include "i2c.h"

/* ---- Public API ---- */

/** One-time init: wake MPU6050, read baseline, turn screen off. */
void MotionService_Init(I2C_HandleTypeDef *i2cHandle);

/** Call every frame (~30-60 Hz): read accel, detect raise gesture, manage backlight. */
void MotionService_Update(void);

/** Query current screen state (1 = on, 0 = off). */
uint8_t MotionService_IsScreenOn(void);

/** Return the global hardware millisecond counter (TIM3, 1ms/tick since boot). */
uint32_t MotionService_GetTime(void);

/** 临时禁止自动熄屏 (1=保持亮屏, 0=恢复自动熄屏)。秒表运行等场景用。 */
void MotionService_SetKeepAwake(uint8_t keepAwake);

/** 上报环境光亮度(lux), 自动调整背光亮度(带滞回)。主循环周期调用。 */
void MotionService_SetAmbientLux(float lux);

void MotionService_OnTouchActivity(void); /* Reset screen-off timer on touch event *///触摸不熄屏

/** 强制亮屏 (闹钟响铃等场景): 唤醒屏幕 + 刷新活动计时, 阻止自动熄屏。 */
void MotionService_ForceScreenOn(void);

#endif /* INC_MOTION_SERVICE_H_ */
