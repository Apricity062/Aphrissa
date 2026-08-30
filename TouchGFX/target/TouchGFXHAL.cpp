/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : TouchGFXHAL.cpp
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

#include <TouchGFXHAL.hpp>

/* USER CODE BEGIN TouchGFXHAL.cpp */

#include <cstring>
#include <touchgfx/hal/OSWrappers.hpp>
#include "spi.h"
#include "stm32u5xx_hal.h"

using namespace touchgfx;

extern SPI_HandleTypeDef hspi1;
extern DMA_HandleTypeDef handle_GPDMA1_Channel0;

/* ────────────────────────────────────────────────────
 *  VSYNC simulation for SPI display (no TE pin)
 * ──────────────────────────────────────────────────── */
extern "C" void TouchGFX_SignalVSync(void)
{
    OSWrappers::signalVSync();
}

/* ────────────────────────────────────────────────────
 *  initialize: init TouchGFX framework.
 *  ILI9341_Init() + FT6336_init() are called in main()
 *  (same flow as the reference 0C-2 example project).
 * ──────────────────────────────────────────────────── */
void TouchGFXHAL::initialize()
{
    TouchGFXGeneratedHAL::initialize();

    /* Clear framebuffer to black.
     * frameBuf is in a NOLOAD section not covered by BSS zeroing,
     * so its initial content is random RAM data. */
    uint16_t* fb = getTFTFrameBuffer();
    if (fb != 0)
    {
        std::memset(fb, 0, 240 * 240 * 2);
    }
}

/* ────────────────────────────────────────────────────
 *  getTFTFrameBuffer / setTFTFrameBuffer
 * ──────────────────────────────────────────────────── */
uint16_t* TouchGFXHAL::getTFTFrameBuffer() const
{
    return TouchGFXGeneratedHAL::getTFTFrameBuffer();
}

void TouchGFXHAL::setTFTFrameBuffer(uint16_t* address)
{
    TouchGFXGeneratedHAL::setTFTFrameBuffer(address);
}

/* ────────────────────────────────────────────────────
 *  USER_SPI_Transmit_DMA — manual GPDMA1 CH0 → SPI1_TX
 *  Sends pSize halfwords.  Copied from the reference project.
 * ──────────────────────────────────────────────────── */
static void USER_SPI_Transmit_DMA(const uint16_t *pData, uint16_t pSize)
{
    /* Set the transaction information */
    hspi1.State       = HAL_SPI_STATE_READY;
    hspi1.ErrorCode   = HAL_SPI_ERROR_NONE;

    /* Init field not used in handle to zero */
    hspi1.RxISR       = NULL;
    hspi1.TxISR       = NULL;

    /* Configure communication direction : 1Line */
    SPI_2LINES_TX(&hspi1);

    /* Packing mode management is enabled by the DMA settings */
    (void)IS_SPI_FULL_INSTANCE(hspi1.Instance);

    /* Clear TXDMAEN bit */
    CLEAR_BIT(hspi1.Instance->CFG1, SPI_CFG1_TXDMAEN);

    /* Update the DMA channel state */
    handle_GPDMA1_Channel0.State = HAL_DMA_STATE_BUSY;
    /* Update the DMA channel error code */
    handle_GPDMA1_Channel0.ErrorCode = HAL_DMA_ERROR_NONE;

    /* Configure the source address, destination address, the data size and clear flags */
    MODIFY_REG(handle_GPDMA1_Channel0.Instance->CBR1, DMA_CBR1_BNDT, ((pSize) & DMA_CBR1_BNDT));

    /* Clear all interrupt flags */
    __HAL_DMA_CLEAR_FLAG(&handle_GPDMA1_Channel0, DMA_FLAG_TC | DMA_FLAG_HT | DMA_FLAG_DTE | DMA_FLAG_ULE | DMA_FLAG_USE | DMA_FLAG_SUSP |
                                                 DMA_FLAG_TO);

    /* Configure DMA channel source address */
    handle_GPDMA1_Channel0.Instance->CSAR = (uint32_t)pData;

    /* Configure DMA channel destination address */
    handle_GPDMA1_Channel0.Instance->CDAR = (uint32_t)&hspi1.Instance->TXDR;

    /* Enable common interrupts: Transfer Complete and Transfer Errors ITs */
    __HAL_DMA_ENABLE_IT(&handle_GPDMA1_Channel0, (DMA_IT_TC | DMA_IT_DTE | DMA_IT_ULE | DMA_IT_USE | DMA_IT_TO));

    /* If Half Transfer complete callback is set, enable the corresponding IT */
    __HAL_DMA_ENABLE_IT(&handle_GPDMA1_Channel0, DMA_IT_HT);

    /* Enable DMA channel */
    __HAL_DMA_ENABLE(&handle_GPDMA1_Channel0);

    /* Set the number of data at current transfer */
    MODIFY_REG(hspi1.Instance->CR2, SPI_CR2_TSIZE, (pSize));

    /* Enable Tx DMA Request */
    SET_BIT(hspi1.Instance->CFG1, SPI_CFG1_TXDMAEN);

    /* Enable the SPI Error Interrupt Bit */
    __HAL_SPI_ENABLE_IT(&hspi1, (SPI_IT_UDR | SPI_IT_FRE | SPI_IT_MODF));

    /* Enable SPI peripheral */
    __HAL_SPI_ENABLE(&hspi1);

    if (((hspi1.Instance->AUTOCR & SPI_AUTOCR_TRIGEN) == 0U) && (hspi1.Init.Mode == SPI_MODE_MASTER))
    {
        /* Master transfer start */
        SET_BIT(hspi1.Instance->CR1, SPI_CR1_CSTART);
    }
}

