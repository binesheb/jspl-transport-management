# JSPL Palarivattom Gate — Arduino IDE Firmware

This is the **primary USB/first-install entry point** for the Palarivattom ESP32 counter.

The project uses Espressif's official **Arduino-ESP32 core**. The CI build is pinned to the same core version used for pilot validation: **3.3.11**.

## Open this file

Open exactly:

`JSPL_PVM_Gate.ino`

Do not open `main.cpp` directly.

The `.ino` entry point reuses the common firmware implementation from `firmware/esp32-minimal/src`, so Arduino IDE and PlatformIO remain on the same source of truth.

## Arduino IDE board

Select:

**ESP32 Dev Module**

Recommended:

- Board: `ESP32 Dev Module`
- Upload speed: `115200`
- Flash frequency: `80 MHz`
- Flash mode: board default
- Port: the COM port of the ESP32

## OTA partitioning

This sketch contains its own `partitions.csv` so the Arduino IDE build uses a dual-OTA application layout. Do not select a factory-only partition scheme for OTA-capable firmware.

## Required Arduino libraries

The firmware **cannot compile without Adafruit SSD1306** because the common source directly includes `Adafruit_SSD1306.h`.

Install these from **Arduino IDE → Library Manager**:

- **Adafruit GFX Library** — 1.12.6
- **Adafruit SSD1306** — 2.5.15
- **Adafruit BusIO** — 1.17.4

### Quick fix from Command Prompt

From the repository's `firmware/arduino` folder, run:

```bat
install-libraries.bat
```

Or install the missing library directly:

```bat
arduino-cli lib install "Adafruit SSD1306@2.5.15"
```

If Arduino IDE reports:

```text
fatal error: Adafruit_SSD1306.h: No such file or directory
```

**do not change the firmware source**. It means the Adafruit SSD1306 library is not installed in the Arduino IDE library search path. Install the library and compile again.

The Espressif Arduino-ESP32 core provides the ESP32-specific APIs used by the firmware, including WiFi, WebServer and Preferences. The OLED driver is external and must be installed separately.

## Hardware

Current prototype:

- Classic ESP32 DevKit
- 128×64 monochrome SSD1306 **SPI** OLED
- 3 momentary push buttons
- Optional active buzzer

Current pin map is kept in `firmware/esp32-minimal/src/config.h`.

| Function | GPIO |
|---|---:|
| KAL / Kaloor | 25 |
| VYT / Vytilla | 26 |
| VAZ / Vazhakala | 27 |
| Buzzer | 32 |
| OLED SCK | 18 |
| OLED MOSI | 23 |
| OLED CS | 5 |
| OLED DC | 16 |
| OLED RST | 17 |

## First USB installation

1. Install the Espressif `arduino-esp32` board package 3.3.11.
2. Select **ESP32 Dev Module**.
3. Install the three required Adafruit libraries, especially **Adafruit SSD1306**.
4. Open `JSPL_PVM_Gate.ino`.
5. Compile.
6. Upload over USB.
7. Open Serial Monitor at `115200`.

After the first successful USB installation, future releases can be installed automatically by OTA.

## Automatic OTA

The device checks GitHub for a newer firmware release on every boot when a working Internet Wi-Fi configuration is stored.

```text
BOOT
  ↓
Start local counter
  ↓
Connect to configured JSPL IoT
  ↓
Check latest GitHub Release
  ↓
Same version? ── YES ──→ NORMAL OPERATION
  │
  NO
  ↓
Download release checksum
  ↓
Download firmware.bin
  ↓
SHA-256 verification
  ↓
Write inactive OTA partition
  ↓
Validate image
  ↓
REBOOT
  ↓
NEW FIRMWARE
```

If Wi-Fi or GitHub is unavailable, OTA is skipped and the existing firmware continues operating.

## Self-hosted configuration

The ESP32 hosts its own local web interface.

Default development access point:

```text
SSID:      JSPL-PVM-GATE
Password:  jsplgate1
Address:   http://192.168.4.1
```

Pages:

```text
/
/settings
/network
```

Configuration is stored in ESP32 NVS.

## Normal operation

Short press:

```text
KAL → waiting +1
VYT → waiting +1
VAZ → waiting +1
```

10-second hold enters release mode.

## Release confirmation

```text
NORMAL
  ↓ 10s hold
RELEASE MODE
  ↓ staff press button
EXITED +1
  ↓ 10s hold
CONFIRM EXIT
  ↓ 10s hold
CONFIRM REDUCE
  ↓
WAITING = WAITING - EXITED
  ↓
NORMAL
```

## GitHub build validation

The repository has an Arduino CLI workflow that installs the same ESP32 core and required display libraries before compiling the `.ino`. This prevents a missing SSD1306 dependency from being mistaken for a firmware source error.

## Production security note

The prototype OTA path currently uses HTTPS transport and verifies the firmware SHA-256 before installation. SHA-256 detects corruption but does not authenticate the publisher. Before fleet deployment, add cryptographic firmware signing/secure boot as a production security layer.
