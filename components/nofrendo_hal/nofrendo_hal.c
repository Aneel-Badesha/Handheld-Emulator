#include "nofrendo_hal.h"

#include "ili9341.h"
#include "nes_input.h"
#include "esp_log.h"

static const char *TAG = "nofrendo_hal";

void hal_display_frame(const uint8_t *palette_frame, int width, int height)
{
    /* TODO: convert palette_frame (256x240 indexed) to RGB565,
     *       scale to 320x240, and DMA to the ILI9341 via ili9341_draw_frame_async(). */
    (void)palette_frame; (void)width; (void)height;
}

void hal_input_poll(uint8_t *p1_buttons)
{
    /* TODO: read NES controller state from hardware inputs */
    if (p1_buttons != NULL) {
        *p1_buttons = nes_input_read();
    }
}

void hal_audio_push(const int16_t *samples, int count)
{
    /* TODO: implement when PCM5102A hardware is added (I2S GPIO 14/47/48) */
    (void)samples; (void)count;
}
