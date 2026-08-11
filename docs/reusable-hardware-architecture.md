# JSPL Transport System — Reusable Hardware Architecture

## 1. Design goal

The production hardware must be reusable across every JSPL showroom and every bus. Do not build a one-off Palarivattom circuit.

The preferred architecture is a **common JSPL Transport Node** based on ESP32, with a role selected by configuration rather than by changing the PCB.

Roles:

- `GATE` — staff destination/ready input and gatekeeper control
- `BUS` — bus status, released/boarded counting and departure control
- `DISPLAY` — optional information-only display node in future

The same controller PCB can be used for all roles. Only the connected peripherals and firmware configuration differ.

## 2. Network principle

Every node connects to the existing `JSPL IoT` SSID.

`JSPL IoT` is treated as an Internet-only network. Nodes must never depend on direct LAN communication, broadcast discovery, mDNS, or fixed local IP addresses.

Connected architecture:

```text
                 JSPL IoT
              Internet-only Wi-Fi
                     |
        +------------+-------------+
        |            |             |
   GATE NODE      BUS NODE      BUS NODE ...
        |            |             |
        +------------+-------------+
                     |
                 Cloud API
```

The cloud is the synchronization point between nodes. If Internet connectivity is temporarily lost, each node continues its local operation and queues events for synchronization.

## 3. Universal controller board

### Recommended controller

Use an **ESP32-S3 module/dev board** for the production-generation controller. The initial prototype may continue using a conventional ESP32 DevKit for cost and speed.

The application should not depend on a particular ESP32 board pinout. Put all GPIO assignments in a hardware configuration layer.

### Core controller features

- ESP32-S3 Wi-Fi/BLE MCU
- USB for programming and diagnostics
- Status RGB LED or separate status LEDs
- Hardware reset button
- Boot/program button
- Non-volatile storage for device identity and offline state
- Watchdog enabled
- Secure configuration storage
- Local event queue in flash
- Optional RTC connector or time synchronization through NTP/cloud

## 4. Power architecture

The common PCB should accept a practical field supply rather than requiring a USB charger.

Preferred:

```text
12 V DC input
      |
      +---- fuse / reverse-polarity protection
      |
      +---- surge / transient protection
      |
      +---- 5 V buck converter
               |
               +---- ESP32 / display / peripherals
```

The exact input range and protection components must be finalized during the electrical design stage. The board should not assume that every installation has the same power source.

For the first pilot, a quality 5 V USB supply or regulated 5 V adapter is acceptable.

## 5. Common peripheral interfaces

Expose standardized connectors so the same PCB can be reused:

- I2C — OLED/display/sensors
- GPIO — push buttons, switches, LEDs
- UART — service/debug and future peripherals
- RS-485 — optional future long-cable peripherals
- Buzzer output
- Digital output — optional gate indicator/relay interface
- Digital input — optional door/limit/status sensor
- Service USB

Do not directly connect mains voltage to the ESP32 PCB. If a future gate lock, lamp, or other actuator is required, use an appropriately rated isolated relay/contactor interface installed by a qualified technician.

## 6. GATE node

The Gate node is the primary showroom control device.

### Inputs

- One destination/ready button per active bus
- Gatekeeper buttons/touch controls for release and operational actions
- Optional physical emergency/override input

### Outputs

- Display
- Buzzer
- Status indicators
- Optional non-safety-critical "PROCEED TO BUS" indicator

### Minimum Palarivattom configuration

```text
GATE-PVM-01

BUS 1 READY button
BUS 2 READY button
BUS 3 READY button

Gatekeeper control:
- select bus
- release queue
- close boarding
- depart/complete
- corrections/admin actions
```

The staff buttons and gatekeeper interface may be physically combined for the pilot, but the software must keep their roles separate.

## 7. BUS node

One identical Bus node is installed on each vehicle.

The node is configured with the vehicle registration number as its Bus ID.

Example development IDs:

- `KL-01-AB-1001`
- `KL-01-AB-1002`
- `KL-01-AB-1003`

### Inputs

