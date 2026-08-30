//这个文件是”控制屏幕亮灭”的model层，传统的model层只能从它这里获取数据，而不能替代它。因为传统的model层之和ui的要的数据有关，熄屏的时候传统model层是用不了的
/*
 * motion_service.c
 *
 *  Encapsulates MPU6050 motion detection and backlight control.
 *  No business logic leaks into main.c or the UI layer — this is the single
 * owner.
 */

#include "motion_service.h"
#include "backlight_service.h"
#include "brightness_mode.h"
#include "bsp_ili9341_4line.h"
#include "main.h"
#include "mpu6050.h"
#include "stm32u5xx_hal.h"
#include "stm32u5xx_hal_gpio.h"
#include <math.h>
#include <stdio.h>

extern volatile uint32_t hw_ms; /* 全局硬件时基: TIM3 中断递增 (定义于 stm32u5xx_it.c) */

/* ---- Tunable parameters ---- */
#define SCREEN_TIMEOUT_MS                                                      \
  10000 /* idle time before auto-off (ms) */ // 自动熄屏时间为10秒
#define TILT_ON_ANGLE 35.0f  // 倾斜角<35°视为"正在看表"，配合jerk唤醒
#define JERK_THRESHOLD 0.35f // 加速度变化率，确保是抬手动作
#define RAD_TO_DEG 57.29578f

/* ---- 环境光 -> 背光亮度 ---- */
#define AMBIENT_BRIGHT_MAX 100u
#define AMBIENT_BRIGHT_MIN 20u
#define AMBIENT_HYST       15u /* 滞回: 目标亮度变化超过该值才应用, 防抖 */

/* ---- Internal state ---- */
static I2C_HandleTypeDef *m_i2c = NULL;
static MPU6050_t m_mpu;

static uint8_t m_screen_on = 0;          // 是否亮屏
static uint8_t m_keep_awake = 0;         // 保持唤醒: 秒表运行中禁止自动熄屏
static uint32_t m_last_motion_tick = 0;  // 最后一次抬手动作的时刻(仅主循环更新,无ISR竞态)
static uint32_t m_touch_count = 0;       // 触摸事件计数(ISR递增,主循环消费)
static uint32_t m_last_seen_touch_count = 0;    // 上次主循环见过的触摸计数
static uint32_t m_last_touch_activity_tick = 0; // 最近一次检测到触摸活动的时刻(仅主循环更新)
static uint8_t m_initialised = 0;
static float m_tilt_angle = 0.0f;        // 当前倾斜角
static float m_prev_mag = 0.0f;          // 上一帧加速度矢量模长
static uint8_t m_ambient_target = 60u;//AMBIENT_BRIGHT_MAX; /* 环境光推算的目标亮度 */
static uint8_t m_touch_wake_armed = 0; /* 熄屏后允许触摸唤醒(唤醒一次后清零) */

/* 环境光(lux) -> 目标亮度(%) 的分段线性映射点 */
static uint8_t lux_to_brightness(float lux) {
  static const struct { float lux; uint8_t b; } PTS[] = {
    {0.0f,     AMBIENT_BRIGHT_MIN},
    {30.0f,    20u},
    {100.0f,   40u},
    {300.0f,   60u},
    {1000.0f,  AMBIENT_BRIGHT_MAX},
  };
  const uint8_t n = sizeof(PTS) / sizeof(PTS[0]);

  if (lux <= PTS[0].lux) return PTS[0].b;
  if (lux >= PTS[n - 1].lux) return PTS[n - 1].b;
  for (uint8_t i = 1; i < n; i++) {
    if (lux <= PTS[i].lux) {
      float t = (lux - PTS[i - 1].lux) / (PTS[i].lux - PTS[i - 1].lux);
      return PTS[i - 1].b + (uint8_t)(t * (float)(PTS[i].b - PTS[i - 1].b));
    }
  }
  return AMBIENT_BRIGHT_MAX;
}

