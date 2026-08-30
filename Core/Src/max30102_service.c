/*
 * max30102_service.c
 *
 *  心率/血氧服务。
 *  周期读 MAX30102 FIFO → 累积 BUFFER_SIZE(500) 个样本 → 跑 Maxim SpO2/HR 算法。
 *  采样率 100Hz, 500 样本 ≈ 5 秒出一个心率/血氧值。
 */
#include "max30102_service.h"
#include "max30102_for_stm32_hal.h"
#include "i2c.h"                  /* hi2c2 */
#include <stdio.h>

extern volatile uint32_t hw_ms;   /* TIM3 硬件时基 */

#define FS           100          /* 有效采样率 Hz */
#define BUFFER_SIZE  (FS * 5)     /* 算法窗口: 500 样本 */
#define READ_INTERVAL_MS 20       /* 每 20ms 读一次 FIFO */
#define SIGNAL_THRESHOLD 30000    /* 无手指时 ir 直流 ~1千, 有手指 ~19万 */
#define HR_MIN 40                 /* 心率有效范围 bpm */
#define HR_MAX 200
#define HR_SMOOTH_N 4             /* 心率平滑系数: 越大越平滑但反应越慢 */

/* ---- 内部状态 ---- */
static max30102_t m_max;
static uint32_t m_ir_buf[BUFFER_SIZE];
static uint32_t m_red_buf[BUFFER_SIZE];
static uint16_t m_buf_idx = 0;
static uint32_t m_last_ir = 0;  /* 最近一次读到的原始 IR 值 */
static uint32_t m_last_red = 0; /* 最近一次读到的原始 Red 值 */
static uint8_t  m_enabled = 0;  /* 是否在测量(进心率屏才开启) */
static int32_t  m_hr_smooth = -999; /* 心率平滑值, 抑制突变 */

static int32_t m_hr = -999;
static int8_t  m_hr_valid = 0;
static int32_t m_spo2 = -999;
static int8_t  m_spo2_valid = 0;

/* ========================================================================
 * Maxim Heart Rate / SpO2 algorithm (来自 Maxim MAX30102 参考实现)
 * ======================================================================== */
#define MA4_SIZE  4 /* DO NOT CHANGE */
#define HAMMING_SIZE 5 /* DO NOT CHANGE */
#define min(x, y) ((x) < (y) ? (x) : (y))

const uint16_t auw_hamm[31] = {41, 276, 512, 276, 41}; /* hamming(5) */

const uint8_t uch_spo2_table[184] = {
    95, 95, 95, 96, 96, 96, 97, 97, 97, 97, 97, 98, 98, 98, 98, 98, 99, 99, 99, 99,
    99, 99, 99, 99, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100,
    100, 100, 100, 100, 99, 99, 99, 99, 99, 99, 99, 99, 98, 98, 98, 98, 98, 98, 97, 97,
    97, 97, 96, 96, 96, 96, 95, 95, 95, 94, 94, 94, 93, 93, 93, 92, 92, 92, 91, 91,
    90, 90, 89, 89, 89, 88, 88, 87, 87, 86, 86, 85, 85, 84, 84, 83, 82, 82, 81, 81,
    80, 80, 79, 78, 78, 77, 76, 76, 75, 74, 74, 73, 72, 72, 71, 70, 69, 69, 68, 67,
    66, 66, 65, 64, 63, 62, 62, 61, 60, 59, 58, 57, 56, 56, 55, 54, 53, 52, 51, 50,
    49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 31, 30, 29,
    28, 27, 26, 25, 23, 22, 21, 20, 19, 17, 16, 15, 14, 12, 11, 10, 9, 7, 6, 5,
    3, 2, 1};

static int32_t an_dx[BUFFER_SIZE - MA4_SIZE]; /* delta */
static int32_t an_x[BUFFER_SIZE];             /* ir */
static int32_t an_y[BUFFER_SIZE];             /* red */

/* 前向声明(避免隐式声明) */
void maxim_find_peaks(int32_t *pn_locs, int32_t *pn_npks, int32_t *pn_x, int32_t n_size,
                      int32_t n_min_height, int32_t n_min_distance, int32_t n_max_num);
