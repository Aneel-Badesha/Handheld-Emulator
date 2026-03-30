# Handheld NES Emulator

Handheld NES emulator running on ESP32-S3 with 8 MB PSRAM, ILI9341 320×240 display, SD card ROM storage, analog thumbstick, and 4 buttons. Built on ESP-IDF 5.x and FreeRTOS. NES emulation via [NOFRENDO](https://github.com/nwagyu/nofrendo).

---

## Hardware

| Component | Part | Notes |
|-----------|------|-------|
| MCU | ESP32-S3 | Dual-core LX7 @ 240 MHz, 8 MB Octal PSRAM |
| Display | ILI9341 2.4" TFT | 320×240, RGB565, SPI2 @ 40 MHz |
| SD card | ILI9341 onboard slot | FatFS, SPI2 @ 4 MHz (shared bus) |
| Thumbstick | Analog XY | ADC1 GPIO 4 (X), GPIO 5 (Y) |
| Button A | Blue | GPIO 21, active-low |
| Button B | Yellow | GPIO 38, active-low |
| Button Start | | GPIO 45, active-low |
| Button Select | | GPIO 46, active-low |
| Audio (future) | PCM5102A + 3.5mm jack | I2S: BCLK=47, WCLK=48, DOUT=14 |

---

## Pin Assignments

```
ILI9341 (SPI2)          SD Card (SPI2, shared)
  MOSI  GPIO 11           MOSI  GPIO 11
  MISO  GPIO 13           MISO  GPIO 13
  SCLK  GPIO 12           SCLK  GPIO 12
  CS    GPIO 10           CS    GPIO  6
  DC    GPIO  9
  RST   GPIO  8
  BL    GPIO  7 (PWM)

Buttons                 Thumbstick (ADC1)
  A      GPIO 21          X  GPIO 4 (ADC1_CH3)
  B      GPIO 38          Y  GPIO 5 (ADC1_CH4)
  Start  GPIO 45
  Select GPIO 46

I2S (reserved — do not use for other functions)
  BCLK  GPIO 47
  WCLK  GPIO 48
  DOUT  GPIO 14
```

> **Avoid:** GPIO 19–20 (USB D+/D-) and GPIO 26–37 (Octal PSRAM interface).

---

## Project Structure

```
Handheld-Emulator/
├── CMakeLists.txt          — ESP-IDF project root
├── Makefile                — Convenience build/flash targets
├── sdkconfig.defaults      — ESP32-S3 config overrides (WiFi/BT off, PSRAM on)
├── design.txt              — Full design document (decisions, tradeoffs, problems)
├── main/
│   ├── CMakeLists.txt
│   └── main.c              — FreeRTOS task setup
└── components/
    ├── ili9341/            — ILI9341 display driver stub (SPI2 bus owner)
    ├── sdcard/             — SD card + FatFS wrapper (adds device to SPI2)
    ├── button/             — GPIO button driver
    ├── thumbstick/         — ADC continuous thumbstick driver
    ├── nes_input/          — Maps hardware to NES controller bitmask
    ├── nofrendo/           — NOFRENDO emulator core (git submodule)
    └── nofrendo_hal/       — HAL bridge: display/input/audio callbacks for NOFRENDO
```

---

## Building

Requires [ESP-IDF 5.x](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/get-started/).

```bash
# Clone with submodules
git clone --recurse-submodules <repo-url>
cd Handheld-Emulator

# Set target and generate sdkconfig
idf.py set-target esp32s3

# Build
idf.py build

# Flash and monitor
idf.py -p COM3 flash monitor
```

Or use the Makefile shortcuts:

```bash
make build
make flash          # uses COM3 by default
make flash-monitor
make menuconfig
make clean
```

---

## NES Input Mapping

| Hardware | NES Button |
|----------|-----------|
| Thumbstick Up | D-Pad Up |
| Thumbstick Down | D-Pad Down |
| Thumbstick Left | D-Pad Left |
| Thumbstick Right | D-Pad Right |
| Button A (blue) | A |
| Button B (yellow) | B |
| Button Start | Start |
| Button Select | Select |

---

## Architecture

Two cores run in parallel for 60 FPS:

- **Core 0:** NES emulator (NOFRENDO) + input polling
- **Core 1:** ILI9341 display DMA + SD card task 

Frame buffers (double-buffered in PSRAM): Core 0 renders into buffer B while Core 1 DMA-transfers buffer A to the display.

See [design.txt](design.txt) for full design documentation including memory layout, tradeoffs, and known challenges.
