#include "ili9341.h"
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
    ESP_LOGI(TAG, "starting emulator: %s", s_rom_path ? s_rom_path : "built-in demo");
    main_loop(s_rom_path ? s_rom_path : "builtin", system_autodetect);

    /* main_loop only returns on quit */
    osd_shutdown();
    vTaskDelete(NULL);
}

void app_main(void)
{
    ESP_LOGI(TAG, "--- init start ---");

    ESP_ERROR_CHECK(ili9341_init());
    ili9341_clear(0x0000);   /* black — confirms display pipeline works */
    ESP_LOGI(TAG, "display OK");

    ESP_ERROR_CHECK(nes_input_init());
    ESP_LOGI(TAG, "input OK");

    ESP_LOGI(TAG, "--- init done, launching emulator ---");

    xTaskCreatePinnedToCore(emulator_task, "emulator", 32768, NULL, 5, NULL, 0);
}