void maxim_peaks_above_min_height(int32_t *pn_locs, int32_t *pn_npks, int32_t *pn_x, int32_t n_size, int32_t n_min_height);
void maxim_remove_close_peaks(int32_t *pn_locs, int32_t *pn_npks, int32_t *pn_x, int32_t n_min_distance);
void maxim_sort_ascend(int32_t *pn_x, int32_t n_size);
void maxim_sort_indices_descend(int32_t *pn_x, int32_t *pn_indx, int32_t n_size);

void maxim_heart_rate_and_oxygen_saturation(uint32_t *pun_ir_buffer, int32_t n_ir_buffer_length,
                                            uint32_t *pun_red_buffer, int32_t *pn_spo2, int8_t *pch_spo2_valid,
                                            int32_t *pn_heart_rate, int8_t *pch_hr_valid)
{
    uint32_t un_ir_mean, un_only_once;
    int32_t k, n_i_ratio_count;
    int32_t i, s, m, n_exact_ir_valley_locs_count, n_middle_idx;
    int32_t n_th1, n_npks, n_c_min;
    int32_t an_ir_valley_locs[15];
    int32_t an_exact_ir_valley_locs[15];
    int32_t an_dx_peak_locs[15];
    int32_t n_peak_interval_sum;

    int32_t n_y_ac, n_x_ac;
    int32_t n_spo2_calc;
    int32_t n_y_dc_max, n_x_dc_max;
    int32_t n_y_dc_max_idx, n_x_dc_max_idx;
    int32_t an_ratio[5], n_ratio_average;
    int32_t n_nume, n_denom;

    /* remove DC of ir signal */
    un_ir_mean = 0;
    for (k = 0; k < n_ir_buffer_length; k++)
        un_ir_mean += pun_ir_buffer[k];
    un_ir_mean = un_ir_mean / n_ir_buffer_length;
    for (k = 0; k < n_ir_buffer_length; k++)
        an_x[k] = pun_ir_buffer[k] - un_ir_mean;

    /* 4 pt Moving Average */
    for (k = 0; k < BUFFER_SIZE - MA4_SIZE; k++) {
        n_denom = (an_x[k] + an_x[k + 1] + an_x[k + 2] + an_x[k + 3]);
        an_x[k] = n_denom / (int32_t)4;
    }

    /* get difference of smoothed IR signal */
    for (k = 0; k < BUFFER_SIZE - MA4_SIZE - 1; k++)
        an_dx[k] = (an_x[k + 1] - an_x[k]);

    /* 2-pt Moving Average to an_dx */
    for (k = 0; k < BUFFER_SIZE - MA4_SIZE - 2; k++) {
        an_dx[k] = (an_dx[k] + an_dx[k + 1]) / 2;
    }

    /* hamming window; flip waveform to detect valley with peak detector */
    for (i = 0; i < BUFFER_SIZE - HAMMING_SIZE - MA4_SIZE - 2; i++) {
        s = 0;
        for (k = i; k < i + HAMMING_SIZE; k++) {
            s -= an_dx[k] * auw_hamm[k - i];
        }
        an_dx[i] = s / (int32_t)1146; /* divide by sum of auw_hamm */
    }

    n_th1 = 0; /* threshold calculation */
    for (k = 0; k < BUFFER_SIZE - HAMMING_SIZE; k++) {
        n_th1 += ((an_dx[k] > 0) ? an_dx[k] : ((int32_t)0 - an_dx[k]));
    }
    n_th1 = n_th1 / (BUFFER_SIZE - HAMMING_SIZE);

    maxim_find_peaks(an_dx_peak_locs, &n_npks, an_dx, BUFFER_SIZE - HAMMING_SIZE, n_th1, 30, 5); /* 峰间距≥30样本(300ms), 过滤重搏切迹/噪声峰 */

    n_peak_interval_sum = 0;
    if (n_npks >= 2) {
        for (k = 1; k < n_npks; k++)
            n_peak_interval_sum += (an_dx_peak_locs[k] - an_dx_peak_locs[k - 1]);
        n_peak_interval_sum = n_peak_interval_sum / (n_npks - 1);
        *pn_heart_rate = (int32_t)(6000 / n_peak_interval_sum); /* beats per minutes */
        *pch_hr_valid = 1;
    } else {
        *pn_heart_rate = -999;
        *pch_hr_valid = 0;
    }

    for (k = 0; k < n_npks; k++)
        an_ir_valley_locs[k] = an_dx_peak_locs[k] + HAMMING_SIZE / 2;

    /* raw value: RED(=y) and IR(=X) */
    for (k = 0; k < n_ir_buffer_length; k++) {
        an_x[k] = pun_ir_buffer[k];
        an_y[k] = pun_red_buffer[k];
    }

    /* find precise min near an_ir_valley_locs */
    n_exact_ir_valley_locs_count = 0;
    for (k = 0; k < n_npks; k++) {
        un_only_once = 1;
        m = an_ir_valley_locs[k];
        n_c_min = 16777216; /* 2^24 */
        if (m + 5 < BUFFER_SIZE - HAMMING_SIZE && m - 5 > 0) {
            for (i = m - 5; i < m + 5; i++)
                if (an_x[i] < n_c_min) {
                    if (un_only_once > 0) {
                        un_only_once = 0;
                    }
                    n_c_min = an_x[i];
                    an_exact_ir_valley_locs[k] = i;
                }
            if (un_only_once == 0)
                n_exact_ir_valley_locs_count++;
        }
    }
    if (n_exact_ir_valley_locs_count < 2) {
        *pn_spo2 = -999;
        *pch_spo2_valid = 0;
        return;
    }

    /* 4 pt MA */
    for (k = 0; k < BUFFER_SIZE - MA4_SIZE; k++) {
        an_x[k] = (an_x[k] + an_x[k + 1] + an_x[k + 2] + an_x[k + 3]) / (int32_t)4;
        an_y[k] = (an_y[k] + an_y[k + 1] + an_y[k + 2] + an_y[k + 3]) / (int32_t)4;
    }

    for (k = 0; k < n_exact_ir_valley_locs_count; k++) {
        if (an_exact_ir_valley_locs[k] > BUFFER_SIZE) {
            *pn_spo2 = -999;
            *pch_spo2_valid = 0;
            return;
        }
    }

    n_ratio_average = 0;
    n_i_ratio_count = 0;
    for (k = 0; k < 5; k++)
        an_ratio[k] = 0;

    for (k = 0; k < n_exact_ir_valley_locs_count - 1; k++) {
        n_y_dc_max = -16777216;
        n_x_dc_max = -16777216;
        if (an_exact_ir_valley_locs[k + 1] - an_exact_ir_valley_locs[k] > 10) {
            for (i = an_exact_ir_valley_locs[k]; i < an_exact_ir_valley_locs[k + 1]; i++) {
                if (an_x[i] > n_x_dc_max) {
                    n_x_dc_max = an_x[i];
                    n_x_dc_max_idx = i;
                }
                if (an_y[i] > n_y_dc_max) {
                    n_y_dc_max = an_y[i];
                    n_y_dc_max_idx = i;
                }
            }
            n_y_ac = (an_y[an_exact_ir_valley_locs[k + 1]] - an_y[an_exact_ir_valley_locs[k]]) * (n_y_dc_max_idx - an_exact_ir_valley_locs[k]);
            n_y_ac = an_y[an_exact_ir_valley_locs[k]] + n_y_ac / (an_exact_ir_valley_locs[k + 1] - an_exact_ir_valley_locs[k]);
            n_y_ac = an_y[n_y_dc_max_idx] - n_y_ac;

            n_x_ac = (an_x[an_exact_ir_valley_locs[k + 1]] - an_x[an_exact_ir_valley_locs[k]]) * (n_x_dc_max_idx - an_exact_ir_valley_locs[k]);
            n_x_ac = an_x[an_exact_ir_valley_locs[k]] + n_x_ac / (an_exact_ir_valley_locs[k + 1] - an_exact_ir_valley_locs[k]);
            n_x_ac = an_x[n_y_dc_max_idx] - n_x_ac;

            n_nume = (n_y_ac * n_x_dc_max) >> 7;
            n_denom = (n_x_ac * n_y_dc_max) >> 7;
            if (n_denom > 0 && n_i_ratio_count < 5 && n_nume != 0) {
                an_ratio[n_i_ratio_count] = (n_nume * 20) / n_denom;
                n_i_ratio_count++;
            }
        }
    }

    maxim_sort_ascend(an_ratio, n_i_ratio_count);
    n_middle_idx = n_i_ratio_count / 2;

    if (n_middle_idx > 1)
        n_ratio_average = (an_ratio[n_middle_idx - 1] + an_ratio[n_middle_idx]) / 2;
    else
        n_ratio_average = an_ratio[n_middle_idx];

    if (n_ratio_average > 2 && n_ratio_average < 184) {
        n_spo2_calc = uch_spo2_table[n_ratio_average];
        *pn_spo2 = n_spo2_calc;
        *pch_spo2_valid = 1;
    } else {
        *pn_spo2 = -999;
        *pch_spo2_valid = 0;
    }
}

