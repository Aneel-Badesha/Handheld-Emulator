#ifndef SDCARD_H
#define SDCARD_H

#include "esp_err.h"
#include <stdint.h>

/*
 * Add the SD card as a device on the SPI2 bus.
 * ili9341_init() must be called first — it initializes the shared SPI2 bus.
 */
esp_err_t sdcard_init(void);

/* Read first line of a file and log it */
esp_err_t sdcard_read_file(const char *path);

/* Write data to a file, overwriting any existing content */
esp_err_t sdcard_write_file(const char *path, const char *data);

/* Unmount the SD card filesystem */
esp_err_t sdcard_deinit(void);

#endif /* SDCARD_H */
