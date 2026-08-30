/*
 * time_service.c
 *
 *  时间服务层: RTC 读写封装 + 串口校时帧解析。
 *  使用 CubeMX 生成的 hrtc (rtc.c), 但所有业务逻辑在此文件内,
 *  不修改 CubeMX 生成文件, 重新生成代码不会丢失本文件内容。
 */

#include "time_service.h"
#include "rtc.h"            /* hrtc, MX_RTC_Init 原型 */
#include "main.h"
#include "stm32u5xx_hal.h"
#include <string.h>
#include <stdio.h>

/* ========================================================================
 * RTC 读写
 * ======================================================================== */

uint8_t TimeService_IsRtcValid(void)
{
  /* RTC 初始化状态位: INITS=1 表示日历已初始化 (HAL 标志查询) */
  return (__HAL_RTC_GET_FLAG(&hrtc, RTC_FLAG_INITS) != RESET) ? 1u : 0u;
}

void TimeService_GetDateTime(uint16_t *year, uint8_t *month, uint8_t *day,
                             uint8_t *weekday, uint8_t *hour,
                             uint8_t *minute, uint8_t *second)
{
  RTC_DateTypeDef d;
  RTC_TimeTypeDef t;

  /* 先等 RTC 寄存器同步 (RSF), 避免读到半个时钟沿的数据 */
  if (__HAL_RTC_GET_FLAG(&hrtc, RTC_FLAG_RSF) == RESET) {
    __HAL_RTC_CLEAR_FLAG(&hrtc, RTC_FLAG_RSF);
    while (__HAL_RTC_GET_FLAG(&hrtc, RTC_FLAG_RSF) == RESET) { }
  }

  if (HAL_RTC_GetTime(&hrtc, &t, RTC_FORMAT_BIN) != HAL_OK) {
    /* 读失败时给个明确的安全值, 避免 UI 显示垃圾 */
    if (hour)    *hour    = 0;
    if (minute)  *minute  = 0;
    if (second)  *second  = 0;
  } else {
    if (hour)    *hour    = t.Hours;
    if (minute)  *minute  = t.Minutes;
    if (second)  *second  = t.Seconds;
  }

  if (HAL_RTC_GetDate(&hrtc, &d, RTC_FORMAT_BIN) != HAL_OK) {
    if (year)     *year    = 2000;
    if (month)    *month   = 1;
    if (day)      *day     = 1;
    if (weekday)  *weekday = 1;
  } else {
    if (year)     *year    = 2000u + d.Year;   /* HAL 的 Year 是 0~99 */
    if (month)    *month   = d.Month;
    if (day)      *day     = d.Date;
    /* 星期: 用日期+蔡勒公式计算, 不信 RTC 的 WeekDay 寄存器
       (首次上电/复位时 WeekDay 是默认值, 可能与日期不同步,
        例如 2000-01-01 复位默认 WeekDay=星期一, 实际是星期六) */
    if (weekday)  *weekday = TimeService_CalcWeekday(
                                (uint16_t)(2000u + d.Year), d.Month, d.Date);
  }
}

/* ========================================================================
 * 校时帧解析
 * ======================================================================== */

/* 格式: $T,YYYY-MM-DD,HH:MM:SS,W\r\n */
static uint8_t parse_time_frame(const char *s,
                                uint16_t *year, uint8_t *month, uint8_t *day,
                                uint8_t *weekday, uint8_t *hour,
                                uint8_t *minute, uint8_t *second)
{
  unsigned y, mo, d, w, h, mi, se;

  if (s[0] != '$' || s[1] != 'T' || s[2] != ',')
    return 0;

  if (sscanf(s + 3, "%u-%u-%u,%u:%u:%u,%u",
             &y, &mo, &d, &h, &mi, &se, &w) != 7)
    return 0;

  /* 范围校验 */
  if (y < 2000 || y > 2099) return 0;
  if (mo < 1 || mo > 12)    return 0;
  if (d  < 1 || d  > 31)    return 0;
  if (h  > 23)              return 0;
  if (mi > 59)              return 0;
  if (se > 59)              return 0;
  if (w  < 1 || w  > 7)     return 0;

  *year    = (uint16_t)y;
  *month   = (uint8_t)mo;
  *day     = (uint8_t)d;
  *weekday = (uint8_t)w;
  *hour    = (uint8_t)h;
  *minute  = (uint8_t)mi;
  *second  = (uint8_t)se;
  return 1;
}

/* 星期 1~7 -> HAL 枚举 */
static uint32_t weekday_to_hal(uint8_t w)
{
  switch (w) {
    case 1: return RTC_WEEKDAY_MONDAY;
    case 2: return RTC_WEEKDAY_TUESDAY;
    case 3: return RTC_WEEKDAY_WEDNESDAY;
    case 4: return RTC_WEEKDAY_THURSDAY;
    case 5: return RTC_WEEKDAY_FRIDAY;
    case 6: return RTC_WEEKDAY_SATURDAY;
    default: return RTC_WEEKDAY_SUNDAY;
  }
}

uint8_t TimeService_ApplyTimeFrame(const char *line)
{
  uint16_t year;
  uint8_t month, day, weekday, hour, minute, second;

  if (!parse_time_frame(line, &year, &month, &day, &weekday,
                        &hour, &minute, &second))
    return 0;

  RTC_DateTypeDef sDate;
  RTC_TimeTypeDef sTime;

  sDate.Year    = (uint8_t)(year - 2000u);   /* HAL: 0~99 */
  sDate.Month   = month;
  sDate.Date    = day;
  sDate.WeekDay = weekday_to_hal(weekday);

  sTime.Hours   = hour;
  sTime.Minutes = minute;
  sTime.Seconds = second;
  sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  sTime.StoreOperation = RTC_STOREOPERATION_RESET;

  if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK)
    return 0;
  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
    return 0;
  return 1;
}

