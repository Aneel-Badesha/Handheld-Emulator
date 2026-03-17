#include "button.h"
#include "ili9341.h"
#include "nes_input.h"
#include "nofrendo_hal.h"
#include "sdcard.h"
#include "thumbstick.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* ------------------------------------------------------------------ */
/*  Pin definitions                                                    */
/* ------------------------------------------------------------------ */

/* Buttons (active-low, internal pull-up) */
#define PIN_BTN_A      21
#define PIN_BTN_B      38
#define PIN_BTN_START  45
#define PIN_BTN_SELECT 46

/* Thumbstick ADC (ADC1) */
#define PIN_STICK_X 4   /* ADC1_CH3 */
#define PIN_STICK_Y 5   /* ADC1_CH4 */

/* ILI9341 display (SPI2 — bus shared with SD card) */
#define PIN_LCD_MOSI 11
#define PIN_LCD_MISO 13
#define PIN_LCD_SCLK 12
#define PIN_LCD_CS   10
#define PIN_LCD_DC    9
#define PIN_LCD_RST   8
#define PIN_LCD_BL    7

/* SD card (SPI2 — shares MOSI/MISO/SCLK with display) */
#define PIN_SD_CS 6

/* I2S reserved for future PCM5102A headphone audio — do not use */
#define PIN_I2S_BCLK 47
#define PIN_I2S_WCLK 48
#define PIN_I2S_DOUT 14

/* ------------------------------------------------------------------ */
/*  Task prototypes                                                    */
/* ------------------------------------------------------------------ */

static void emulator_task(void *arg);
static void display_task(void *arg);
static void input_task(void *arg);
static void sdcard_task(void *arg);

/* ------------------------------------------------------------------ */
/*  app_main                                                          */
/* ------------------------------------------------------------------ */

void app_main(void)
{
    /* Initialize hardware */
    ili9341_init();
    sdcard_init();
    nes_input_init();
    thumbstick_init();

    /* Spawn FreeRTOS tasks */
    xTaskCreatePinnedToCore(sdcard_task,   "sdcard",    4096, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(input_task,    "input",     4096, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(display_task,  "display",   8192, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(emulator_task, "emulator", 16384, NULL, 5, NULL, 0);
}

/* ------------------------------------------------------------------ */
/*  Task stubs — to be implemented                                    */
/* ------------------------------------------------------------------ */

static void sdcard_task(void *arg)
{
    /* TODO: list ROMs on SD card, load selected ROM into PSRAM */
    vTaskDelete(NULL);
}

static void input_task(void *arg)
{
    /* TODO: poll thumbstick + buttons, update shared NES input state */
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

static void display_task(void *arg)
{
    /* TODO: receive rendered frame buffers and DMA to ILI9341 */
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(16));
    }
}

static void emulator_task(void *arg)
{
    /* TODO: run NOFRENDO emulator loop — calls hal_display_frame / hal_input_poll */
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(16));
    }
}
