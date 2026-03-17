#include "sdcard.h"

#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include <errno.h>
#include <string.h>
#include <sys/stat.h>

static const char *TAG = "sdcard";

/* SD card CS pin — MOSI/MISO/SCLK are shared with the ILI9341 on SPI2 */
#define SD_CARD_CS   6
#define MOUNT_POINT  "/sdcard"

static sdmmc_card_t *s_card = NULL;
static sdmmc_host_t  s_host;

esp_err_t sdcard_init(void)
{
    esp_err_t rc;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024,
    };

    ESP_LOGI(TAG, "Initializing SD card on SPI2");
    s_host = (sdmmc_host_t)SDSPI_HOST_DEFAULT();
    s_host.slot = SPI2_HOST;    /* SPI2 bus — initialized by ili9341_init() */
    s_host.max_freq_khz = 4000; /* 4 MHz — conservative for breadboard wiring */

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs  = SD_CARD_CS;
    slot_config.host_id  = s_host.slot;

    ESP_LOGI(TAG, "Mounting filesystem at %s", MOUNT_POINT);
    rc = esp_vfs_fat_sdspi_mount(MOUNT_POINT, &s_host, &slot_config, &mount_config, &s_card);
    if (rc != ESP_OK) {
        if (rc == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem");
        } else {
            ESP_LOGE(TAG, "Failed to initialize card: %s", esp_err_to_name(rc));
        }
        return rc;
    }

    ESP_LOGI(TAG, "Filesystem mounted");
    sdmmc_card_print_info(stdout, s_card);
    return ESP_OK;
}

esp_err_t sdcard_read_file(const char *path)
{
    ESP_LOGI(TAG, "Reading file %s", path);
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return ESP_FAIL;
    }

    char line[256];
    fgets(line, sizeof(line), f);
    fclose(f);

    char *pos = strchr(line, '\n');
    if (pos) {
        *pos = '\0';
    }
    ESP_LOGI(TAG, "Read: '%s'", line);
    return ESP_OK;
}

esp_err_t sdcard_write_file(const char *path, const char *data)
{
    ESP_LOGI(TAG, "Writing file %s", path);
    FILE *f = fopen(path, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing (errno %d)", errno);
        return ESP_FAIL;
    }
    fprintf(f, "%s", data);
    fclose(f);
    ESP_LOGI(TAG, "File written");
    return ESP_OK;
}

esp_err_t sdcard_deinit(void)
{
    esp_vfs_fat_sdcard_unmount(MOUNT_POINT, s_card);
    s_card = NULL;
    ESP_LOGI(TAG, "Card unmounted");
    /* SPI2 bus is owned by ili9341 — do not free it here */
    return ESP_OK;
}