/* ========================================================================
 * 串口行缓冲 (供 USART1 接收中断喂字节)
 * ======================================================================== */

#define RX_BUF_SIZE 64

static volatile uint8_t  rx_buf[RX_BUF_SIZE];
static volatile uint16_t rx_len = 0;
static volatile uint8_t  rx_line_ready = 0;   /* 收到完整一行 (含 \n) */

void TimeService_UartRxByte(uint8_t byte)
{
  if (rx_line_ready)
    return;                    /* 上一行还没被消费, 丢弃新字节 */

  if (byte == '\n') {
    if (rx_len > 0) {
      /* 去掉可能的 \r */
      if (rx_buf[rx_len - 1] == '\r')
        rx_len--;
      rx_buf[rx_len] = '\0';
      rx_line_ready = 1;
      /* 注意: 不清零 rx_len, 主循环消费时用它做拷贝长度 */
    }
    return;
  }

  if (rx_len < (RX_BUF_SIZE - 1)) {
    rx_buf[rx_len++] = byte;
  } else {
    /* 超长行: 丢弃整行, 等待重新开始 */
    rx_len = 0;
  }
}

uint8_t TimeService_PollPendingFrame(void)
{
  char line[RX_BUF_SIZE];
  uint16_t len;
  uint16_t i;

  if (!rx_line_ready)
    return 0;

  /* 拷贝出缓冲 (中断可能在写), 再清标志 */
  len = rx_len;
  for (i = 0; i <= len; i++)    /* 含 '\0' */
    line[i] = (char)rx_buf[i];
  rx_len = 0;
  rx_line_ready = 0;

  if (TimeService_ApplyTimeFrame(line)) {
    printf("OK\r\n");
    return 1;
  }
  printf("ERR\r\n");
  return 0;
}

/* ========================================================================
 * 直接设置时间/日期 (滚轮设置用, 星期自动计算)
 * ======================================================================== */

/* 蔡勒公式 (Zeller's congruence): 返回 1=周一..7=周日 */
uint8_t TimeService_CalcWeekday(uint16_t year, uint8_t month, uint8_t day)
{
  int y = (int)year;
  int m = (int)month;
  int d = (int)day;

  /* 蔡勒公式要求 1月/2月 当作上一年的 13月/14月 */
  if (m < 3) {
    m += 12;
    y--;
  }

  int c = y / 100;              /* 世纪 */
  int yy = y % 100;             /* 年份后两位 */
  /* 公式: h = (d + 13*(m+1)/5 + yy + yy/4 + c/4 + 5*c) mod 7, h: 0=周六..6=周五 */
  int h = (d + 13 * (m + 1) / 5 + yy + yy / 4 + c / 4 + 5 * c) % 7;

  /* 转成 1=周一..7=周日: h=0->周六(6), h=1->周日(7), h=2->周一(1)... */
  if (h == 0) return 6;   /* 周六 */
  if (h == 1) return 7;   /* 周日 */
  return (uint8_t)(h - 1); /* 周一(2)->1, 周二(3)->2 ... 周五(6)->5 */
}

uint8_t TimeService_DaysInMonth(uint16_t year, uint8_t month)
{
  static const uint8_t days[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
  if (month < 1 || month > 12) return 31;

  if (month == 2) {
    /* 闰年: 能被4整除 且 (不能被100整除 或 能被400整除) */
    if ((year % 4 == 0) && (year % 100 != 0 || year % 400 == 0))
      return 29;
    return 28;
  }
  return days[month - 1];
}

uint8_t TimeService_SetDateTime(uint16_t year, uint8_t month, uint8_t day,
                                uint8_t hour, uint8_t minute, uint8_t second)
{
  /* 参数校验 */
  if (year < 2000 || year > 2099) return 0;
  if (month < 1 || month > 12)    return 0;
  if (day < 1 || day > 31)        return 0;
  if (hour > 23)                  return 0;
  if (minute > 59)                return 0;
  if (second > 59)                return 0;

  RTC_DateTypeDef sDate;
  RTC_TimeTypeDef sTime;

  sDate.Year    = (uint8_t)(year - 2000u);   /* HAL: 0~99 */
  sDate.Month   = month;
  sDate.Date    = day;
  sDate.WeekDay = weekday_to_hal(TimeService_CalcWeekday(year, month, day)); /* 日期自动算星期 */

  sTime.Hours   = hour;
  sTime.Minutes = minute;
  sTime.Seconds = second;
  sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  sTime.StoreOperation = RTC_STOREOPERATION_RESET;

  if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK)
    return 0;
  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
    return 0;
  return 1;
}

/* ========================================================================
 * 时制偏好 (12/24 小时制, BKP 寄存器掉电保存)
 * ======================================================================== */

#define BKP_REG_HOUR_FORMAT 1u   /* RTC 备份寄存器 #1: 0x0000_0012=12h, 0x0000_0018=24h */

void TimeService_SetHourFormat(uint8_t format)
{
  uint32_t val = (format == TIME_HOUR_FORMAT_12) ? TIME_HOUR_FORMAT_12
                                                 : TIME_HOUR_FORMAT_24;
  HAL_RTCEx_BKUPWrite(&hrtc, BKP_REG_HOUR_FORMAT, val);
}

uint8_t TimeService_GetHourFormat(void)
{
  uint32_t val = HAL_RTCEx_BKUPRead(&hrtc, BKP_REG_HOUR_FORMAT);
  return (val == TIME_HOUR_FORMAT_12) ? TIME_HOUR_FORMAT_12 : TIME_HOUR_FORMAT_24;
}
