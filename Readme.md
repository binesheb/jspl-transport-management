# JSPL Transport Management

Cloud-connected staff transport counting and boarding management for JSPL showrooms.

## Palarivattom pilot

- Showroom ID: `PVM`
- Showroom: Palarivattom
- Wi-Fi SSID: `JSPL IoT`
- Network model: Internet-only. ESP32 devices communicate through the cloud and do not require device-to-device LAN communication.
- Development buses:
  - `KL-01-AB-1001`
  - `KL-01-AB-1002`
  - `KL-01-AB-1003`

The development registrations are placeholders. Production will use the actual vehicle registration numbers.

## System principles

1. The vehicle registration number is the user-facing permanent Bus ID.
2. Each showroom has a permanent short Showroom ID, such as `PVM`.
3. Every ESP32 connects only to `JSPL IoT`.
4. `ONLINE` does not automatically mean `READY FOR BOARDING`.
5. Bus devices continue counting during temporary Internet loss.
6. Offline events are stored locally and synchronized when connectivity returns.
7. Transport counts are derived from recorded events for auditability and correction.
8. Wi-Fi credentials, API keys and other secrets must never be committed to Git.

## Core workflow

```text
Staff leaves showroom
        |
        v
Exit Counter +1 for selected bus
        |
        v
JSPL IoT -> Internet -> Cloud
        |
        v
Expected count increases
        |
        v
Staff enters bus
        |
        v
Bus Unit +1 boarded
        |
        v
Cloud updates live count
```

A bus becomes `READY TO DEPART` only when its active boarding session is complete and the expected and boarded counts match. Connectivity status and boarding status are separate concepts.

## Architecture

```text
ESP32 Exit Counter ----\\
                        \\
                         JSPL IoT -> Internet -> Cloud API / MQTT
                        /                         |          |
ESP32 Bus Units --------/                          |          +--> Web Dashboard
                                                  +------------> PostgreSQL
```

## Core entities

- **Showroom** — permanent location such as `PVM`.
- **Bus** — physical vehicle identified by registration number.
- **Device** — physical ESP32 assigned to a function or bus.
- **Trip** — a scheduled transport movement connecting a showroom, bus, route and date/time.
- **Boarding Session** — the active period during which staff are assigned to a bus.
- **Transport Event** — an immutable action such as `STAFF_EXIT`, `BOARD`, `BUS_EXIT` or a correction.

## Offline-first requirement

The bus ESP32 must remain usable if JSPL IoT or Internet connectivity temporarily fails. It keeps the current trip state and queues locally generated events. When connectivity returns, queued events are uploaded and acknowledged by the server. Duplicate delivery must not create duplicate passenger counts.

## Repository layout

```text
backend/       API, services and MQTT integration
database/      Schema and migrations
dashboard/     Web control interface
firmware/      ESP32 exit-counter and bus-unit firmware
docs/          Architecture, hardware and operating documentation
config/        Non-secret development configuration
deployment/    Docker and deployment configuration
```

## Arduino IDE build

The Arduino IDE entry point is:

`firmware/arduino/JSPL_PVM_Gate/JSPL_PVM_Gate.ino`

The Arduino sketch intentionally includes the same source used by the PlatformIO build, so there is one firmware implementation. The PlatformIO project declares the display libraries automatically, but Arduino IDE does not automatically read `platformio.ini` dependency declarations.

### Required Arduino libraries

Install these from **Arduino IDE → Library Manager** before compiling:

- **Adafruit GFX Library** — `1.12.1` or newer
- **Adafruit SSD1306** — `2.5.15` or newer
- **Adafruit BusIO** — installed automatically as a dependency of Adafruit GFX/SSD1306

The ESP32 core used by the current local build is **Espressif ESP32 3.3.11**.

If the compiler reports:

```text
fatal error: Adafruit_SSD1306.h: No such file or directory
```

install **Adafruit SSD1306** through Library Manager and compile again. This is a missing Arduino IDE library, not a firmware source-code error.

### Recommended build

For CI/release builds, use PlatformIO because `firmware/esp32-minimal/platformio.ini` already declares the required dependencies and the OTA partition configuration.

## OTA updates

The ESP32 firmware checks the configured GitHub Release on boot when network connectivity is available. A newer firmware is downloaded, verified and installed through the OTA partition before rebooting. If GitHub or the Internet is unavailable, the existing firmware continues operating normally.

## Development approach

The pilot will first be implemented as a software-simulated system so the event model, API, MQTT flow and dashboard can be tested before physical ESP32 hardware is introduced. The physical firmware will then use the same cloud contracts.

## Pilot success criteria

The first milestone is complete when we can simulate all three Palarivattom buses, start a trip, register staff leaving for each bus, register boarding, show expected/boarded/remaining counts live, handle duplicate/out-of-order events safely, and recover correctly from a simulated Internet outage.
