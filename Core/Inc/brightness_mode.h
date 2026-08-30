#ifndef BRIGHTNESS_MODE_H
#define BRIGHTNESS_MODE_H

#include <stdint.h>

#define BRIGHT_MODE_AUTO 1   // 自动（环境光）
#define BRIGHT_MODE_MANUAL 0 // 手动（滑条值）

void Brightness_SetMode(uint8_t mode);           // 供 UI 设置
uint8_t Brightness_GetMode(void);                // 供主循环查询
void Brightness_SetManualValue(uint8_t percent); // 滑条值
uint8_t Brightness_GetManualValue(void);

#endif /* BRIGHTNESS_MODE_H */