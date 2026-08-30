/*
 * brightness_mode.c
 *
 *  亮度模式跨层状态: TouchGFX UI 与裸机主循环共享的"自动/手动"开关 + 手动亮度值。
 *  定义在独立小文件, 供 settingsView (UI) 与 main.c (裸机) 共同读写。
 */

#include "brightness_mode.h"
#include <stdint.h>

/* ---- Internal state ---- */
static uint8_t m_mode = BRIGHT_MODE_AUTO;   /* 默认自动调光 */
static uint8_t m_manual_value = 60;         /* 手动亮度默认 60% */

void Brightness_SetMode(uint8_t mode)
{
  m_mode = (mode == BRIGHT_MODE_MANUAL) ? BRIGHT_MODE_MANUAL : BRIGHT_MODE_AUTO;
}

uint8_t Brightness_GetMode(void)
{
  return m_mode;
}

void Brightness_SetManualValue(uint8_t percent)
{
  if (percent > 100u)
    percent = 100u;
  m_manual_value = percent;
}

uint8_t Brightness_GetManualValue(void)
{
  return m_manual_value;
}
