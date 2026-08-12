# JSPL Palarivattom V1 — Staff Availability Counter

This firmware is the first real hardware prototype for the Palarivattom showroom exit counter.

It runs **completely on one ESP32**. The ESP32 hosts the web server and can create its own Wi-Fi access point, so no cloud server, Raspberry Pi, PC or Internet connection is required for the pilot counter workflow.

## Actual pilot hardware

The Palarivattom pilot hardware is fixed to the **HW-724 / ESP32-WROOM-32** board shown in the project hardware documentation.

- MCU: ESP32-WROOM-32
- Board: HW-724
- PlatformIO / Arduino target: `esp32dev` / ESP32 Dev Module
- Integrated OLED: 0.96" 128x64 SSD1306
- OLED interface: I2C
- OLED SDA: GPIO 5
- OLED SCL: GPIO 4
- OLED address: `0x3C`
- USB: Micro-USB
- USB-UART: CP2102

## What this version does

Three configurable destination buttons:

- **KAL** = Kaloor
- **VYT** = Vytilla
- **VAZ** = Vazhakala

### Normal mode

- Short press a destination button → ready/waiting count **+1**.
- Hold a destination button for the configured long-press duration (default 10 seconds) → enter release mode for that destination.
- The hold action has a live OLED progress bar.

### Release mode

The selected destination button now represents staff physically exiting the showroom.

- Short press → **Exited +1**.
- The waiting count is deliberately frozen while the group is being released.
- Hold for the configured long-press duration → confirm the physical exit count.
- Hold again → confirm reduction of the waiting queue.
- The final action performs the queue reduction and returns to normal mode.

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

The display is the integrated **128x64 monochrome SSD1306 I2C OLED**.

```text
PVM-GATE-01

 KAL          VYT          VAZ
  90           12            8

          READY TO BOARD
```

During release mode it switches to a focused screen showing waiting and exited counts. During every long hold it shows a filling progress bar and elapsed time.

## Hardware

- HW-724 / ESP32-WROOM-32
- Integrated 0.96" 128x64 SSD1306 I2C OLED
- 3 momentary push buttons
- Optional active buzzer

### Pinout

| Function | GPIO |
|---|---:|
| KAL button | 25 |
| VYT button | 26 |
| VAZ button | 13 |
| Buzzer | 16 |
| OLED SDA | 5 |
| OLED SCL | 4 |
| OLED I2C address | `0x3C` |

### Button wiring

Buttons use `INPUT_PULLUP`:

```text
GPIO 25 ── KAL button ── GND
GPIO 26 ── VYT button ── GND
GPIO 13 ── VAZ button ── GND
```

No external pull-up resistor is required.

### OLED

The OLED is integrated into the HW-724 board:

```text
GPIO 5 ── SDA
GPIO 4 ── SCL
3V3    ── VCC
GND    ── GND
```

### Buzzer

For the pilot hardware:

```text
GPIO 16 ── active buzzer signal / +
GND     ── buzzer −
```

Use an appropriate transistor/driver if the selected buzzer requires more current than an ESP32 GPIO should supply directly.

### Circuit diagram

See the repository diagram:

`docs/hardware/circuit-diagram.svg`

The diagram is based on the actual HW-724 pinout supplied for this pilot. It replaces the earlier generic SPI-OLED circuit.

## Local web server

The ESP32 creates a local Wi-Fi access point:

- SSID: `JSPL-PVM-GATE`
- Password: `jsplgate1`
- Dashboard: `http://192.168.4.1`
- Settings: `http://192.168.4.1/settings`
- Network / OTA: `http://192.168.4.1/network`

The dashboard is a live monitor of the physical counter. Physical buttons remain the primary operational controls for V1.

## Device settings

The ESP32 has its own configuration page. No computer or external server is required.

The settings page supports:

### Device

- Device ID
- Device name
- Showroom
- Installation point

### Bus preparation

The bus controller is not active in V1, but reusable device configuration already supports:

- Vehicle registration number
- Bus capacity
- Bus enabled/disabled

