/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : STM32TouchController.cpp
  ******************************************************************************
  * This file was created by TouchGFX Generator 4.26.1. This file is only
  * generated once! Delete this file from your project and re-generate code
  * using STM32CubeMX or change this file manually to update it.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* USER CODE BEGIN STM32TouchController */

#include <STM32TouchController.hpp>
#include "bsp_ft6336.h"

extern "C" {
#include "motion_service.h"
}

extern "C" FT6336_TouchPointType tp;
extern volatile uint32_t ft6336_on_touch_count;

void STM32TouchController::init()
{
    /**
     * Initialize touch controller and driver
     *
     * FT6336_init() is called in main() (same flow as the
     * reference 0C-2 example project).
     */
}

bool STM32TouchController::sampleTouch(int32_t& x, int32_t& y)
{
    /**
     * return true if a touch has been detected, otherwise false.
     * Called by the TouchGFX framework every tick.
     */
    uint16_t xDiff = 0, yDiff = 0;
    static uint16_t pI_Touch_X = 0, pI_Touch_Y = 0;

    /* touch interrupt raised (EXTI5 → FT6336_irq_fuc sets this flag) */
    if (ft6336_on_touch_count)
    {
        uint8_t id1 = FT6336_read_touch1_id(); /* id1 = 0 or 1 */
        tp.tp[id1].status = (tp.tp[id1].status == release) ? touch : stream;
        tp.tp[id1].x = FT6336_read_touch1_x();
        tp.tp[id1].y = FT6336_read_touch1_y();
        tp.tp[~id1 & 0x01].status = release;

        /* update to TouchGFX */
        if (tp.tp[0].status == 1)   /* max two touch points */
        {
            xDiff = tp.tp[0].x > pI_Touch_X ? (tp.tp[0].x - pI_Touch_X) : (pI_Touch_X - tp.tp[0].x);
            yDiff = tp.tp[0].y > pI_Touch_Y ? (tp.tp[0].y - pI_Touch_Y) : (pI_Touch_Y - tp.tp[0].y);
            /* debounce threshold */
            if ((xDiff + yDiff) > 5)
            {
                pI_Touch_X = tp.tp[0].x;
                pI_Touch_Y = tp.tp[0].y;
            }
            /* landscape touch coordinate transform */
            x = pI_Touch_Y;
            y = 240 - pI_Touch_X;
        }

        ft6336_on_touch_count = 0;
        return true;
    }

    return false;
}

/* USER CODE END STM32TouchController */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
