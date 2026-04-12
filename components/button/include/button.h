#ifndef BUTTON_H
#define BUTTON_H

#include "driver/gpio.h"
#include "esp_err.h"
#include <stdbool.h>

/* Button GPIO pin assignments */
#define BTN_A      ((gpio_num_t)4)
#define BTN_B      ((gpio_num_t)5)
#define BTN_START  ((gpio_num_t)6)
#define BTN_SELECT ((gpio_num_t)7)

/* Configure a GPIO pin as an active-low input with internal pull-up */
esp_err_t init_button(gpio_num_t gpio_num);

/* Return true if the button is currently pressed (pin reads low) */
bool button_is_pressed(gpio_num_t gpio_num);

#endif /* BUTTON_H */
