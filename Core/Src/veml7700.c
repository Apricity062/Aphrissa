#include "veml7700.h"
#include <stdio.h>

/* I2C 有限超时(ms)。与 MPU6050 的 i2c_timeout=100 一致:
   传感器不应答时必须尽快返回, 不能 HAL_MAX_DELAY 无限阻塞主循环。 */
#define VEML7700_I2C_TIMEOUT 100u

uint8_t veml7700_init(veml7700_t *dev, I2C_HandleTypeDef *hi2c) {
    dev->hi2c = hi2c;
    dev->gain = VEML7700_GAIN_1_8;
    dev->integration_time = VEML7700_IT_100MS;
    dev->last_status = HAL_OK;

    /* ALS_CONF: SD=bit0, INT_EN=bit1, PERS=bits[5:4], ALS_IT=bits[9:6], ALS_GAIN=bits[12:11]
       SD=0 -> power on. */
    uint16_t config = (dev->gain << 11) | (dev->integration_time << 6);
    uint8_t data[2] = { config & 0xFF, config >> 8 };

    dev->last_status = HAL_I2C_Mem_Write(dev->hi2c, VEML7700_ADDR, VEML7700_ALS_CONFIG,
                                         I2C_MEMADD_SIZE_8BIT, data, 2, VEML7700_I2C_TIMEOUT);
    if (dev->last_status != HAL_OK) {
        return 0;
    }

    HAL_Delay(5);
    return 1;
}

uint16_t veml7700_read_als(veml7700_t *dev) {
    uint8_t data[2];

    dev->last_status = HAL_I2C_Mem_Read(dev->hi2c, VEML7700_ADDR, VEML7700_ALS_DATA,
                                        I2C_MEMADD_SIZE_8BIT, data, 2, VEML7700_I2C_TIMEOUT);
    if (dev->last_status != HAL_OK) {
        return 0xFFFF; /* caller must check last_status (0xFFFF is also a valid saturated reading) */
    }
    return (uint16_t)((data[1] << 8) | data[0]);
}

uint16_t veml7700_read_white(veml7700_t *dev) {
    uint8_t data[2];

    dev->last_status = HAL_I2C_Mem_Read(dev->hi2c, VEML7700_ADDR, VEML7700_WHITE_DATA,
                                        I2C_MEMADD_SIZE_8BIT, data, 2, VEML7700_I2C_TIMEOUT);
    if (dev->last_status != HAL_OK) {
        return 0xFFFF;
    }
    return (uint16_t)((data[1] << 8) | data[0]);
}

uint16_t veml7700_read_config(veml7700_t *dev) {
    uint8_t data[2];

    dev->last_status = HAL_I2C_Mem_Read(dev->hi2c, VEML7700_ADDR, VEML7700_ALS_CONFIG,
                                        I2C_MEMADD_SIZE_8BIT, data, 2, VEML7700_I2C_TIMEOUT);
    if (dev->last_status != HAL_OK) {
        return 0xFFFF;
    }
    return (uint16_t)((data[1] << 8) | data[0]);
}

static float veml7700_gain_value(uint8_t gain) {
    switch (gain) {
    case VEML7700_GAIN_1:   return 1.0f;
    case VEML7700_GAIN_2:   return 2.0f;
    case VEML7700_GAIN_1_4: return 0.25f;
    case VEML7700_GAIN_1_8:
    default:                return 0.125f;
    }
}

static uint32_t veml7700_it_value(uint8_t it) {
    switch (it) {
    case VEML7700_IT_25MS:  return 25;
    case VEML7700_IT_50MS:  return 50;
    case VEML7700_IT_200MS: return 200;
    case VEML7700_IT_400MS: return 400;
    case VEML7700_IT_800MS: return 800;
    case VEML7700_IT_100MS:
    default:                return 100;
    }
}

float veml7700_compute_lux(veml7700_t *dev, uint16_t raw_als) {
    float gain_val = veml7700_gain_value(dev->gain);
    float it_ms = (float)veml7700_it_value(dev->integration_time);

    /* Resolution = MAX_RES(0.0036 @ 800ms/gain2) * (IT_MAX/IT) * (GAIN_MAX/gain)
       e.g. gain 1/8 + 100ms -> 0.4608 lux/LSB */
    float resolution = 0.0036f * (800.0f / it_ms) * (2.0f / gain_val);

    float lux = resolution * raw_als;

    /* Non-linear correction (Vishay app note 84323, for gain=1/8 & IT=100ms) */
    lux = (((6.0135e-13f * lux - 9.3924e-9f) * lux + 8.1488e-5f) * lux + 1.0023f) * lux;
    return lux;
}

/* ---- 串口调试输出 ---- */

static const char *veml7700_gain_str(uint8_t gain) {
    switch (gain) {
    case VEML7700_GAIN_1:   return "1x";
    case VEML7700_GAIN_2:   return "2x";
    case VEML7700_GAIN_1_8: return "1/8";
    case VEML7700_GAIN_1_4: return "1/4";
    default:                return "?";
    }
}

static const char *veml7700_it_str(uint8_t it) {
    switch (it) {
    case VEML7700_IT_25MS:  return "25ms";
    case VEML7700_IT_50MS:  return "50ms";
    case VEML7700_IT_100MS: return "100ms";
    case VEML7700_IT_200MS: return "200ms";
    case VEML7700_IT_400MS: return "400ms";
    case VEML7700_IT_800MS: return "800ms";
    default:                return "?";
    }
}

void veml7700_debug_print(veml7700_t *dev, uint16_t raw_als, float lux) {
    uint16_t white = veml7700_read_white(dev);
    HAL_StatusTypeDef white_st = dev->last_status;
    uint16_t cfg = veml7700_read_config(dev);
    HAL_StatusTypeDef cfg_st = dev->last_status;

    int lux_int = (int)lux;
    int lux_frac = (int)((lux - (float)lux_int) * 1000.0f);
    if (lux_frac < 0)
        lux_frac = -lux_frac;

    printf("[VEML] raw=%u(0x%04X) white=%u lux=%d.%03d%s\r\n",
           raw_als, (unsigned)raw_als, white, lux_int, lux_frac,
           (raw_als >= 0xFFF0u) ? " [SATURATED]" : "");
    printf("[VEML] gain=%s IT=%s als_cfg=0x%04X st:white=%d cfg=%d\r\n",
           veml7700_gain_str(dev->gain), veml7700_it_str(dev->integration_time),
           (unsigned)cfg, (int)white_st, (int)cfg_st);
}
