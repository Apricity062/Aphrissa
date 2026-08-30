// 这个文件存放着model，负责数据的处理和存储，并通知presenter数据的变化，受害者腐殖质
// modelListener是一个接口类，定义了presenter需要实现的方法，当model的数据发生变化时，model会使用modelListener调用presenter的相应方法来通知view更新UI，“听者从腐殖质带来信息”
#include <cstdint>
#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>
#include <stdio.h>

#ifndef SIMULATOR
extern "C" {
#include "alarm_service.h"
#include "backlight_service.h"
#include "brightness_mode.h"
#include "max30102_service.h"
#include "motion_service.h"
#include "stopwatch_service.h"
#include "time_service.h"
}
#endif

Model::Model() : modelListener(0) {}

void Model::tick() {
#ifndef SIMULATOR
  screenOn = MotionService_IsScreenOn(); /* 从目标板同步屏幕状态 */

  /* 真实时间源: RTC (LSE 32.768kHz 晶振走时), 每秒读取一次 */
  uint16_t y;
  uint8_t mo, d, w, h, mi, s;
  TimeService_GetDateTime(&y, &mo, &d, &w, &h, &mi, &s);

  /* 只有秒发生变化时才刷新 UI, 避免每帧都触发重绘 */
  if (h != currentHour || mi != currentMinute || s != currentSecond) {
    currentHour = h;
    currentMinute = mi;
    currentSecond = s;
    currentYear = (uint8_t)(y - 2000u); /* UI 显示 2000+year */
    currentMonth = mo;
    currentDay = d;
    currentWeekday = w;

    int32_t hr = Max30102_GetHeartRate();
    int32_t spo2 = Max30102_GetSpO2();

    if (modelListener) {
      modelListener->updateTime(currentHour, currentMinute, currentSecond);
      modelListener->updateDate(currentYear, currentMonth, currentDay,
                                currentWeekday);
      modelListener->updateHeartRate(hr, spo2);
    }
  }

  /* 秒表: 每 tick 轮询一次裸机服务, 值/状态变了才推送 (避免每帧刷 UI) */
  {
    uint32_t ms = Stopwatch_GetElapsedMs();
    bool running = Stopwatch_IsRunning() != 0;
    if (ms != lastStopwatchMs || running != lastStopwatchRunning) {
      lastStopwatchMs = ms;
      lastStopwatchRunning = running;
      currentStopwatchMs = ms;
      stopwatchRunning = running;
      if (modelListener) {
        modelListener->updateStopwatch(ms, running);
      }
    }
  }

  /* 亮度: 同步裸机状态到 Model 缓存 (供 View 进界面时拉取) */
  brightnessMode = Brightness_GetMode();
  brightnessValue = Brightness_GetManualValue();

  /* 闹钟: 轮询响铃状态, 变化时推送 (开始响/关闭响) */
  {
    uint8_t ring = AlarmService_IsRinging();
    uint8_t idx  = AlarmService_GetRingingIndex();
    if (ring != alarmRinging || (ring && idx != alarmRingingIndex)) {
      alarmRinging = ring;
      alarmRingingIndex = idx;
      if (modelListener) {
        modelListener->updateAlarmRing(ring, idx);
      }
    }
  }
#endif
}

bool Model::isScreenOn() const { return screenOn; }

/* ========================================================================
 * 秒表 (MVP: View 经 presenter 调用这里, 不直接碰裸机服务)
 * ======================================================================== */
void Model::stopwatchStart() {
#ifndef SIMULATOR
  Stopwatch_Start();
  MotionService_SetKeepAwake(1); /* 秒表运行中: 禁止自动熄屏 */
#endif
}

void Model::stopwatchStop() {
#ifndef SIMULATOR
  Stopwatch_Stop();
  MotionService_SetKeepAwake(0);
#endif
}

void Model::stopwatchReset() {
#ifndef SIMULATOR
  Stopwatch_Reset();
  MotionService_SetKeepAwake(0);
#endif
}

void Model::releaseKeepAwake() {
#ifndef SIMULATOR
  MotionService_SetKeepAwake(0);
#endif
}

/* ========================================================================
 * 亮度 (MVP: View 经 presenter 调用这里)
 * ======================================================================== */
void Model::setBrightnessMode(uint8_t mode) {
#ifndef SIMULATOR
  Brightness_SetMode(mode);
  brightnessMode = mode;
#endif
}

void Model::setBrightnessValue(uint8_t value) {
#ifndef SIMULATOR
  Brightness_SetManualValue(value);
  brightnessValue = value;
#endif
}

/* ========================================================================
 * 手电筒 (lightning 屏幕: 开=拉满, 关=恢复原亮度+原模式)
 * ======================================================================== */
void Model::flashlightOn() {
#ifndef SIMULATOR
  savedBrightness = brightnessValue;      /* 记录当前亮度 */
  savedBrightnessMode = brightnessMode;   /* 记录当前模式 (自动/手动) */
  Brightness_SetMode(BRIGHT_MODE_MANUAL); /* 手动模式, 让主循环应用 */
  Brightness_SetManualValue(100);         /* 拉满 */
  brightnessMode = BRIGHT_MODE_MANUAL;
  brightnessValue = 100;
#endif
}

void Model::flashlightOff() {
#ifndef SIMULATOR
  Brightness_SetMode(savedBrightnessMode);    /* 恢复原模式 (自动/手动) */
  Brightness_SetManualValue(savedBrightness); /* 恢复原亮度 */
  brightnessMode = savedBrightnessMode;
  brightnessValue = savedBrightness;
#endif
}



/*时间设置相关↓*/
void Model::setDateTime(uint16_t y, uint8_t mo, uint8_t d, uint8_t h,
                        uint8_t mi) { // 设置时间
#ifndef SIMULATOR
  TimeService_SetDateTime(y, mo, d, h, mi, 0); /* 秒固定 0 */
#endif
}

void Model::setHourFormat(uint8_t fmt) {
#ifndef SIMULATOR
  TimeService_SetHourFormat(fmt);
#endif
} // 存偏好

uint8_t Model::getHourFormat() const {
#ifndef SIMULATOR
  return TimeService_GetHourFormat();
#else
  return 24;
#endif
}

/* ========================================================================
 * 闹钟 (MVP: View 经 presenter 调用这里, 不直接碰裸机服务)
 * ======================================================================== */
uint8_t Model::getAlarm(uint8_t index, uint8_t *hour, uint8_t *minute,
                        uint8_t *enabled) {
#ifndef SIMULATOR
  return AlarmService_Get(index, hour, minute, enabled);
#else
  return 0;
#endif
}

void Model::setAlarm(uint8_t index, uint8_t hour, uint8_t minute,
                     uint8_t enabled) {
#ifndef SIMULATOR
  AlarmService_Set(index, hour, minute, enabled);
#endif
}

void Model::dismissAlarm() {
#ifndef SIMULATOR
  AlarmService_Dismiss();
#endif
}