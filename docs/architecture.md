# Architecture

## Pilot scope

The first production-shaped pilot is Palarivattom (`PVM`) with three buses. Development uses placeholder Kerala registration numbers until the real vehicle registrations are supplied.

## Connectivity

Every ESP32 connects only to the `JSPL IoT` SSID. The network provides Internet access but intentionally does not provide device-to-device LAN communication. Therefore all device coordination happens through the cloud API/MQTT layer.

## Event-first counting

A button press is represented as an immutable event. The server derives trip counts from events:

- `staff_exit` increases expected count.
- `staff_board` increases boarded count.
- `staff_exit_bus` increases the count of people leaving the bus.

The `event_id` is unique so retransmission after an offline period is safe and idempotent.

## Offline operation

The bus unit must maintain a local queue of events when the Internet is unavailable. Once connectivity returns, it republishes queued events using their original IDs. The API treats duplicate event IDs as already accepted.

## State model

`online` is a device connectivity state. It does not automatically mean `ready_for_boarding`. Boarding is controlled by a trip/session state in the backend.

Planned trip states:

`planned -> boarding -> boarding_complete -> departed -> completed`
