#ifndef NES_INPUT_H
#define NES_INPUT_H

#include "esp_err.h"
#include <stdint.h>

/* NES controller button bitmask (standard bit ordering) */
#define NES_BTN_A      (1 << 0)
#define NES_BTN_B      (1 << 1)
#define NES_BTN_SELECT (1 << 2)
#define NES_BTN_START  (1 << 3)
#define NES_BTN_UP     (1 << 4)
#define NES_BTN_DOWN   (1 << 5)
#define NES_BTN_LEFT   (1 << 6)
#define NES_BTN_RIGHT  (1 << 7)

/*
 * Initialize NES input — configures all 4 GPIO buttons and thumbstick ADC.
 * Call after ili9341_init() (so SPI2 bus exists for sdcard to share).
 */
esp_err_t nes_input_init(void);

/*
 * Read the current input state and return a bitmask of pressed NES buttons.
 * Thumbstick deflection beyond the deadzone is treated as a D-pad press.
 * Thread-safe — may be called from any task.
 */
uint8_t nes_input_read(void);

#endif /* NES_INPUT_H */