void maxim_find_peaks(int32_t *pn_locs, int32_t *pn_npks, int32_t *pn_x, int32_t n_size,
                      int32_t n_min_height, int32_t n_min_distance, int32_t n_max_num)
{
    maxim_peaks_above_min_height(pn_locs, pn_npks, pn_x, n_size, n_min_height);
    maxim_remove_close_peaks(pn_locs, pn_npks, pn_x, n_min_distance);
    *pn_npks = min(*pn_npks, n_max_num);
}

void maxim_peaks_above_min_height(int32_t *pn_locs, int32_t *pn_npks, int32_t *pn_x, int32_t n_size, int32_t n_min_height)
{
    int32_t i = 1, n_width;
    *pn_npks = 0;

    while (i < n_size - 1) {
        if (pn_x[i] > n_min_height && pn_x[i] > pn_x[i - 1]) {
            n_width = 1;
            while (i + n_width < n_size && pn_x[i] == pn_x[i + n_width])
                n_width++;
            if (pn_x[i] > pn_x[i + n_width] && (*pn_npks) < 15) {
                pn_locs[(*pn_npks)++] = i;
                i += n_width + 1;
            } else
                i += n_width;
        } else
            i++;
    }
}

void maxim_remove_close_peaks(int32_t *pn_locs, int32_t *pn_npks, int32_t *pn_x, int32_t n_min_distance)
{
    int32_t i, j, n_old_npks, n_dist;

    maxim_sort_indices_descend(pn_x, pn_locs, *pn_npks);

    for (i = -1; i < *pn_npks; i++) {
        n_old_npks = *pn_npks;
        *pn_npks = i + 1;
        for (j = i + 1; j < n_old_npks; j++) {
            n_dist = pn_locs[j] - (i == -1 ? -1 : pn_locs[i]);
            if (n_dist > n_min_distance || n_dist < -n_min_distance)
                pn_locs[(*pn_npks)++] = pn_locs[j];
        }
    }

    maxim_sort_ascend(pn_locs, *pn_npks);
}

