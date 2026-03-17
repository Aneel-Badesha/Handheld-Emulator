#ifndef ILI9341_H
#define ILI9341_H

#include "esp_err.h"
#include <stdint.h>

/* Display dimensions */
#define ILI9341_WIDTH  320
#define ILI9341_HEIGHT 240

/* RGB565 color helpers */
#define RGB565(r, g, b) ((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3))
#define COLOR_BLACK   0x0000
#define COLOR_WHITE   0xFFFF

/*
 * Initialize the ILI9341 display and the SPI2 bus.
 * Must be called before sdcard_init(), which adds its device to the same bus.
 */
esp_err_t ili9341_init(void);

/* Fill the entire display with a solid color */
void ili9341_clear(uint16_t color);

/* Draw a filled rectangle */
void ili9341_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);

/*
 * Begin a non-blocking DMA transfer of a full 320x240 RGB565 frame.
 * buf must remain valid until ili9341_wait_frame_done() returns.
 */
esp_err_t ili9341_draw_frame_async(const uint16_t *buf);

/* Block until the in-flight DMA frame transfer completes */
esp_err_t ili9341_wait_frame_done(void);

/* Release display and SPI2 bus resources */
esp_err_t ili9341_deinit(void);

#endif /* ILI9341_H */
