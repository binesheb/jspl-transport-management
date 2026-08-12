#pragma once

// JSPL Transport Counter - HW-724 / ESP32-WROOM-32
// Integrated OLED: SSD1306 128x64, I2C address 0x3C.
// Verified published HW-724 references identify OLED SDA=GPIO5 and SCL=GPIO4.

constexpr uint8_t PIN_KALOOR   = 25;
constexpr uint8_t PIN_VYTILLA  = 26;
constexpr uint8_t PIN_VAZHAKALA = 27;
constexpr uint8_t PIN_BUZZER   = 32;

constexpr uint8_t OLED_SDA = 5;
constexpr uint8_t OLED_SCL = 4;
constexpr uint8_t OLED_ADDR = 0x3C;
constexpr uint8_t SCREEN_WIDTH = 128;
constexpr uint8_t SCREEN_HEIGHT = 64;

constexpr uint32_t DEFAULT_LONG_PRESS_MS = 10000;
constexpr uint32_t DEFAULT_BUTTON_DEBOUNCE_MS = 35;
constexpr uint32_t DEFAULT_MESSAGE_MS = 1400;
constexpr uint32_t MAX_QUEUE_COUNT = 9999;

constexpr char DEFAULT_DEVICE_ID[] = "PVM-GATE-01";
constexpr char DEFAULT_AP_NAME[] = "JSPL-PVM-GATE";
constexpr char DEFAULT_AP_PASSWORD[] = "jsplgate1";
