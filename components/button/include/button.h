#ifndef BUTTON_H
#define BUTTON_H

#include "driver/gpio.h"
#include "esp_err.h"
#include <stdbool.h>

/* Button GPIO pin assignments */
#define BTN_A      ((gpio_num_t)21)  /* Blue button */
#define BTN_B      ((gpio_num_t)38)  /* Yellow button */
#define BTN_START  ((gpio_num_t)45)
#define BTN_SELECT ((gpio_num_t)46)

/* Configure a GPIO pin as an active-low input with internal pull-up */
esp_err_t init_button(gpio_num_t gpio_num);

/* Return true if the button is currently pressed (pin reads low) */
bool button_is_pressed(gpio_num_t gpio_num);

#endif /* BUTTON_H */