static inline float
accel_magnitude(float ax, float ay,
                float az) // 计算加速度矢量模长，是个内部函数
{
  return sqrt(ax * ax + ay * ay + az * az);
}
static inline float
compute_tilt(float ax, float ay,
             float az) // 设备倾斜角: 0°=屏幕水平朝上, 90°=屏幕垂直
{
  /* Standard tilt: angle between Z-axis (screen normal) and gravity.
   * in-plane = sqrt(ax² + ay²) — components parallel to screen
   * out-of-plane = |az|        — component perpendicular to screen */
  float in_plane = sqrtf(ax * ax + ay * ay);
  if (in_plane < 0.001f && fabsf(az) < 0.001f)
    return 0.0f;
  return atan2f(in_plane, fabsf(az)) * RAD_TO_DEG;
}

/* ========================================================================
 * Public API
 * ======================================================================== */

void MotionService_Init(I2C_HandleTypeDef *i2cHandle) {
  m_i2c = i2cHandle;

  /* Wake MPU6050 and get a baseline reading */
  if (MPU6050_Init(m_i2c) == 0) {
    printf("[MPU] Init OK\r\n");
    printf("[MPU] Raw: X=%d Y=%d Z=%d\r\n",
           m_mpu.Accel_X_RAW, m_mpu.Accel_Y_RAW, m_mpu.Accel_Z_RAW);
    MPU6050_Read_Accel(m_i2c, &m_mpu);
    m_prev_mag = accel_magnitude(m_mpu.Ax, m_mpu.Ay, m_mpu.Az);
    m_tilt_angle = compute_tilt(m_mpu.Ax, m_mpu.Ay, m_mpu.Az);
    m_initialised = 1;
  } else {
    printf("[MPU] Init FAILED! Check wiring.\r\n");
    m_initialised = 0;
  }

  /* ILI9341_Init has already run (called in main before MotionService_Init).
   * Explicitly wake to guarantee the display controller is out of sleep. */
  ILI9341_Wake();
  Backlight_SetBrightness(AMBIENT_BRIGHT_MAX); /* 初始满亮, 首个环境光读数后再调整 */
  m_screen_on = 1;
  m_touch_wake_armed = 0;
  m_last_motion_tick = hw_ms;
  m_last_touch_activity_tick = hw_ms;
  m_last_seen_touch_count = m_touch_count;
  printf("[MPU] Screen ON (init)\r\n");
}

