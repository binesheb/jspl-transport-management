#pragma once

// JSPL Transport Counter - HW-724 / ESP32-WROOM-32.
// Hardware baseline: HW-724 with integrated 0.96" 128x64 SSD1306 OLED.
// OLED is I2C: SDA=GPIO5, SCL=GPIO4, address=0x3C.

// Staff readiness / release buttons (active LOW with internal pull-up).
constexpr uint8_t PIN_KALOOR    = 25;
constexpr uint8_t PIN_VYTILLA   = 26;
constexpr uint8_t PIN_VAZHAKALA = 13;

// Optional buzzer. GPIO16 is exposed on the HW-724 and is reserved for the buzzer.
// Set to 255 to disable the buzzer.
constexpr uint8_t PIN_BUZZER = 16;

// Integrated 0.96" 128x64 SSD1306 OLED over I2C.
constexpr uint8_t OLED_SDA = 5;
constexpr uint8_t OLED_SCL = 4;
constexpr uint8_t OLED_ADDR = 0x3C;
constexpr uint8_t SCREEN_WIDTH = 128;
constexpr uint8_t SCREEN_HEIGHT = 64;

// Operational defaults.
constexpr uint32_t DEFAULT_LONG_PRESS_MS = 10000;
constexpr uint32_t DEFAULT_BUTTON_DEBOUNCE_MS = 35;
constexpr uint32_t DEFAULT_MESSAGE_MS = 1400;
constexpr uint32_t MAX_QUEUE_COUNT = 9999;

// Device defaults. Operational configuration is stored in ESP32 NVS.
constexpr char DEFAULT_DEVICE_ID[] = "PVM-GATE-01";
constexpr char DEFAULT_AP_NAME[] = "JSPL-PVM-GATE";
constexpr char DEFAULT_AP_PASSWORD[] = "jsplgate1";
