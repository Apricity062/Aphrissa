/*
 * time_service.h
 *
 *  时间服务层: 封装 RTC 读写 + 串口校时帧解析。
 *  独立于 CubeMX 生成的 rtc.c (MX_RTC_Init 只管外设初始化),
 *  所有业务逻辑放在这个文件里, CubeMX 重新生成不会覆盖。
 *
 *  校时协议 (USART1, 115200 8N1, 一行一帧):
 *    $T,YYYY-MM-DD,HH:MM:SS,W\r\n
 *  例:  $T,2026-08-15,20:30:00,5\r\n   (W=星期, 1=周一 ... 7=周日)
 *  成功写入 RTC 后回发 "OK\r\n", 格式错误回发 "ERR\r\n"。
 */

#ifndef INC_TIME_SERVICE_H_
#define INC_TIME_SERVICE_H_

#include <stdint.h>

/* ---- Public API ---- */

/** 校时帧解析: 解析一行 "$T,..." 并写入 RTC。
 *  @param line  以 '\0' 结尾的一行文本 (可含末尾 \r\n, 会被忽略)
 *  @return 1 = 校时成功, 0 = 格式错误/写入失败
 */
uint8_t TimeService_ApplyTimeFrame(const char *line);

/** 读当前 RTC 时间 (24h, BCD 已转十进制)。 */
void TimeService_GetDateTime(uint16_t *year, uint8_t *month, uint8_t *day,
                             uint8_t *weekday, uint8_t *hour,
                             uint8_t *minute, uint8_t *second);

/** 判断 RTC 是否已初始化过 (首次上电 BKP 域无有效时间时返回 0)。 */
uint8_t TimeService_IsRtcValid(void);

/** 供 USART1 接收回调使用的单字节喂入: 内部做行缓冲, 收到 '\n' 自动解析。 */
void TimeService_UartRxByte(uint8_t byte);

/** 供主循环/Model 轮询: 若收到完整校时帧则解析并返回 1 (可选, 非中断驱动时用)。 */
uint8_t TimeService_PollPendingFrame(void);

/** 直接设置 RTC 日期时间 (星期自动计算) */
uint8_t TimeService_SetDateTime(uint16_t year, uint8_t month, uint8_t day,
                                uint8_t hour, uint8_t minute, uint8_t second);

/** 根据年月日计算星期 (蔡勒公式或查表法): 返回 1=周一..7=周日 */
uint8_t TimeService_CalcWeekday(uint16_t year, uint8_t month, uint8_t day);

/** 返回某年某月的天数 (28/29/30/31, 含闰年判断)。 */
uint8_t TimeService_DaysInMonth(uint16_t year, uint8_t month);

/* ========================================================================
 * 时制偏好 (12/24 小时制显示偏好, 存 BKP 寄存器掉电保持)
 * ======================================================================== */

#define TIME_HOUR_FORMAT_24 24u
#define TIME_HOUR_FORMAT_12 12u

/** 保存 12/24 小时制显示偏好 (掉电不丢, BKP 寄存器)。 */
void TimeService_SetHourFormat(uint8_t format);

/** 读取 12/24 小时制显示偏好 (默认 24)。 */
uint8_t TimeService_GetHourFormat(void);

#endif /* INC_TIME_SERVICE_H_ */
