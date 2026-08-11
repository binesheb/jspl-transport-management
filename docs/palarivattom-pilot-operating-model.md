# Palarivattom Pilot — Operating Model

## Purpose

The pilot is a gate-controlled staff transport release system. Staff declare that they are ready to board and select their destination. The gatekeeper decides when to release the waiting queue. The system does not decide when a group is large enough.

## Devices

- Staff Ready Counter: one device at the showroom exit/gate.
- Gate Controller: the gatekeeper's control interface. This may initially be combined with the staff-ready device during testing.
- One Bus Controller per bus.
- All ESP32 devices use the `JSPL IoT` SSID.
- Because `JSPL IoT` does not permit device-to-device communication, device state must be exchanged through the cloud in the connected version.

## Three buses

Development vehicle IDs:

- `KL-01-AB-1001`
- `KL-01-AB-1002`
- `KL-01-AB-1003`

Production will use the actual vehicle registration numbers.

## Staff flow

1. A staff member arrives at the exit.
2. She presses the button for her destination/bus.
3. The destination queue increases by one.
4. The staff member remains inside the showroom until the gatekeeper releases the queue.
5. The gatekeeper chooses when to release the queue for a bus.
6. The system may release no more people than the bus's currently available seats.
7. The gatekeeper may release fewer than the bus capacity. Bus capacity is a safety limit, not a target.
8. Released staff proceed to the bus and boarding is counted.
9. Once all released staff have boarded, the gatekeeper/driver may mark the bus ready to depart.

## Example

A bus has capacity 27 and 20 staff are waiting for it.

The gatekeeper can release all 20. The bus does not need to reach 27 passengers.

After boarding:

```text
Capacity:       27
Waiting:         0
Released:       20
Boarded:        20
Available:       7
Status: READY TO DEPART
```

## Gatekeeper authority

The gatekeeper controls:

- When a queue is released.
- Which bus's queue is released.
- Whether to release the full waiting queue or a smaller group.
- When boarding is closed.
- When the bus may depart.

The system enforces:

- A bus that is not ready for boarding cannot receive a release.
- A release cannot exceed available seats.
- A staff member cannot be released twice from the same queue event.
- Boarding cannot exceed the number released for that bus/group.
- Departure should normally require all released staff to have boarded; an explicit operator override can be added later.

## States

### Bus

`NOT_READY -> READY_FOR_BOARDING -> BOARDING -> READY_TO_DEPART -> DEPARTED`

### Staff queue entry

`READY -> RELEASED -> BOARDED`

A queue entry that has not been released remains inside the showroom.

## V1 pilot principle

Do not introduce automatic group-size rules, timers, minimum thresholds, or automatic dispatch decisions. The pilot is intended to validate the human-controlled operating process first.
