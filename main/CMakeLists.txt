idf_component_register(SRCS "main.c"
                       INCLUDE_DIRS "."
                       REQUIRES button thumbstick sdcard ili9341 nes_input nofrendo_hal freertos esp_timer)
