#include "nes_input.h"

#include "button.h"
#include "esp_log.h"
#include "thumbstick.h"

static const char *TAG = "nes_input";

/* Thumbstick thresholds — 40% deadzone around center (2048) */
#define STICK_HIGH (THUMBSTICK_CENTER + THUMBSTICK_THRESHOLD)  /* 2867 */
#define STICK_LOW  (THUMBSTICK_CENTER - THUMBSTICK_THRESHOLD)  /* 1229 */

esp_err_t nes_input_init(void)
{
    esp_err_t rc;

    rc = init_button(BTN_A);
    if (rc != ESP_OK) return rc;

    rc = init_button(BTN_B);
    if (rc != ESP_OK) return rc;

    rc = init_button(BTN_START);
    if (rc != ESP_OK) return rc;

    rc = init_button(BTN_SELECT);
    if (rc != ESP_OK) return rc;

    rc = thumbstick_init();
    if (rc != ESP_OK) return rc;

    ESP_LOGI(TAG, "NES input initialized");
    return ESP_OK;
}

uint8_t nes_input_read(void)
{
    uint8_t buttons = 0;

    /* GPIO buttons */
    if (button_is_pressed(BTN_A))      buttons |= NES_BTN_A;
    if (button_is_pressed(BTN_B))      buttons |= NES_BTN_B;
    if (button_is_pressed(BTN_START))  buttons |= NES_BTN_START;
    if (button_is_pressed(BTN_SELECT)) buttons |= NES_BTN_SELECT;

    /* Thumbstick → D-pad */
    uint32_t x = 0, y = 0;
    if (thumbstick_get_values(&x, &y) == ESP_OK) {
        if (y > STICK_HIGH) buttons |= NES_BTN_UP;
        if (y < STICK_LOW)  buttons |= NES_BTN_DOWN;
        if (x < STICK_LOW)  buttons |= NES_BTN_LEFT;
        if (x > STICK_HIGH) buttons |= NES_BTN_RIGHT;
    }

    return buttons;
}
