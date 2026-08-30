#ifndef _BUZZER_SERVICE_H_
#define _BUZZER_SERVICE_H_

#include <stdbool.h>

void Buzzer_Init(void);         // 初始化: PA2 置高(静音)
void Buzzer_Set(bool on);       // true=响(拉低), false=静音(拉高)
void Buzzer_Tick(void);         // 1ms 节奏控制 (TIM3 中断调用)
void Buzzer_StartPattern(void); // 响铃节奏: 如 响0.5s 停0.5s 循环
void Buzzer_Stop(void);         // 停止

#endif
