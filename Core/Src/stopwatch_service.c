/*
 * stopwatch_service.c
 *
 *  状态机: 未计时 → 运行中 → 已暂停 →(再次 Start)运行中 →(Reset)未计时。
 *  时间源 = TIM3 硬件时基(hw_ms, 1ms/格), 无符号减法天然防 49.7 天回绕。
 */
#include "stopwatch_service.h"
#include "motion_service.h" /* MotionService_GetTime() 返回 TIM3 hw_ms */

static uint32_t m_start_tick = 0; /* 本次运行段的起始时刻 */
static uint32_t m_accum_ms   = 0; /* 之前已暂停段累计的毫秒 */
static uint8_t  m_running    = 0;

void Stopwatch_Start(void)
{
  m_start_tick = MotionService_GetTime();
  m_running = 1;
}

void Stopwatch_Stop(void)
{
  if (m_running) {
    m_accum_ms += MotionService_GetTime() - m_start_tick; /* 精确到 1ms 冻结 */
    m_running = 0;
  }
}

void Stopwatch_Reset(void)
{
  m_accum_ms = 0;
  m_start_tick = 0;
  m_running = 0;
}

uint32_t Stopwatch_GetElapsedMs(void)
{
  uint32_t e = m_accum_ms;
  if (m_running) {
    e += MotionService_GetTime() - m_start_tick;
  }
  return e;
}

uint8_t Stopwatch_IsRunning(void) { return m_running; }
