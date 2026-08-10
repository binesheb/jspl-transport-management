# JSPL Palarivattom V1 — Staff Availability Counter

This firmware is the first real hardware prototype for the Palarivattom showroom exit counter.

It runs **completely on one ESP32**. The ESP32 hosts the web server and can also create its own Wi-Fi access point, so no cloud server, Raspberry Pi, PC or Internet connection is required for the pilot.

## What this version does

Three hostel/destination buttons:

- **KAL** = Kaloor
- **VYT** = Vytilla
- **VAZ** = Vazhakala

### Normal mode

- Short press a hostel button → ready/waiting count **+1**.
- Hold a hostel button for **10 seconds** → enter release mode for that hostel.
- The 10-second action has a live OLED progress bar.

### Release mode

The selected hostel button now represents staff physically exiting the showroom.

- Short press → **Exited +1**.
- The waiting count is deliberately frozen while the group is being released.
- Hold for 10 seconds → confirm the physical exit count.
- Hold for another 10 seconds → confirm reduction of the waiting queue.
- The final action performs `waiting = waiting - exited` and returns to normal mode.

Example:

```text
KAL waiting = 90

Release mode
27 staff physically exit

Confirm

KAL waiting = 63
``` 

The bus controller is **not part of V1**. This device is only a reliable three-channel counter for staff available to board.

## OLED UI

The display is a **128x64 monochrome OLED**. The normal screen uses the display as efficiently as possible:

```text
PVM-GATE-01                         *

 KAL          VYT          VAZ
  90           12            8

          READY TO BOARD
```

During release mode it switches to a focused screen showing waiting and exited counts. During every 10-second hold it shows a filling progress bar and elapsed time.

## Hardware

- ESP32 DevKit V1 / compatible classic ESP32
- 128x64 SSD1306 **SPI** OLED
- 3 momentary push buttons
- Optional buzzer

### Pinout

The current configuration is in `src/config.h`.

| Function | GPIO |
|---|---:|
| KAL button | 25 |
| VYT button | 26 |
| VAZ button | 27 |
| Buzzer | 32 |
| OLED SCK | 18 |
| OLED MOSI | 23 |
| OLED CS | 5 |
| OLED DC | 16 |
| OLED RST | 17 |

OLED power:

- VCC → 3.3V
- GND → GND

Buttons use `INPUT_PULLUP`:

- One side → GPIO
- Other side → GND

### Important

The pin map assumes a **classic ESP32 DevKit / ESP32-WROOM style board** and an SPI SSD1306 module. If your ESP32 or OLED uses different pins/interface, change only `src/config.h`.

## Local web server

The ESP32 creates a temporary local Wi-Fi access point:

- SSID: `JSPL-PVM-GATE`
- Password: `jsplgate1`
- Web page: `http://192.168.4.1`

The web page is currently a **live monitor** of the physical counter. Physical buttons are the primary control for this V1.

## Persistent storage

Counts are stored using ESP32 `Preferences` (NVS), so the waiting counts survive a normal reboot/power cycle.

## Build

Open `firmware/esp32-minimal` in PlatformIO and upload to an ESP32 DevKit.

Then:

1. Open Serial Monitor at 115200 baud.
2. Connect a phone to `JSPL-PVM-GATE`.
3. Open `http://192.168.4.1`.
4. Test the three buttons.

## Current scope

This version intentionally does **not** include:

- Bus controllers
- Bus capacity enforcement
- Bus registration assignment
- Multi-trip management
- Cloud synchronization
- JSPL IoT integration

Those will be added only after the physical counter workflow is proven at Palarivattom.
