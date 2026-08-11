#pragma once

// Palarivattom V1: three hostel counter buttons.
// Change these pins if your ESP32 board uses different GPIOs.

// Buttons (active LOW with internal pull-up)
constexpr uint8_t PIN_KALOOR = 25;
constexpr uint8_t PIN_VYTILLA = 26;
constexpr uint8_t PIN_VAZHAKALA = 27;

// Optional buzzer. Set to 255 to disable.
constexpr uint8_t PIN_BUZZER = 32;

// 128x64 SPI OLED. Common VSPI wiring for a classic ESP32.
// OLED: VCC=3.3V, GND=GND, SCK=18, MOSI=23, CS=5, DC=16, RST=17.
constexpr uint8_t OLED_SCK = 18;
constexpr uint8_t OLED_MOSI = 23;
constexpr uint8_t OLED_CS = 5;
constexpr uint8_t OLED_DC = 16;
constexpr uint8_t OLED_RST = 17;
constexpr uint8_t SCREEN_WIDTH = 128;
constexpr uint8_t SCREEN_HEIGHT = 64;

constexpr uint32_t BUTTON_DEBOUNCE_MS = 35;
constexpr uint32_t LONG_PRESS_MS = 10000;
constexpr uint32_t UI_MESSAGE_MS = 1800;

constexpr char DEVICE_NAME[] = "PVM-GATE-01";
constexpr char AP_NAME[] = "JSPL-PVM-GATE";
constexpr char AP_PASSWORD[] = "jsplgate1";
