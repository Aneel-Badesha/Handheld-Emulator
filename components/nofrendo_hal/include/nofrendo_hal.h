#ifndef NOFRENDO_HAL_H
#define NOFRENDO_HAL_H

#include <stdint.h>

/*
 * Hardware Abstraction Layer — functions called by NOFRENDO internals.
 * Implementations in nofrendo_hal.c bridge NOFRENDO to ESP32-S3 hardware.
 */

/*
 * Receive a completed NES frame and send it to the ILI9341 display.
 * palette_frame: 256*240 bytes of palette indices (NES native format).
 * Scale from 256x240 to 320x240 and convert to RGB565 before DMA transfer.
 */
void hal_display_frame(const uint8_t *palette_frame, int width, int height);

/*
 * Poll hardware inputs and fill p1_buttons with the NES controller bitmask.
 * Uses nes_input_read() internally.
 */
void hal_input_poll(uint8_t *p1_buttons);

/*
 * Receive APU audio samples and push them to the I2S DMA buffer.
 * samples: interleaved stereo int16 samples.
 * count: number of int16 samples (total, both channels).
 * Not implemented until PCM5102A hardware is added.
 */
void hal_audio_push(const int16_t *samples, int count);

#endif /* NOFRENDO_HAL_H */
