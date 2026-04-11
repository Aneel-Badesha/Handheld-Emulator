#include "ili9341.h"
#include "sdcard.h"
#include "nes_input.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "main";

/* Read inputs in a loop to confirm buttons and thumbstick work */
static void input_test_task(void *arg)
{
    uint8_t prev = 0;
    for (;;) {
        uint8_t cur = nes_input_read();
        if (cur != prev) {
            ESP_LOGI(TAG, "input: 0x%02X  A=%d B=%d SEL=%d STA=%d U=%d D=%d L=%d R=%d",
                cur,
                !!(cur & NES_BTN_A),
                !!(cur & NES_BTN_B),
                !!(cur & NES_BTN_SELECT),
                !!(cur & NES_BTN_START),
                !!(cur & NES_BTN_UP),
                !!(cur & NES_BTN_DOWN),
                !!(cur & NES_BTN_LEFT),
                !!(cur & NES_BTN_RIGHT));
            prev = cur;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "--- init start ---");

    /* ili9341_init() must come first — it owns the SPI2 bus */
    ESP_ERROR_CHECK(ili9341_init());
    ESP_LOGI(TAG, "display OK");

    /* Paint the screen blue so we can see the display is alive */
    ili9341_clear(RGB565(0, 0, 255));

    esp_err_t sd_err = sdcard_init();
    if (sd_err != ESP_OK) {
        ESP_LOGW(TAG, "SD card init failed (0x%x) — skipping", sd_err);
    } else {
        ESP_LOGI(TAG, "SD card OK");
    }

    ESP_ERROR_CHECK(nes_input_init());
    ESP_LOGI(TAG, "input OK");

    ESP_LOGI(TAG, "--- init done ---");

    /* Blink through a few colors to confirm DMA blit works */
    uint16_t colors[] = {
        RGB565(255, 0,   0),    /* red   */
        RGB565(0,   255, 0),    /* green */
        RGB565(0,   0,   255),  /* blue  */
        RGB565(0,   0,   0),    /* black */
    };
    for (int i = 0; i < 4; i++) {
        ili9341_clear(colors[i]);
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    xTaskCreatePinnedToCore(input_test_task, "input_test", 4096, NULL, 5, NULL, 0);
}
