/*
 * alarm_service.h
 *
 *  闹钟服务层: 8 个闹钟(时/分/启停) + BKP 掉电保存 + RTC Alarm A 软件接力。
 *  独立于 CubeMX 生成的 rtc.c, 重新生成不会覆盖。
 *
 *  硬件只有一个 RTC Alarm A, 8 个闹钟用"软件接力"调度:
 *    每次修改闹钟或响铃触发后, 扫描所有启用闹钟, 把"下一个最近触发时刻"
 *    写进 RTC Alarm A (掩码日期/星期 → 每天重复)。
 *    到点触发中断 → 置响铃标志 + 启动蜂鸣器节奏 + 重调度下一个。
 */

#ifndef INC_ALARM_SERVICE_H_
#define INC_ALARM_SERVICE_H_

#include <stdint.h>

#define ALARM_SERVICE_COUNT 8u   /* 闹钟数量, 与 UI 层 ALARM_COUNT 保持一致 */

/* ---- Public API ---- */

/** 初始化: 从 BKP 寄存器恢复 8 个闹钟数据 + 重调度最近的闹钟。
 *  main 初始化阶段调用一次 (RTC 初始化之后)。 */
void AlarmService_Init(void);

/** 读单个闹钟 (index 0..7)。返回 1=成功, 0=索引越界。 */
uint8_t AlarmService_Get(uint8_t index, uint8_t *hour, uint8_t *minute,
                         uint8_t *enabled);

/** 写单个闹钟 (时/分/启停), 自动存入 BKP + 重调度 Alarm A。 */
void AlarmService_Set(uint8_t index, uint8_t hour, uint8_t minute,
                      uint8_t enabled);

/** 是否正在响铃 (Model tick 轮询用)。 */
uint8_t AlarmService_IsRinging(void);

/** 当前正在响铃的闹钟索引 (仅 IsRinging 时有效)。 */
uint8_t AlarmService_GetRingingIndex(void);

/** 用户关闭响铃: 停蜂鸣器 + 清响铃标志。 */
void AlarmService_Dismiss(void);

/** 主循环周期调用: 响铃超时自动停止 / 响铃期间强制亮屏。 */
void AlarmService_Tick(void);

/** RTC Alarm A 中断入口 (由 HAL_RTC_AlarmAEventCallback 调用, 中断上下文)。 */
void AlarmService_OnAlarmA(void);

#endif /* INC_ALARM_SERVICE_H_ */
