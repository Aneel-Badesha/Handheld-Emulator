#include "ili9341.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <string.h>

static const char *TAG = "ili9341";

/* SPI2 pin assignments — shared with SD card (separate CS pins) */
#define ILI9341_PIN_MOSI 11
#define ILI9341_PIN_MISO 13
#define ILI9341_PIN_SCLK 12
#define ILI9341_PIN_CS   10
#define ILI9341_PIN_DC   46
#define ILI9341_PIN_RST   3
#define ILI9341_PIN_BL   45

/* SPI clock — ILI9341 supports up to 40 MHz */
#define ILI9341_SPI_FREQ_HZ (10 * 1000 * 1000)

/* Command codes */
#define CMD_SWRESET  0x01
#define CMD_SLPOUT   0x11
#define CMD_DISPON   0x29
#define CMD_CASET    0x2A
#define CMD_RASET    0x2B
#define CMD_RAMWR    0x2C
#define CMD_MADCTL   0x36
#define CMD_COLMOD   0x3A
#define CMD_PWCTR1   0xC0
#define CMD_PWCTR2   0xC1
#define CMD_VMCTR1   0xC5
#define CMD_VMCTR2   0xC7
#define CMD_FRMCTR1  0xB1
#define CMD_DFUNCTR  0xB6
#define CMD_GMCTRP1  0xE0
#define CMD_GMCTRN1  0xE1

/*
 * MADCTL: landscape 320x240, BGR pixel order
 * MX (0x40) + MV (0x20) swaps axes for landscape; BGR (0x08) matches most ILI9341 panels
 * Adjust if colours appear inverted or display is mirrored
 */
#define MADCTL_VAL (0x20 | 0x08)

/*
 * Async frame DMA: split 240 lines into 15 chunks of 16 lines each
 * Each chunk = 320 * 16 * 2 = 10 240 bytes, well within the DMA descriptor limit
 *
 * Frame buffer passed to ili9341_draw_frame_async() must contain RGB565 pixels
 * in big-endian byte order (high byte first), as required by the ILI9341 over SPI
 * Use __builtin_bswap16() or htons() when filling the buffer
 */
#define FRAME_LINES_PER_CHUNK  16
#define FRAME_NUM_CHUNKS       (ILI9341_HEIGHT / FRAME_LINES_PER_CHUNK)  /* 15 */
#define FRAME_CHUNK_BYTES      (ILI9341_WIDTH * FRAME_LINES_PER_CHUNK * sizeof(uint16_t))

/* 5 address-window setup transactions + pixel-data chunks */
#define FRAME_SETUP_TRANS  5
#define FRAME_TOTAL_TRANS  (FRAME_SETUP_TRANS + FRAME_NUM_CHUNKS)  /* 20 */

static spi_device_handle_t s_spi;

/* Pre-built transactions for full-frame async DMA */
static spi_transaction_t s_frame_trans[FRAME_TOTAL_TRANS];

/* Address-window data payloads — constant for a full-screen blit */
static const uint8_t s_caset_data[4] = {
    0x00, 0x00,
    (ILI9341_WIDTH  - 1) >> 8, (ILI9341_WIDTH  - 1) & 0xFF   /* 0x01, 0x3F = 319 */
};
static const uint8_t s_raset_data[4] = {
    0x00, 0x00,
    (ILI9341_HEIGHT - 1) >> 8, (ILI9341_HEIGHT - 1) & 0xFF   /* 0x00, 0xEF = 239 */
};

/* ------------------------------------------------------------------ */
/*  SPI helpers                                                        */
/* ------------------------------------------------------------------ */

/* Pre-transfer callback: drive DC low (command) or high (data) */
static void IRAM_ATTR spi_pre_cb(spi_transaction_t *t)
{
    gpio_set_level(ILI9341_PIN_DC, (int)t->user);
}

static void lcd_cmd(uint8_t cmd)
{
    spi_transaction_t t = {
        .length = 8,
        .flags  = SPI_TRANS_USE_TXDATA,
        .user   = (void *)0,   /* DC = 0 */
    };
    t.tx_data[0] = cmd;
    ESP_ERROR_CHECK(spi_device_polling_transmit(s_spi, &t));
}

static void lcd_data(const uint8_t *data, size_t len)
{
    if (len == 0) return;
    spi_transaction_t t = {
        .length    = len * 8,
        .tx_buffer = data,
        .user      = (void *)1,   /* DC = 1 */
    };
    ESP_ERROR_CHECK(spi_device_polling_transmit(s_spi, &t));
}

