# JSPL Palarivattom Gate — Arduino IDE Firmware

This is the **primary USB/first-install entry point** for the Palarivattom ESP32 counter.

The project uses Espressif's official **Arduino-ESP32 core**. The CI build is pinned to the same core version used for the pilot validation: **3.3.11**.

## Open this file

Open exactly:

`JSPL_PVM_Gate.ino`

Do not open `main.cpp` directly.

The `.ino` entry point reuses the common firmware implementation from `firmware/esp32-minimal/src`, so Arduino IDE and PlatformIO remain on the same source of truth.

## Arduino IDE board

Select:

**ESP32 Dev Module**

This is intended for the classic ESP32 / ESP32-WROOM class board used for the V1 prototype.

Recommended:

- Board: `ESP32 Dev Module`
- Upload speed: `115200`
- Flash frequency: `80 MHz`
- Flash mode: board default
- Port: the COM port of the ESP32

### OTA partitioning

This sketch contains its own `partitions.csv` so the Arduino IDE build uses a dual-OTA application layout:

```text
ota_0
ota_1
otadata
```

Do **not** select a factory-only partition scheme for the OTA-capable firmware.

The partition table is sized for the 4 MB classic ESP32 target used by the pilot.

## Required Arduino libraries

Install these from **Arduino IDE → Library Manager**:

- **Adafruit GFX Library** — 1.12.1 or newer
- **Adafruit SSD1306** — 2.5.15 or newer
- **Adafruit BusIO** — 1.17.2 or newer

The Espressif Arduino-ESP32 core provides the ESP32-specific APIs used by the firmware, including:

- `WiFi`
- `WebServer`
- `Preferences`
- `HTTPClient`
- `WiFiClientSecure`
- `Update`

The OLED driver is **not part of the Espressif core**, so Adafruit SSD1306/GFX remain required.

## Hardware

The current prototype uses:

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

Buttons are active LOW:

```text
GPIO ── button ── GND
```

OLED:

```text
VCC → 3.3V
GND → GND
```

Verify the exact OLED module before wiring; this firmware assumes an SPI SSD1306 module.

## First USB installation

1. Install the Espressif `arduino-esp32` board package.
2. Select **ESP32 Dev Module**.
3. Install the three required Adafruit libraries.
4. Open `JSPL_PVM_Gate.ino`.
5. Compile.
6. Upload over USB.
7. Open Serial Monitor at `115200`.

After the first successful USB installation, future releases can be installed automatically by OTA.

## Automatic OTA

The device checks GitHub for a newer firmware release **on every boot**, provided a working Internet Wi-Fi configuration is stored.

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

If Wi-Fi or GitHub is unavailable, OTA is skipped and the **existing firmware continues operating**. The counter must not depend on Internet availability merely to start.

The release assets are:

```text
firmware.bin
firmware.sha256
ota-version.txt
```

The device uses GitHub's `releases/latest/download/...` asset URLs, so the firmware does not need a hard-coded release number.

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

Settings include:

- Device ID
- Device name
- Showroom
- Installation point
- Bus registration number
- Bus capacity
- Destination names/codes
- Long-press duration
- Debounce
- Display message duration
- Administrator PIN
- JSPL IoT Wi-Fi credentials

Configuration is stored in ESP32 NVS.

**Change the development administrator PIN before operational deployment.**

## Normal operation

Short press:

```text
KAL → waiting +1
VYT → waiting +1
VAZ → waiting +1
```

10-second hold:

```text
NORMAL
  ↓
RELEASE MODE
```

The OLED displays a filling progress bar during the hold.

In release mode, short presses count staff physically exiting the showroom. The waiting count remains frozen until the gatekeeper completes the confirmation sequence.

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

This allows a queue of 90 to release 27, leaving 63 waiting, rather than incorrectly resetting the whole queue.

## GitHub build validation

The repository has an Arduino CLI workflow that:

- installs Espressif Arduino-ESP32 **3.3.11**
- installs the exact display libraries
- compiles the `.ino`
- exports the compiled binary
- stores the build output as a GitHub Actions artifact

This gives us a repeatable check that the file you open in Arduino IDE remains buildable.

## Production security note

The prototype OTA path currently uses HTTPS transport and verifies the firmware SHA-256 before installation. SHA-256 detects corruption but does **not** authenticate the publisher. Before fleet deployment, add certificate verification and cryptographic firmware signing/secure boot as a production security layer.
