# Arduino IDE Build

This folder is the Arduino IDE entry point for the JSPL Palarivattom V1 counter.

## Open in Arduino IDE

Open:

`JSPL_PVM_Gate.ino`

Do not open `main.cpp` directly.

The `.ino` wrapper includes the existing firmware source from `firmware/esp32-minimal/src`, so the Arduino IDE and PlatformIO builds use the same implementation.

## Arduino IDE settings

- Board: **ESP32 Dev Module** (classic ESP32 / ESP32-WROOM compatible)
- Upload Speed: 115200 or higher if stable
- Flash Frequency: 80 MHz
- Partition Scheme: use a partition scheme with OTA support; the PlatformIO build uses the project's custom `partitions_ota.csv`.
- Port: select the COM port of the ESP32

## Libraries

Install from Arduino IDE Library Manager:

- Adafruit GFX Library
- Adafruit SSD1306

The ESP32 Arduino core provides the remaining ESP32 libraries used by the firmware, including WiFi, WebServer, Preferences, HTTPClient and Update.

## Important

The Arduino IDE wrapper intentionally reuses the PlatformIO source files. This prevents two different firmware implementations from drifting apart.

For the exact production/pilot build, use the PlatformIO project because it explicitly selects `partitions_ota.csv` and is also used by the GitHub release pipeline.
