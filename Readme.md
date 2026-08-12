# JSPL Transport Management

Cloud-connected staff transport counting and boarding management for JSPL showrooms.

## Palarivattom pilot

- Showroom ID: `PVM`
- Showroom: Palarivattom
- Wi-Fi SSID: `JSPL IoT`
- Network model: Internet-only. ESP32 devices do not depend on device-to-device LAN communication.
- Development buses:
  - `KL-01-AB-1001`
  - `KL-01-AB-1002`
  - `KL-01-AB-1003`

The development registrations are placeholders. Production will use the actual vehicle registration numbers.

## Current hardware milestone

The first physical milestone is a **three-destination staff readiness counter running completely on one ESP32**.

```text
ESP32
├── 128×64 SPI OLED
├── KAL button
├── VYT button
├── VAZ button
├── optional buzzer
└── self-hosted web server
```

The bus controller is deliberately postponed until this counter workflow is proven physically.

## Arduino IDE — primary first-install method

The firmware can be opened directly in Arduino IDE:

`firmware/arduino/JSPL_PVM_Gate/JSPL_PVM_Gate.ino`

The project uses Espressif's official **Arduino-ESP32 core 3.3.11** for the pilot build.

Required libraries:

- Adafruit GFX Library 1.12.1+
- Adafruit SSD1306 2.5.15+
- Adafruit BusIO 1.17.2+

The Arduino sketch contains its own `partitions.csv` with two OTA application slots. This is important because the device must be capable of automatic firmware updates after the first USB installation.

## Automatic OTA

After the first USB upload, the device can update itself from GitHub.

```text
ESP32 boots
    ↓
Connect to configured JSPL IoT
    ↓
Check latest GitHub Release
    ↓
New release?
    ├── No → continue normally
    └── Yes
          ↓
      Download firmware
          ↓
      Verify SHA-256
          ↓
      Install inactive OTA slot
          ↓
      Reboot
          ↓
      New firmware
```

If Internet/GitHub is unavailable, the current firmware continues operating.

GitHub releases contain:

```text
firmware.bin
firmware.sha256
ota-version.txt
```

The device checks the latest release rather than a hard-coded version.

## Self-hosted device configuration

Every ESP32 provides its own local web interface. No external server is required to configure the prototype.

```text
http://192.168.4.1/
http://192.168.4.1/settings
http://192.168.4.1/network
```

Configuration includes:

- Device ID/name
- Showroom
- Installation point
- Bus registration number
- Bus capacity
- Three destination codes/names
- Long-press timing
- Debounce timing
- Admin PIN
- JSPL IoT Wi-Fi credentials

Configuration is stored in ESP32 NVS and survives reboot.

## Staff counter workflow

### Normal mode

A staff member presses the button for the destination they intend to board:

```text
Short press → +1 ready/waiting
```

Example:

```text
KAL 90
VYT 12
VAZ  8
```

### Release mode

The gatekeeper holds the destination button for 10 seconds. The OLED displays a live progress bar.

The selected button then counts staff physically exiting:

```text
WAITING 90
EXITED  27
```

The waiting count remains unchanged during the physical exit.

After the gatekeeper confirms the exit batch through the two-stage long-press confirmation, the queue is reduced:

```text
90 - 27 = 63
```

The device returns to normal mode.

This means a queue of 90 can be released in multiple bus loads without losing the remaining queue.

## System principles

1. The vehicle registration number is the permanent user-facing Bus ID.
2. Each showroom has a permanent short Showroom ID, such as `PVM`.
3. Every ESP32 connects only to `JSPL IoT`.
4. `ONLINE` does not automatically mean `READY FOR BOARDING`.
5. Bus devices must continue counting during temporary Internet loss.
6. Offline events must be stored locally and synchronized when connectivity returns.
7. Transport counts are derived from recorded events for auditability and correction.
8. Wi-Fi credentials, API keys and other secrets must never be committed to Git.
9. The gatekeeper decides when to release a queue; the system should enforce safety limits but not invent a minimum group size.

## Cloud architecture — later stage

```text
ESP32 Gate Nodes ----\
                      \
                       JSPL IoT → Internet → Cloud API / MQTT
                      /                         |          |
ESP32 Bus Nodes -----/                          |          +--> Dashboard
                                                +------------> PostgreSQL
```

Because `JSPL IoT` does not provide device-to-device communication, cloud synchronization is the coordination mechanism once the cloud-connected phase is enabled.

## Core entities

- **Showroom** — permanent location such as `PVM`.
- **Bus** — physical vehicle identified by registration number.
- **Device** — physical ESP32 assigned to a function or bus.
- **Trip** — a specific transport movement by a bus.
- **Boarding Session** — active period during which staff are assigned to a bus.
- **Transport Event** — immutable action such as staff ready, release, board, bus exit, departure or correction.

## Repository layout

```text
backend/       API, services and MQTT integration
dashboard/     Web control interface
firmware/      ESP32 firmware
docs/          Architecture, hardware and operating documentation
config/        Non-secret development configuration
deployment/    Docker and deployment configuration
```

## Build validation

GitHub Actions validates the Arduino IDE firmware using the same Espressif Arduino-ESP32 core and display library versions documented above.

PlatformIO remains available as a secondary engineering/release build environment, but **Arduino IDE is the straightforward first-install path for the physical pilot**.

## Production security gate

The prototype OTA path uses HTTPS and verifies the downloaded firmware SHA-256 before installation. Before fleet deployment, the OTA system must additionally use authenticated firmware/signing and certificate verification, with rollback/health confirmation.
