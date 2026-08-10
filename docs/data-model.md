# Data Model

## Showrooms

A showroom is identified by a short permanent code.

| Field | Example |
|---|---|
| `id` | UUID |
| `code` | `PVM` |
| `name` | `Palarivattom` |
| `active` | `true` |

## Buses

A bus is identified to users by its vehicle registration number.

| Field | Example |
|---|---|
| `id` | UUID |
| `registration_number` | `KL-01-AB-1001` |
| `capacity` | `40` |
| `active` | `true` |

The database may use an internal UUID, but UI and operational workflows use `registration_number`.

## Devices

| Field | Example |
|---|---|
| `id` | UUID |
| `device_code` | `BUS-KL01AB1001` |
| `device_type` | `BUS_UNIT` |
| `showroom_id` | nullable |
| `bus_id` | nullable |
| `last_seen_at` | timestamp |
| `firmware_version` | `0.1.0` |
| `active` | `true` |

An exit counter belongs to a showroom. A bus unit belongs to a bus. A bus is not permanently assigned to a showroom.

## Trips

A trip binds a bus to a showroom and operating context for a specific run.

Important fields:

- `id`
- `showroom_id`
- `bus_id`
- `service_date`
- `route_name`
- `status`
- `boarding_opened_at`
- `departed_at`
- `completed_at`

## Transport events

Events are append-only records.

Important fields:

- `id` — server UUID
- `event_id` — unique device-generated idempotency key
- `trip_id`
- `device_id`
- `event_type`
- `occurred_at`
- `sequence_number`
- `metadata`
- `received_at`

Initial event types:

- `STAFF_EXIT`
- `BOARD`
- `STAFF_EXIT_BUS`
- `TRIP_DEPARTED`
- `TRIP_COMPLETED`
- `CORRECTION`

The initial pilot uses `STAFF_EXIT` to increase expected passengers and `BOARD` to increase boarded passengers.
