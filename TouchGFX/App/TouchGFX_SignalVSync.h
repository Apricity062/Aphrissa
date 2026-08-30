/**
 ******************************************************************************
 * @file    TouchGFX_SignalVSync.h
 * @brief   C-callable wrapper to simulate VSYNC for SPI displays without TE pin
 ******************************************************************************
 */
#ifndef __TOUCHGFX_SIGNAL_VSYNC_H__
#define __TOUCHGFX_SIGNAL_VSYNC_H__

#ifdef __cplusplus
extern "C" {
#endif

/** Signal TouchGFX that a VSYNC has occurred.
 *  Call this once per frame before touchgfx_taskEntry().
 *  Implemented in TouchGFXHAL.cpp (USER CODE section). */
void TouchGFX_SignalVSync(void);

#ifdef __cplusplus
}
#endif

#endif /* __TOUCHGFX_SIGNAL_VSYNC_H__ */
