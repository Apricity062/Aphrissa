/*
 * max30102_service.h
 *
 *  心率/血氧服务: 封装 MAX30102 HAL 驱动 + Maxim HR/SpO2 算法。
 *  主循环周期调用 Update(), 累积样本后自动算出心率/血氧供 UI 读取。
 */

#ifndef INC_MAX30102_SERVICE_H_
#define INC_MAX30102_SERVICE_H_

#include <stdint.h>

/* ---- Public API ---- */
void    Max30102_Init(void);        /* 配置传感器(I2C2, 地址0x57) */
void    Max30102_Start(void);       /* 初始化并启动测量(进心率屏时调) */
void    Max30102_Stop(void);        /* 关闭测量+传感器休眠(离开心率屏时调, 省电) */
void    Max30102_Update(void);      /* 主循环每帧调用: 读FIFO+累积+算算法 */

int32_t Max30102_GetHeartRate(void); /* bpm, -999 = 无效 */
int32_t Max30102_GetSpO2(void);      /* % , -999 = 无效 */
uint8_t Max30102_IsHeartRateValid(void);
uint8_t Max30102_IsSpO2Valid(void);

#endif /* INC_MAX30102_SERVICE_H_ */
