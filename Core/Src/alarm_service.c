/*
 * alarm_service.c
 *
 *  闹钟服务层实现:
 *    - 8 个闹钟 (时/分/启停), RAM 镜像 + BKP 寄存器掉电保存
 *    - RTC Alarm A 软件接力: 硬件只有 1 个闹钟, 每次修改/关闭后扫描
 *      所有启用闹钟, 把"下一个最近触发时刻"写进 Alarm A (每天重复)
 *    - 响铃状态机: 触发 → 置标志 + 蜂鸣器节奏; Dismiss/超时 → 停
 *
 *  RTC 中断入口: HAL_RTC_AlarmAEventCallback (本文件实现弱回调) → OnAlarmA
 */

#include "alarm_service.h"
#include "buzzer_service.h"
#include "motion_service.h"
#include "rtc.h" /* hrtc */
#include "stm32u5xx_hal.h"
#include "time_service.h"

/* BKP 寄存器: #1 已被时制偏好占用, 闹钟用 #2..#9 */
#define BKP_BASE_ALARM 2u

/* 响铃最长时间 (超时自动停止, 防止一直响) */
#define RING_AUTO_STOP_MS 60000u

/* ---- 数据 (RAM 镜像) ---- */
static uint8_t s_hour[ALARM_SERVICE_COUNT];
static uint8_t s_minute[ALARM_SERVICE_COUNT];
static uint8_t s_enabled[ALARM_SERVICE_COUNT];

/* ---- 响铃状态 ---- */
static volatile uint8_t s_ringing = 0;
static volatile uint8_t s_ring_index = 0;     /* 正在响的闹钟索引 */
static volatile uint32_t s_ring_start_ms = 0; /* 响铃起始时刻 (hw_ms) */

/* 当前 Alarm A 指向的闹钟索引 (触发时用它作为响铃索引) */
static uint8_t s_scheduled_index = 0;

/* ========================================================================
 * 软件接力: 扫描所有启用闹钟, 找"下一个最近触发时刻", 写进 RTC Alarm A
 * ======================================================================== */

static void alarm_reschedule(void) {
  uint16_t y;
  uint8_t mo, d, w, h, mi, s;
  TimeService_GetDateTime(&y, &mo, &d, &w, &h, &mi, &s);
  uint16_t now_min = (uint16_t)(h * 60u + mi); /* 当前时刻(分钟) */

  int16_t best_diff = -1;
  uint8_t best_idx = 0;
  uint8_t found = 0;

  for (uint8_t i = 0; i < ALARM_SERVICE_COUNT; i++) {
    if (!s_enabled[i])
      continue;
    /* 正在响铃的闹钟: 今天已响过, 排到明天 (避免响铃期间被重复调度) */
    if (s_ringing && i == s_ring_index)
      continue;

    uint16_t t = (uint16_t)(s_hour[i] * 60u + s_minute[i]);
    int16_t diff = (int16_t)(t - now_min); /* 今天剩余分钟 */
    if (diff <= 0)
      diff += (int16_t)(24 * 60); /* 今天已过 → 明天 */

    if (!found || diff < best_diff) {
      found = 1;
      best_diff = diff;
      best_idx = i;
    }
  }

  /* 没有启用闹钟: 停用 Alarm A, 避免误触发 */
  if (!found) {
    HAL_RTC_DeactivateAlarm(&hrtc, RTC_ALARM_A);
    s_scheduled_index = 0;
    return;
  }

  /* 设置 Alarm A: 掩掉日期/星期 → 每天同一时刻重复 */
  RTC_AlarmTypeDef sAlarm;
  sAlarm.AlarmTime.Hours = s_hour[best_idx];
  sAlarm.AlarmTime.Minutes = s_minute[best_idx];
  sAlarm.AlarmTime.Seconds = 0;
  sAlarm.AlarmTime.SubSeconds = 0;
  sAlarm.AlarmMask = RTC_ALARMMASK_DATEWEEKDAY;           /* 忽略日期/星期 */
  sAlarm.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_ALL; /* 忽略亚秒 */
  sAlarm.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE;
  sAlarm.AlarmDateWeekDay = 0x1;
  sAlarm.Alarm = RTC_ALARM_A;
  HAL_RTC_SetAlarm_IT(&hrtc, &sAlarm, RTC_FORMAT_BIN);
  s_scheduled_index = best_idx;
}

/* ========================================================================
 * Public API
 * ======================================================================== */

void AlarmService_Init(void) {
  for (int i = 0; i < ALARM_SERVICE_COUNT; i++) {
    uint32_t reg = HAL_RTCEx_BKUPRead(&hrtc, BKP_BASE_ALARM + i);
    s_enabled[i] = (reg & 0x80000000u) ? 1u : 0u;
    s_hour[i] = (uint8_t)((reg >> 8) & 0xFFu);
    s_minute[i] = (uint8_t)(reg & 0xFFu);
    /* 数据校验 (BKP 内容异常时归零) */
    if (s_hour[i] > 23)
      s_hour[i] = 0;
    if (s_minute[i] > 59)
      s_minute[i] = 0;
  }
  s_ringing = 0;
  s_ring_index = 0;
  alarm_reschedule();
}

uint8_t AlarmService_Get(uint8_t index, uint8_t *hour, uint8_t *minute,
                         uint8_t *enabled) {
  if (index >= ALARM_SERVICE_COUNT)
    return 0;
  if (hour)
    *hour = s_hour[index];
  if (minute)
    *minute = s_minute[index];
  if (enabled)
    *enabled = s_enabled[index];
  return 1;
}

void AlarmService_Set(uint8_t index, uint8_t hour, uint8_t minute,
                      uint8_t enabled) {
  if (index >= ALARM_SERVICE_COUNT)
    return;
  if (hour > 23)
    hour = 23;
  if (minute > 59)
    minute = 59;

  s_hour[index] = hour;
  s_minute[index] = minute;
  s_enabled[index] = enabled ? 1u : 0u;

  /* 掉电保存: (enabled<<31) | (hour<<8) | minute */
  uint32_t reg = (s_enabled[index] ? 0x80000000u : 0u) |
                 ((uint32_t)s_hour[index] << 8) | s_minute[index];
  HAL_RTCEx_BKUPWrite(&hrtc, BKP_BASE_ALARM + index, reg);

  alarm_reschedule();
}

uint8_t AlarmService_IsRinging(void) { return s_ringing; }
uint8_t AlarmService_GetRingingIndex(void) { return s_ring_index; }

void AlarmService_Dismiss(void) {
  s_ringing = 0;
  Buzzer_Stop();
  alarm_reschedule(); /* 排下一个闹钟 */
}

void AlarmService_Tick(void) {
  if (!s_ringing)
    return;

  /* 响铃期间强制亮屏 (刷新活动计时, 不熄屏) */
  MotionService_ForceScreenOn();

  /* 超时自动停止, 防止一直响 */
  if (MotionService_GetTime() - s_ring_start_ms >= RING_AUTO_STOP_MS) {
    AlarmService_Dismiss();
  }
}

/* ========================================================================
 * RTC 中断入口
 * ======================================================================== */

void AlarmService_OnAlarmA(void) {
  s_ringing = 1;
  s_ring_index = s_scheduled_index;
  s_ring_start_ms = MotionService_GetTime();
  Buzzer_StartPattern(); /* 启动响铃节奏 (中断里只写 GPIO, 安全) */
}

/* HAL 弱回调: RTC Alarm A 触发 (由 HAL_RTC_AlarmIRQHandler 调用) */
void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc) {
  (void)hrtc;
  AlarmService_OnAlarmA();
}
