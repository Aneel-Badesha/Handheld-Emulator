#ifndef SDCARD_H
#define SDCARD_H

#include "esp_err.h"

/* Initialize the SPI3 bus for the SD card (MOSI=15, MISO=16, SCLK=17, CS=18) */
esp_err_t sdcard_bus_init(void);

/* Mount the SD card filesystem — call after sdcard_bus_init() */
esp_err_t sdcard_init(void);

/* Read first line of a file and log it */
esp_err_t sdcard_read_file(const char *path);

/* Write data to a file, overwriting any existing content */
esp_err_t sdcard_write_file(const char *path, const char *data);

/* Unmount the SD card filesystem */
esp_err_t sdcard_deinit(void);

#endif /* SDCARD_H */