The final project will use the **vehicle registration number as the bus identifier**, while the software model keeps destination/route separate from a physical bus assignment.

### Destinations

Each of the three physical buttons can be configured without recompiling the firmware:

- Button 1 code/name
- Button 2 code/name
- Button 3 code/name

Default configuration:

```text
Button 1 → KAL / Kaloor
Button 2 → VYT / Vytilla
Button 3 → VAZ / Vazhakala
```

### Operation

The following parameters are stored in the ESP32:

- Long-press duration — default 10,000 ms
- Button debounce — default 35 ms
- OLED message duration — default 1,400 ms

### Security

Settings changes require the local administrator PIN.

Default development PIN:

```text
1234
```

**Change this before operational deployment.**

The configuration is stored in ESP32 NVS using `Preferences`, so it survives a normal reboot.

## Network and OTA

The device can optionally connect to a configured JSPL IoT Wi-Fi network while continuing to host its local access point.

Configure it from:

```text
http://192.168.4.1/network
```

The page supports:

- Wi-Fi SSID
- Wi-Fi password
- Administrator authentication
- Manual OTA check

Credentials are stored locally in ESP32 NVS and are not committed to GitHub.

### Automatic firmware update flow

When Internet Wi-Fi is configured, the firmware checks GitHub Releases after boot:

```text
ESP32 boots
   ↓
Connect to JSPL IoT
   ↓
Read latest release version
   ↓
Compare with installed version
   ↓
New version?
   ↓
Download checksum
   ↓
Download firmware
   ↓
Calculate SHA-256 while writing OTA partition
   ↓
Checksum matches?
   ↓
Validate OTA image
   ↓
Reboot into new firmware
```

The release pipeline publishes firmware and checksum assets for OTA distribution.

**Current security status:** HTTPS is used for the prototype, but certificate verification is intentionally not yet enabled. SHA-256 protects against corrupted or incomplete firmware installation; it is **not a substitute for authenticated firmware signing**. Before production rollout, add certificate verification and cryptographic firmware signing.

## Persistent counter storage

Counts are stored using ESP32 `Preferences` (NVS), so waiting counts survive a normal reboot/power cycle.

Factory reset from the settings page resets **configuration only**. It does not silently erase the staff counters.

For the production architecture, confirmed transport events should additionally have unique IDs, timestamps, device IDs and durable event records so they can synchronize safely after an offline period.

## Build

Open `firmware/esp32-minimal` in PlatformIO and select the ESP32 Dev Module / `esp32dev` target.

Then:

1. Connect the HW-724 using Micro-USB.
2. Open Serial Monitor at 115200 baud.
3. Connect a phone to `JSPL-PVM-GATE`.
4. Open `http://192.168.4.1`.
5. Open `/settings` and change the development PIN.
6. Open `/network` and configure JSPL IoT only if OTA testing is required.
7. Test the three physical buttons.
8. Verify the OLED displays correctly.
9. Power-cycle the ESP32 and verify the counts remain intact.

## Release process

Firmware releases are created from semantic version tags:

```text
v1.0.1
v1.0.2
v1.1.0
```

GitHub Actions builds the PlatformIO firmware and publishes the release assets automatically.

Do **not** create a production release until the physical Palarivattom pilot has passed the hardware and workflow checklist.

## Current scope

This version intentionally does **not** include:

- Bus controllers
- Bus capacity enforcement at the gate
- Physical bus boarding counters
- Multi-trip management
- Cloud synchronization of transport events
- JSPL IoT application integration
- Authenticated/signed OTA firmware
- Staff identity/RFID tracking

These will be added after the physical counter workflow is proven at Palarivattom.

## Pilot gate before custom PCB

Validate physically before freezing reusable hardware:

- destination button reliability
- duplicate-press behaviour
- queue accuracy
- gatekeeper release workflow
- bus readiness state
- capacity protection
- released-versus-boarded reconciliation
- departure workflow
- Wi-Fi reconnect
- Internet loss and recovery
- power-cycle recovery
- cloud synchronization after offline operation
- display readability
- button ergonomics
- enclosure and power reliability