void maxim_sort_ascend(int32_t *pn_x, int32_t n_size)
{
    int32_t i, j, n_temp;
    for (i = 1; i < n_size; i++) {
        n_temp = pn_x[i];
        for (j = i; j > 0 && n_temp < pn_x[j - 1]; j--)
            pn_x[j] = pn_x[j - 1];
        pn_x[j] = n_temp;
    }
}

void maxim_sort_indices_descend(int32_t *pn_x, int32_t *pn_indx, int32_t n_size)
{
    int32_t i, j, n_temp;
    for (i = 1; i < n_size; i++) {
        n_temp = pn_indx[i];
        for (j = i; j > 0 && pn_x[n_temp] > pn_x[pn_indx[j - 1]]; j--)
            pn_indx[j] = pn_indx[j - 1];
        pn_indx[j] = n_temp;
    }
}

/* ========================================================================
 * Service API
 * ======================================================================== */

void Max30102_Init(void)
{
    max30102_init(&m_max, &hi2c2);
    max30102_reset(&m_max);
    HAL_Delay(10); /* 等复位完成 */

    max30102_set_fifo_config(&m_max, max30102_smp_ave_1, 1, 7);
    max30102_set_led_pulse_width(&m_max, max30102_pw_16_bit);
    max30102_set_adc_resolution(&m_max, max30102_adc_8192); /* 量程加大, 留更多余量 */
    max30102_set_sampling_rate(&m_max, max30102_sr_100); /* FIFO_CONFIG 已确认可写(采样平均1), 100Hz 匹配算法 */
    max30102_set_led_current_1(&m_max, 15.0f); /* 实际RED LED: 之前30mA会顶满(93%)削波, 调小 */
    max30102_set_led_current_2(&m_max, 22.0f); /* 实际IR LED: 加大, 心率信号更强更清晰 */
    max30102_set_mode(&m_max, max30102_spo2);

    /* 联调: 重写 FIFO_CONFIG 并立即读回, 确认采样平均能否设成 1 */
    {
        uint8_t c = 0, m = 0, pid = 0, fc = 0x17; /* SMP_AVE=1, rollover=1, A_FULL=7 */
        max30102_write(&m_max, 0x08, &fc, 1);
        fc = 0;
        max30102_read(&m_max, 0x0a, &c, 1);  /* SPO2_CONFIG */
        max30102_read(&m_max, 0x09, &m, 1);  /* MODE_CONFIG */
        max30102_read(&m_max, 0xff, &pid, 1); /* PART_ID */
        max30102_read(&m_max, 0x08, &fc, 1);  /* FIFO_CONFIG 读回 */
        printf("[MAX] init spo2_cfg=0x%02X mode=0x%02X part=0x%02X fifo=0x%02X\n", c, m, pid, fc);
    }

    m_buf_idx = 0;
    m_hr = -999;
    m_hr_valid = 0;
    m_spo2 = -999;
    m_spo2_valid = 0;
}

