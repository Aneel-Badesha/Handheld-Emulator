#include "ili9341.h"

#include "driver/spi_master.h"
#include "esp_log.h"

static const char *TAG = "ili9341";

/* SPI2 pin assignments — shared with SD card (separate CS pins) */
#define ILI9341_PIN_MOSI 11
#define ILI9341_PIN_MISO 13
#define ILI9341_PIN_SCLK 12
#define ILI9341_PIN_CS   10
#define ILI9341_PIN_DC    9
#define ILI9341_PIN_RST   8
#define ILI9341_PIN_BL    7

/* SPI clock — ILI9341 supports up to 40 MHz */
#define ILI9341_SPI_FREQ_HZ (40 * 1000 * 1000)

/* TODO: implement ILI9341 driver */

esp_err_t ili9341_init(void)
{
    ESP_LOGI(TAG, "ili9341_init: not yet implemented");
    return ESP_OK;
}

void ili9341_clear(uint16_t color)
{
    (void)color;
}

void ili9341_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
    (void)x; (void)y; (void)w; (void)h; (void)color;
}

esp_err_t ili9341_draw_frame_async(const uint16_t *buf)
{
    (void)buf;
    return ESP_OK;
}

esp_err_t ili9341_wait_frame_done(void)
{
    return ESP_OK;
}

esp_err_t ili9341_deinit(void)
{
    return ESP_OK;
}