/* ────────────────────────────────────────────────────
 *  flushFrameBuffer(const Rect& rect)
 *  GPDMA1 DMA-based flush: 16-bit SPI mode + 62KB buffer.
 * ──────────────────────────────────────────────────── */
static uint16_t flushAreaBuf[31745];   /* ~62KB partial-refresh buffer */

void TouchGFXHAL::flushFrameBuffer(const touchgfx::Rect& rect)
{
    __IO uint16_t *pixels;                     /* frame buffer address */
    __IO uint16_t pheight = 0, pWidth = 0, pBuffCnt = 0;
    /* saved length */
    __IO uint32_t pTotalPixel = rect.width * rect.height * 2;
    __IO uint32_t pFull   = pTotalPixel / 63488;  /* 62KB rounded down */
    __IO uint32_t pRemain = pTotalPixel % 63488;  /* 62KB remainder */

    /* set display area */
    ILI9341_SetArea(rect.x, rect.y, rect.x + rect.width - 1, rect.y + rect.height - 1);
    ILI9341_WriteRAM_Prepare();                 /* start writing GRAM */

    /* SPI data 16-bit: buffer is little-endian, ILI9341 expects MSB first */
    hspi1.Instance->CFG1 &= (~0x1F);
    hspi1.Instance->CFG1 |= SPI_DATASIZE_16BIT;

    if ((rect.width == HAL::DISPLAY_WIDTH) && (rect.height == HAL::DISPLAY_HEIGHT))  /* full-screen refresh */
    {
        /* pixel start address */
        pixels = getClientFrameBuffer() + rect.x + (rect.y) * HAL::DISPLAY_WIDTH;
        /* transmit 62KB pixel chunks */
        for (pBuffCnt = 0; pBuffCnt < pFull; pBuffCnt++)
        {
            USER_SPI_Transmit_DMA((uint16_t *)pixels, 63488);  /* max 0xFFFF per DMA */
            pixels = pixels + 31744;               /* address offset */
            while (HAL_DMA_GetState(&handle_GPDMA1_Channel0) != HAL_DMA_STATE_READY);  /* wait */
            HAL_Delay(0);
            HAL_SPI_Abort(&hspi1);                 /* terminate blocking transfer */
        }
        USER_SPI_Transmit_DMA((uint16_t *)pixels, pRemain);   /* remaining data */
        while (HAL_DMA_GetState(&handle_GPDMA1_Channel0) != HAL_DMA_STATE_READY);
        HAL_Delay(0);
        HAL_SPI_Abort(&hspi1);
    }
    else  /* partial refresh */
    {
        for (pheight = 0; pheight < rect.height; pheight++)
        {
            /* buffer little-endian, ILI9341 MSB first */
            pixels = getClientFrameBuffer() + rect.x + (pheight + rect.y) * HAL::DISPLAY_WIDTH;
            /* read pixels into buffer */
            for (pWidth = 0; pWidth < rect.width; pWidth++)
            {
                flushAreaBuf[pBuffCnt++] = *pixels;   /* read pixel */
                pixels++;
                if (pBuffCnt >= 31744)                /* buffer full → DMA to screen */
                {
                    USER_SPI_Transmit_DMA((uint16_t *)flushAreaBuf, 63488);
                    while (HAL_DMA_GetState(&handle_GPDMA1_Channel0) != HAL_DMA_STATE_READY);
                    HAL_Delay(0);
                    HAL_SPI_Abort(&hspi1);
                    pBuffCnt = 0;
                }
            }
        }
        USER_SPI_Transmit_DMA((uint16_t *)flushAreaBuf, pBuffCnt * 2);  /* remaining */
        while (HAL_DMA_GetState(&handle_GPDMA1_Channel0) != HAL_DMA_STATE_READY);
        HAL_Delay(0);
        HAL_SPI_Abort(&hspi1);
    }

    /* restore SPI data size to 8-bit */
    hspi1.Instance->CFG1 &= (~0x1F);
    hspi1.Instance->CFG1 |= SPI_DATASIZE_8BIT;

    TouchGFXGeneratedHAL::flushFrameBuffer(rect);
}

/* ────────────────────────────────────────────────────
 *  blockCopy
 * ──────────────────────────────────────────────────── */
bool TouchGFXHAL::blockCopy(void* RESTRICT dest, const void* RESTRICT src, uint32_t numBytes)
{
    return TouchGFXGeneratedHAL::blockCopy(dest, src, numBytes);
}

/* ────────────────────────────────────────────────────
 *  Interrupt / Controller stubs
 * ──────────────────────────────────────────────────── */
void TouchGFXHAL::configureInterrupts()
{
    TouchGFXGeneratedHAL::configureInterrupts();
}

void TouchGFXHAL::enableInterrupts()
{
    TouchGFXGeneratedHAL::enableInterrupts();
}

void TouchGFXHAL::disableInterrupts()
{
    TouchGFXGeneratedHAL::disableInterrupts();
}

void TouchGFXHAL::enableLCDControllerInterrupt()
{
    /* Not used for SPI display */
}

bool TouchGFXHAL::beginFrame()
{
    return TouchGFXGeneratedHAL::beginFrame();
}

void TouchGFXHAL::endFrame()
{
    TouchGFXGeneratedHAL::endFrame();
}

/* USER CODE END TouchGFXHAL.cpp */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