- Boarded +1 button
- Depart/close boarding control
- Optional exit-bus +1 button for the return/exit-count feature
- Optional door/vehicle status input

### Outputs

- OLED or larger display
- Buzzer
- Status LED
- Optional external beacon

### Display information

At minimum:

```text
KL-01-AB-1001

CAPACITY       27
RELEASED       20
BOARDED        17
TO BOARD        3
AVAILABLE       7

BOARDING
```

After all released staff board:

```text
CAPACITY       27
RELEASED       20
BOARDED        20
TO BOARD        0
AVAILABLE       7

READY TO DEPART
```

## 8. No dedicated Wi-Fi gateway at each showroom

Do not deploy a separate ESP32 gateway just to bridge the devices. Each node has its own Wi-Fi connection to `JSPL IoT`.

This reduces hardware, wiring, failure points and installation complexity.

A gateway may be introduced later only if the business requires local buffering, local LAN isolation, or another protocol that cannot run directly on the nodes.

## 9. Device identity

Every controller receives a permanent device identity separate from its role.

Example:

```text
Device ID: PVM-GATE-01
Role:      GATE
Showroom:  PVM

Device ID: PVM-BUS-01
Role:      BUS
Showroom:  PVM
Bus ID:    KL-01-AB-1001
```

The hardware MAC address is a technical identifier only. It must not be used as the human-facing Bus ID.

The vehicle registration number remains the permanent Bus ID in the application.

## 10. Configuration instead of rewiring

The same hardware must be deployable at another showroom by changing configuration:

```text
showroom_id = PVM
role = GATE
```

or:

```text
showroom_id = PVM
role = BUS
bus_id = KL-01-AB-1001
capacity = 27
```

For another showroom, the same board becomes for example:

```text
showroom_id = TVM
role = BUS
bus_id = <actual registration>
capacity = 35
```

No firmware source-code modification should be required for normal deployment.

## 11. Offline-first behaviour

Every node must maintain a local event queue.

Example:

```text
Internet lost
     |
     v
Local event recorded
     |
     v
Local count updated
     |
     v
Device continues operating
     |
Internet returns
     |
     v
Queued events synchronized
```

A power cycle must not silently lose confirmed events.

## 12. Hardware modularity

The production PCB should be designed as a controller plus replaceable peripheral modules:

```text
+------------------------------------------------+
|              JSPL TRANSPORT NODE              |
|                                                |
| ESP32-S3 controller                            |
| Power + protection                             |
| Wi-Fi                                          |
| Local storage                                  |
|                                                |
| I2C ---- Display module                       |
| GPIO --- Button module                        |
| GPIO --- Status/buzzer module                 |
| GPIO --- Optional sensor/relay module         |
| RS485 --- Future long-distance module          |
+------------------------------------------------+
```

This lets the same controller be repaired or upgraded without redesigning the entire system.

## 13. Recommended pilot hardware stages

### Stage A — Bench

- ESP32 DevKit
- 0.96-inch SSD1306 OLED
- 3 destination buttons
- 1 boarding button
- 1 reset/service button
- buzzer

### Stage B — Palarivattom pilot

- 1 reusable GATE node
- 3 reusable BUS nodes
- `JSPL IoT` connectivity
- proper enclosures
- regulated power
- physical buttons suitable for repeated daily use

### Stage C — Production

Move from development boards to a custom common controller PCB using ESP32-S3, protected power input, pluggable connectors and service access.

## 14. Hardware rules

1. Never hard-code showroom or vehicle identity into the electrical design.
2. Never require local IP addresses for device communication.
3. Never store Wi-Fi passwords in the Git repository.
4. Do not make a safety-critical door lock dependent on the ESP32.
5. Use local persistence for operational events.
6. Use a watchdog and safe boot/recovery behaviour.
7. Use connectors and labels suitable for field replacement.
8. Keep the controller board identical wherever practical.
9. Treat capacity as a configurable maximum, not a dispatch target.
10. Keep staff-ready, gate-release and bus-boarded actions as separate logical events.
