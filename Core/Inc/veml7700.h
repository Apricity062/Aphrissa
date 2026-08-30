#ifndef VEML7700_H
#define VEML7700_H

#include "stm32u5xx_hal.h"

#define VEML7700_ADDR  (0x10 << 1)
#define VEML7700_ALS_CONFIG         0x00
#define VEML7700_ALS_DATA           0x04
#define VEML7700_WHITE_DATA         0x05
#define VEML7700_INTERRUPTSTATUS    0x06

/* ALS_GAIN field (bits[12:11]) */
#define VEML7700_GAIN_1   0x00
#define VEML7700_GAIN_2   0x01
#define VEML7700_GAIN_1_8 0x02
#define VEML7700_GAIN_1_4 0x03

/* ALS_IT field (bits[9:6], 4-bit) */
#define VEML7700_IT_25MS  0x0C
#define VEML7700_IT_50MS  0x08
#define VEML7700_IT_100MS 0x00
#define VEML7700_IT_200MS 0x01
#define VEML7700_IT_400MS 0x02
#define VEML7700_IT_800MS 0x03

typedef struct {
    I2C_HandleTypeDef *hi2c;
    uint8_t gain;
    uint8_t integration_time;
    HAL_StatusTypeDef last_status;
} veml7700_t;

uint8_t veml7700_init(veml7700_t *dev, I2C_HandleTypeDef *hi2c);
uint16_t veml7700_read_als(veml7700_t *dev);
uint16_t veml7700_read_white(veml7700_t *dev);
uint16_t veml7700_read_config(veml7700_t *dev);
float veml7700_compute_lux(veml7700_t *dev, uint16_t raw_als);

/** 串口调试输出: raw/white/lux/增益/积分时间/配置寄存器/状态/饱和提示。 */
void veml7700_debug_print(veml7700_t *dev, uint16_t raw_als, float lux);

#endif
