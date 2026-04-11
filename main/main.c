#include "ili9341.h"
#include "sdcard.h"
#include "nes_input.h"
#include "osd.h"
#include "nofrendo.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "main";

/* ROM path set by sdcard_task once a ROM is selected — NULL runs built-in demo */
static const char *s_rom_path = NULL;

static void emulator_task(void *arg)
{
    if (osd_init() != 0) {
        ESP_LOGE(TAG, "osd_init failed");
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "starting emulator: %s", s_rom_path ? s_rom_path : "built-in demo");
    main_loop(s_rom_path, system_autodetect);

    /* main_loop only returns on quit */
    osd_shutdown();
    vTaskDelete(NULL);
}

void app_main(void)
{
    ESP_LOGI(TAG, "--- init start ---");

    ESP_ERROR_CHECK(ili9341_init());
    ESP_LOGI(TAG, "display OK");

    ESP_ERROR_CHECK(sdcard_bus_init());
    esp_err_t sd_err = sdcard_init();
    if (sd_err != ESP_OK) {
        ESP_LOGW(TAG, "SD card unavailable (0x%x) — no ROM browser", sd_err);
    } else {
        ESP_LOGI(TAG, "SD card OK");
    }

    ESP_ERROR_CHECK(nes_input_init());
    ESP_LOGI(TAG, "input OK");

    ESP_LOGI(TAG, "--- init done, launching emulator ---");

    /* 16 KB stack — NOFRENDO's 6502 core uses significant stack depth */
    xTaskCreatePinnedToCore(emulator_task, "emulator", 16384, NULL, 5, NULL, 0);
}
