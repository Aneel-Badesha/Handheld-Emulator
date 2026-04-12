#include "nes_input.h"

#include "button.h"
#include "esp_log.h"
#include "thumbstick.h"

static const char *TAG = "nes_input";

/* Thumbstick mapping tuned for real hardware variance */
#define STICK_DEADZONE 450

static bool s_stick_calibrated = false;
static int32_t s_stick_center_x = THUMBSTICK_CENTER;
static int32_t s_stick_center_y = THUMBSTICK_CENTER;

static void s_calibrate_stick_center(uint32_t x, uint32_t y)
{
    s_stick_center_x = (int32_t)x;
    s_stick_center_y = (int32_t)y;
    s_stick_calibrated = true;
    ESP_LOGI(TAG, "thumbstick center calibrated: x=%ld y=%ld",
             (long)s_stick_center_x, (long)s_stick_center_y);
}

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
        if (!s_stick_calibrated) {
            s_calibrate_stick_center(x, y);
        }

        int32_t dx = (int32_t)x - s_stick_center_x;
        int32_t dy = (int32_t)y - s_stick_center_y;

        if (dy > STICK_DEADZONE)  buttons |= NES_BTN_UP;
        if (dy < -STICK_DEADZONE) buttons |= NES_BTN_DOWN;
        if (dx < -STICK_DEADZONE) buttons |= NES_BTN_LEFT;
        if (dx > STICK_DEADZONE)  buttons |= NES_BTN_RIGHT;
    }

    return buttons;
}
