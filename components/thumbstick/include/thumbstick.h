#ifndef THUMBSTICK_H
#define THUMBSTICK_H

#include "esp_err.h"
#include <stdint.h>

/* ADC pin assignments — ADC1 channels on ESP32-S3 */
#define THUMBSTICK_PIN_X 4  /* ADC1_CH3 */
#define THUMBSTICK_PIN_Y 5  /* ADC1_CH4 */

/* Raw ADC range: 0–4095. Center ~2048. */
#define THUMBSTICK_CENTER    2048
#define THUMBSTICK_THRESHOLD  819  /* 40% of 2048 — deadzone boundary */

/* Initialize thumbstick ADC and start internal background read task */
esp_err_t thumbstick_init(void);

/* Copy the latest X and Y raw ADC values (0–4095) into the provided pointers */
esp_err_t thumbstick_get_values(uint32_t *x_out, uint32_t *y_out);

/* Stop the background task and release ADC resources */
esp_err_t thumbstick_deinit(void);

#endif /* THUMBSTICK_H */