/* Set the column/row address window for subsequent RAMWR pixel writes */
static void set_addr_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    uint8_t col[4] = { x0 >> 8, x0 & 0xFF, x1 >> 8, x1 & 0xFF };
    uint8_t row[4] = { y0 >> 8, y0 & 0xFF, y1 >> 8, y1 & 0xFF };
    lcd_cmd(CMD_CASET); lcd_data(col, 4);
    lcd_cmd(CMD_RASET); lcd_data(row, 4);
    lcd_cmd(CMD_RAMWR);
}

/* ------------------------------------------------------------------ */
/*  One-time async transaction setup                                   */
/* ------------------------------------------------------------------ */

static void build_frame_transactions(void)
{
    memset(s_frame_trans, 0, sizeof(s_frame_trans));

    /* [0] CASET command */
    s_frame_trans[0].length     = 8;
    s_frame_trans[0].flags      = SPI_TRANS_USE_TXDATA;
    s_frame_trans[0].tx_data[0] = CMD_CASET;
    s_frame_trans[0].user       = (void *)0;

    /* [1] CASET data: columns 0–319 */
    s_frame_trans[1].length    = 4 * 8;
    s_frame_trans[1].tx_buffer = s_caset_data;
    s_frame_trans[1].user      = (void *)1;

    /* [2] RASET command */
    s_frame_trans[2].length     = 8;
    s_frame_trans[2].flags      = SPI_TRANS_USE_TXDATA;
    s_frame_trans[2].tx_data[0] = CMD_RASET;
    s_frame_trans[2].user       = (void *)0;

    /* [3] RASET data: rows 0–239 */
    s_frame_trans[3].length    = 4 * 8;
    s_frame_trans[3].tx_buffer = s_raset_data;
    s_frame_trans[3].user      = (void *)1;

    /* [4] RAMWR command */
    s_frame_trans[4].length     = 8;
    s_frame_trans[4].flags      = SPI_TRANS_USE_TXDATA;
    s_frame_trans[4].tx_data[0] = CMD_RAMWR;
    s_frame_trans[4].user       = (void *)0;

    /* [5–19] Pixel chunks — tx_buffer is set per call in draw_frame_async */
    for (int i = 0; i < FRAME_NUM_CHUNKS; i++) {
        s_frame_trans[FRAME_SETUP_TRANS + i].length = FRAME_CHUNK_BYTES * 8;
        s_frame_trans[FRAME_SETUP_TRANS + i].user   = (void *)1;
    }
}

/* ------------------------------------------------------------------ */
/*  Public API                                                         */
/* ------------------------------------------------------------------ */

esp_err_t ili9341_init(void)
{
    /* Configure DC, RST, BL as GPIO outputs */
    gpio_config_t io = {
        .pin_bit_mask = (1ULL << ILI9341_PIN_DC)  |
                        (1ULL << ILI9341_PIN_RST) |
                        (1ULL << ILI9341_PIN_BL),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io));

    gpio_set_level(ILI9341_PIN_BL, 0);   /* backlight off during init */

    /* Hardware reset: hold RST low for 10 ms, then release and wait for the
     * display to come out of reset (ILI9341 datasheet: 120 ms minimum) */
    gpio_set_level(ILI9341_PIN_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(ILI9341_PIN_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(120));

    /* Initialize SPI2 bus */
    spi_bus_config_t bus = {
        .mosi_io_num     = ILI9341_PIN_MOSI,
        .miso_io_num     = ILI9341_PIN_MISO,
        .sclk_io_num     = ILI9341_PIN_SCLK,
        .quadwp_io_num   = -1,
        .quadhd_io_num   = -1,
        .max_transfer_sz = ILI9341_WIDTH * ILI9341_HEIGHT * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO));

    spi_device_interface_config_t dev = {
        .clock_speed_hz = ILI9341_SPI_FREQ_HZ,
        .mode           = 0,
        .spics_io_num   = ILI9341_PIN_CS,
        .queue_size     = FRAME_TOTAL_TRANS,
        .pre_cb         = spi_pre_cb,
    };
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &dev, &s_spi));

    /* ---- ILI9341 software init sequence ---- */
    lcd_cmd(CMD_SWRESET);
    vTaskDelay(pdMS_TO_TICKS(150));

    lcd_cmd(CMD_SLPOUT);
    vTaskDelay(pdMS_TO_TICKS(120));

    lcd_cmd(CMD_PWCTR1);  lcd_data((uint8_t[]){0x23}, 1);           /* VRH = 4.60 V */
    lcd_cmd(CMD_PWCTR2);  lcd_data((uint8_t[]){0x10}, 1);           /* SAP, BT */
    lcd_cmd(CMD_VMCTR1);  lcd_data((uint8_t[]){0x3E, 0x28}, 2);     /* VCOMH/VCOML */
    lcd_cmd(CMD_VMCTR2);  lcd_data((uint8_t[]){0x86}, 1);
    lcd_cmd(CMD_COLMOD);  lcd_data((uint8_t[]){0x55}, 1);           /* 16-bit RGB565 */
    lcd_cmd(CMD_FRMCTR1); lcd_data((uint8_t[]){0x00, 0x18}, 2);     /* ~60 Hz */
    lcd_cmd(CMD_DFUNCTR); lcd_data((uint8_t[]){0x08, 0x82, 0x27}, 3);
    lcd_cmd(CMD_MADCTL);  lcd_data((uint8_t[]){MADCTL_VAL}, 1);

    lcd_cmd(CMD_GMCTRP1);
    lcd_data((uint8_t[]){0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08,
                          0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03,
                          0x0E, 0x09, 0x00}, 15);

    lcd_cmd(CMD_GMCTRN1);
    lcd_data((uint8_t[]){0x00, 0x0E, 0x14, 0x03, 0x11, 0x07,
                          0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C,
                          0x31, 0x36, 0x0F}, 15);

    lcd_cmd(CMD_DISPON);
    vTaskDelay(pdMS_TO_TICKS(100));

    gpio_set_level(ILI9341_PIN_BL, 1);   /* backlight on */

    build_frame_transactions();

    ESP_LOGI(TAG, "ILI9341 init complete (320x240, RGB565)");
    return ESP_OK;
}