void MotionService_Update(void) {
  uint32_t now = hw_ms; /* 时间源: TIM3 硬件时基 (心跳 LED 已移至 TIM3 中断) */

  if (!m_initialised)
    return;

  /* ── 触摸活动检测: 计数有变化 或 引脚按着 → 刷新触摸活动时刻 ── */
  /* (计数在ISR递增, 这里仅主循环读取比较, 无回绕竞态) */
  uint8_t touch_activity = 0;
  if (m_touch_count != m_last_seen_touch_count) {
    m_last_seen_touch_count = m_touch_count;
    m_last_touch_activity_tick = now;
    touch_activity = 1;
  }
  if (HAL_GPIO_ReadPin(TP_INT_GPIO_Port, TP_INT_Pin) == GPIO_PIN_RESET) {
    m_last_touch_activity_tick = now;
    touch_activity = 1;
  }

  /* ── 触摸唤醒: 熄屏后出现触摸(新按下事件或引脚被按住) → 亮屏 ── */
  if (!m_screen_on && m_touch_wake_armed && touch_activity) {
    m_touch_wake_armed = 0; /* 只唤醒一次, 避免按住期间每帧重复 ILI9341_Wake(内部有120ms延时) */
    ILI9341_Wake();
    m_screen_on = 1;
    Backlight_SetBrightness(m_ambient_target); /* 按环境光恢复亮度 */
    printf("[MPU] >>> SCREEN ON (touch) <<<\r\n");
  }

  /*读加速度*/
  MPU6050_Read_Accel(m_i2c, &m_mpu); // 读加速度
  /*计算两个关键值*/
  float mag = accel_magnitude(m_mpu.Ax, m_mpu.Ay, m_mpu.Az);
  float jerk = fabsf(
      mag - m_prev_mag); // 加速度幅值变化率（这个函数为取绝对值，之前的减现在的
  m_prev_mag = mag;

  float tilt = compute_tilt(m_mpu.Ax, m_mpu.Ay, m_mpu.Az);
  m_tilt_angle = tilt;

#if 0 /* 暂时屏蔽 MPU 调试串口输出(联调 MAX30102 时清屏用) */
  /* ---- debug: throttled serial output every 500ms ---- */
  {
    static uint32_t last_print = 0;
    if (now - last_print >= 500) {
      printf("[MPU] tilt=%d.%d jerk=%d.%03d screen=%d touch=%lu idleM=%lu idleT=%lu | X=%d Y=%d Z=%d\r\n",
             (int)tilt, (int)(fabsf(tilt)*10)%10,
             (int)jerk, (int)(jerk*1000)%1000,
             m_screen_on,
             (unsigned long)m_touch_count,
             (unsigned long)(now - m_last_motion_tick),
             (unsigned long)(now - m_last_touch_activity_tick),
             m_mpu.Accel_X_RAW, m_mpu.Accel_Y_RAW, m_mpu.Accel_Z_RAW);
      last_print = now;
    }
  }
#endif

  /*抬手亮屏判断*/
  int is_horizontal = (fabsf(tilt) < TILT_ON_ANGLE); // 姿态接近水平
  int has_motion = (jerk > JERK_THRESHOLD);          // 有加速动作

  /* ── Wake: wrist-raise detected ── */
  if (is_horizontal && has_motion) {
    if (!m_screen_on) {
      ILI9341_Wake();
      m_screen_on = 1;
      Backlight_SetBrightness(m_ambient_target); /* 按环境光恢复亮度 */
      printf("[MPU] >>> SCREEN ON <<<\r\n");
    }
    m_last_motion_tick = now; // extend screen-on time
  }

  /* ── 秒表运行中(keepAwake): 禁止自动熄屏, 并刷新活动时刻(解除后不会立刻熄屏) ── */
  if (m_keep_awake) {
    m_last_motion_tick = now;
    m_last_touch_activity_tick = now;
  }

  /* ── Sleep: 无抬手动作超时 且 无触摸活动超时, 才熄屏 ── */
  /* 抬手刷新 m_last_motion_tick; 触摸(计数变化或轮询)刷新 m_last_touch_activity_tick */
  if (m_screen_on && !m_keep_awake && (now - m_last_motion_tick > SCREEN_TIMEOUT_MS) && (now - m_last_touch_activity_tick > SCREEN_TIMEOUT_MS)) {
    ILI9341_Sleep();
    m_screen_on = 0;
    m_touch_wake_armed = 1; /* 熄屏后允许触摸唤醒 */
    Backlight_SetBrightness(0); /* 熄屏: 背光关 */
    printf("[MPU] Screen OFF (timeout, touch=%lu)\r\n", (unsigned long)m_touch_count);
  }
}

void MotionService_SetAmbientLux(float lux) {
  uint8_t new_target = lux_to_brightness(lux);
  if (Brightness_GetMode() == BRIGHT_MODE_MANUAL) return;
  /* 滞回: 目标亮度变化足够大才应用, 避免临界光强下反复跳变 */
  if ((new_target >= (uint8_t)(m_ambient_target + AMBIENT_HYST)) ||
      (new_target + AMBIENT_HYST <= m_ambient_target)) {
    m_ambient_target = new_target;
    if (m_screen_on)
      Backlight_SetBrightness(m_ambient_target);
  }
}

void MotionService_SetKeepAwake(uint8_t keepAwake) { m_keep_awake = keepAwake; }

void MotionService_ForceScreenOn(void) {
  uint32_t now = hw_ms;
  if (!m_screen_on) {
    ILI9341_Wake();
    m_screen_on = 1;
    m_touch_wake_armed = 0;
    Backlight_SetBrightness(m_ambient_target); /* 按环境光恢复亮度 */
    printf("[MPU] >>> SCREEN ON (force) <<<\r\n");
  }
  /* 刷新活动计时: 响铃期间持续保持亮屏 */
  m_last_motion_tick = now;
  m_last_touch_activity_tick = now;
}

uint8_t MotionService_IsScreenOn(void) { return m_screen_on; }

uint32_t MotionService_GetTime(void) { return hw_ms; }

void MotionService_OnTouchActivity(void) {
  /* 中断上下文: 只递增计数, 不写时间戳(避免与主循环的无符号回绕竞态) */
  m_touch_count++;
}