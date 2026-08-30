/*
 * stopwatch_service.h
 *
 *  毫秒级秒表: 基于 TIM3 硬件时基(MotionService_GetTime), 精确到 1ms。
 *  启动/停止那一刻读取的是硬件计数, 无软件累积误差。
 *  与屏幕无关, chronograph 界面只负责显示与按键。
 */

#ifndef INC_STOPWATCH_SERVICE_H_
#define INC_STOPWATCH_SERVICE_H_

#include <stdint.h>

/* ---- Public API ---- */
void     Stopwatch_Start(void);        /* 开始/继续计时 */
void     Stopwatch_Stop(void);         /* 暂停(冻结当前累计值) */
void     Stopwatch_Reset(void);        /* 清零 */
uint32_t Stopwatch_GetElapsedMs(void); /* 运行中=实时毫秒, 暂停=冻结值 */
uint8_t  Stopwatch_IsRunning(void);    /* 1=运行中 */

#endif /* INC_STOPWATCH_SERVICE_H_ */
