# ESP32 Minimal Transport Counter

This is the first hardware prototype of JSPL Transport Management.

It is intentionally **100% standalone on one ESP32**. There is no backend, database, MQTT broker or Internet dependency.

## What it demonstrates

- Expected staff count
- Boarded staff count
- Remaining staff count
- Staff exits from the bus
- Ready-to-depart state
- Physical push buttons
- 128x64 SSD1306 OLED display
- Local web interface hosted by the ESP32
- Counts retained across ESP32 reboot using Preferences

## Hardware

- ESP32 DevKit V1 / compatible ESP32 board
- 0.96-inch SSD1306 I2C OLED, address `0x3C`
- 4 momentary push buttons

### Pinout

| Function | ESP32 GPIO |
|---|---:|
| OLED SDA | 21 |
| OLED SCL | 22 |
| Staff left / Expected +1 | 25 |
| Boarded +1 | 26 |
| Exit bus +1 | 27 |
| Reset trip | 32 |

Buttons use `INPUT_PULLUP`: connect one side of each button to the GPIO and the other side to GND.

## Standalone Wi-Fi

The ESP32 starts its own temporary Wi-Fi access point for this prototype:

- SSID: `JSPL-BUS-XXXX`
- Password: `jsplbus1`
- Web page: `http://192.168.4.1`

`XXXX` is derived from the end of the configured development bus registration.

This AP is only for the prototype. The production version will connect to `JSPL IoT` and use the cloud architecture defined in the main application.

## Build

Open `firmware/esp32-minimal` in PlatformIO and upload to an ESP32 DevKit.

Then open the serial monitor at 115200 baud. Connect a phone/laptop to the ESP32 Wi-Fi and open `http://192.168.4.1`.

## Counting rules

- `STAFF LEFT +1` increases Expected.
- `BOARD +1` increases Boarded only while Boarded < Expected.
- `EXIT BUS +1` increases Exited only while Exited < Boarded.
- `RESET TRIP` clears all counts.
- When Expected > 0 and Boarded == Expected, status becomes `READY`.

## Why this prototype comes first

Before adding cloud communication, we want to prove the physical interaction and counting rules on real hardware. Once this works, the same state machine can be connected to the cloud and split across the showroom exit device and bus device.