/* 启动测量(进心率屏时调用) */
void Max30102_Start(void)
{
    Max30102_Init();
    m_enabled = 1;
    m_buf_idx = 0;
    m_hr = -999; m_hr_valid = 0;
    m_spo2 = -999; m_spo2_valid = 0;
    m_hr_smooth = -999;
}

/* 停止测量 + 传感器休眠(离开心率屏时调用, 省电) */
void Max30102_Stop(void)
{
    m_enabled = 0;
    max30102_shutdown(&m_max, 1); /* 传感器进低功耗模式 */
}

void Max30102_Update(void)
{
    static uint32_t last_read = 0;
    uint32_t now = hw_ms;
    if (!m_enabled)
        return; /* 未测量(不在心率屏)时跳过 */
    if (now - last_read < READ_INTERVAL_MS)
        return; /* 节流: 每 20ms 读一次 FIFO */
    last_read = now;

    /* 每秒打印一次状态(联调用, 之后可删) */
    {
        static uint32_t last_print = 0;
        if (now - last_print >= 1000) {
            last_print = now;
            printf("[MAX] hr=%d spo2=%d hrV=%d spo2V=%d\n",
                   (int)m_hr, (int)m_spo2, (int)m_hr_valid, (int)m_spo2_valid);
        }
    }

    int8_t n = max30102_read_fifo(&m_max);
    if (n < 1)
        return;

    for (int8_t i = 0; i < n && m_buf_idx < BUFFER_SIZE; i++) {
        /* eepj 驱动把 FIFO 的前3字节标成 ir、后3字节标成 red, 实际是反的:
         * MAX30102 SpO2 模式顺序 = [RED, IR]。这里换回来, 让算法用真正的 IR 算心率 */
        m_ir_buf[m_buf_idx] = m_max._red_samples[i];  /* 实际 IR */
        m_last_ir = m_max._red_samples[i];
        m_red_buf[m_buf_idx] = m_max._ir_samples[i];  /* 实际 RED */
        m_last_red = m_max._ir_samples[i];
        m_buf_idx++;
    }

    if (m_buf_idx >= BUFFER_SIZE) {
        maxim_heart_rate_and_oxygen_saturation(m_ir_buf, BUFFER_SIZE, m_red_buf,
                                               &m_spo2, &m_spo2_valid, &m_hr, &m_hr_valid);
        /* 信号太弱(无手指/接触差): 覆盖为无效 */
        if (m_last_ir < SIGNAL_THRESHOLD) {
            m_hr = -999; m_hr_valid = 0;
            m_spo2 = -999; m_spo2_valid = 0;
        }
        /* 心率范围过滤 + 平滑, 抑制突变 */
        if (m_hr_valid && m_hr >= HR_MIN && m_hr <= HR_MAX) {
            if (m_hr_smooth < 0) {
                m_hr_smooth = m_hr;
            } else {
                m_hr_smooth = (m_hr_smooth * (HR_SMOOTH_N - 1) + m_hr) / HR_SMOOTH_N;
            }
            m_hr = m_hr_smooth;
        } else {
            m_hr = -999; m_hr_valid = 0;
            m_spo2 = -999; m_spo2_valid = 0;
            m_hr_smooth = -999;
        }
        m_buf_idx = 0;
    }
}

int32_t Max30102_GetHeartRate(void) { return m_hr; }
int32_t Max30102_GetSpO2(void) { return m_spo2; }
uint8_t Max30102_IsHeartRateValid(void) { return m_hr_valid; }
uint8_t Max30102_IsSpO2Valid(void) { return m_spo2_valid; }
