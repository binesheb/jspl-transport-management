# Architecture

## Pilot scope

The first production-shaped pilot is Palarivattom (`PVM`) with three buses. Development bus registrations are `KL-01-AB-1001`, `KL-01-AB-1002`, and `KL-01-AB-1003`; these are placeholders until the real vehicle registrations are supplied.

## Connectivity

Every ESP32 connects only to the `JSPL IoT` SSID using its configured Wi-Fi credentials. The network provides Internet access but does not provide device-to-device LAN communication. Therefore all device communication is brokered by the cloud.

```text
                 JSPL IoT
                    |
              Internet access
                    |
       +------------+------------+
       |                         |
 ESP32 Exit Counter          ESP32 Bus Unit
       |                         |
       +------------+------------+
                    |
               Cloud MQTT
                    |
          +---------+---------+
          |                   |
      PostgreSQL          Web Dashboard
```

## Core entities

- **Showroom**: permanent business location identifier such as `PVM`.
- **Bus**: physical vehicle. The vehicle registration number is the user-facing Bus ID.
- **Device**: physical ESP32 and its role (`EXIT_COUNTER` or `BUS_UNIT`).
- **Trip**: a specific movement by one bus from one showroom on a date/time.
- **Transport event**: immutable event such as `STAFF_EXIT`, `BOARD`, `STAFF_EXIT_BUS`, or `TRIP_DEPARTED`.

## State model

`OFFLINE -> ONLINE -> READY -> BOARDING -> BOARDING_COMPLETE -> DEPARTED -> COMPLETED`

Online status is based on device heartbeat and does not itself mean that a bus is ready for boarding. Boarding is explicitly opened for a trip.

## Offline-first bus counting

A bus unit maintains a durable local event queue. A button press is accepted locally, displayed immediately, and queued for synchronization. When Internet connectivity returns, queued events are published to the cloud with unique event IDs. The server must treat event IDs as idempotency keys so retransmission cannot double-count a passenger.

## Counting model

The system stores events rather than trusting a single mutable counter. Current expected and boarded counts are derived from the active trip's accepted events. This creates an audit trail and permits administrative corrections without rewriting history.

## Security principles

- Never commit Wi-Fi passwords, MQTT credentials, API secrets, or production database passwords.
- Each device has a unique device ID and authentication credential.
- Cloud APIs authenticate devices separately from human dashboard users.
- Server-side validation prevents boarding counts from exceeding expected counts unless an authorized override is recorded.