void ili9341_clear(uint16_t color)
{
    /* Byte-swap: SPI transmits memory in address order (low byte first on
     * little-endian ESP32-S3), but ILI9341 expects the high byte first */
    uint16_t be = (color << 8) | (color >> 8);
    uint16_t line[ILI9341_WIDTH];
    for (int i = 0; i < ILI9341_WIDTH; i++) {
        line[i] = be;
    }
    set_addr_window(0, 0, ILI9341_WIDTH - 1, ILI9341_HEIGHT - 1);
    for (int row = 0; row < ILI9341_HEIGHT; row++) {
        lcd_data((uint8_t *)line, sizeof(line));
    }
}

void ili9341_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
    if (w <= 0 || h <= 0) return;

    /* Clip to display bounds */
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > ILI9341_WIDTH)  w = ILI9341_WIDTH  - x;
    if (y + h > ILI9341_HEIGHT) h = ILI9341_HEIGHT - y;
    if (w <= 0 || h <= 0) return;

    uint16_t be = (color << 8) | (color >> 8);
    uint16_t line[ILI9341_WIDTH];
    for (int i = 0; i < w; i++) line[i] = be;

    set_addr_window(x, y, x + w - 1, y + h - 1);
    for (int row = 0; row < h; row++) {
        lcd_data((uint8_t *)line, w * sizeof(uint16_t));
    }
}

esp_err_t ili9341_draw_frame_async(const uint16_t *buf)
{
    /* Point each pixel-chunk transaction at the corresponding slice of buf
     * buf must stay valid until ili9341_wait_frame_done() returns, and must
     * contain pixels in big-endian RGB565 byte order */
    for (int i = 0; i < FRAME_NUM_CHUNKS; i++) {
        s_frame_trans[FRAME_SETUP_TRANS + i].tx_buffer =
            (const uint8_t *)buf + i * FRAME_CHUNK_BYTES;
    }

    for (int i = 0; i < FRAME_TOTAL_TRANS; i++) {
        esp_err_t ret = spi_device_queue_trans(s_spi, &s_frame_trans[i], portMAX_DELAY);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "queue_trans[%d] failed: %s", i, esp_err_to_name(ret));
            return ret;
        }
    }
    return ESP_OK;
}

esp_err_t ili9341_wait_frame_done(void)
{
    spi_transaction_t *t;
    for (int i = 0; i < FRAME_TOTAL_TRANS; i++) {
        esp_err_t ret = spi_device_get_trans_result(s_spi, &t, portMAX_DELAY);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "get_trans_result[%d] failed: %s", i, esp_err_to_name(ret));
            return ret;
        }
    }
    return ESP_OK;
}

esp_err_t ili9341_deinit(void)
{
    gpio_set_level(ILI9341_PIN_BL, 0);
    esp_err_t ret = spi_bus_remove_device(s_spi);
    if (ret == ESP_OK) ret = spi_bus_free(SPI2_HOST);
    s_spi = NULL;
    return ret;
}
