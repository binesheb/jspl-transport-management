# Architecture Audit — Open Issues and Decisions

This document records issues found during review of the reusable hardware architecture and the decisions required before the Palarivattom pilot is frozen.

## 1. Destination must be separated from Bus ID

A staff member should select their **destination/route**, not necessarily a physical vehicle registration number.

A bus can be assigned to a destination for a particular trip, and that assignment can change.

Therefore the data model should use:

```text
Destination / Route
        |
        +---- Trip
                 |
                 +---- Bus (vehicle registration number)
```

The gate device should show destination labels to staff. The gatekeeper then releases a destination queue to the bus currently assigned to that destination/trip.

For the Palarivattom pilot, if each bus has a permanently known destination, the UI can visually combine the two, but the software model must keep them separate.

## 2. Online is not the same as ready

A bus node being connected to `JSPL IoT` only proves connectivity.

Separate states are required:

```text
OFFLINE
ONLINE / NOT READY
READY FOR BOARDING
BOARDING
READY TO DEPART
DEPARTED
```

A release may only be issued when the bus is in `READY FOR BOARDING` or an explicitly permitted boarding state.

## 3. Capacity calculation must include released-but-not-boarded staff

Do not calculate available seats as simply `capacity - boarded` when releases are outstanding.

Correct pilot logic:

```text
outstanding_released = released - boarded
available_for_release = capacity - boarded - outstanding_released
```

Example:

```text
Capacity:              27
Boarded:               18
Released:              20
Outstanding released:  2
Available for release:  7
```

This prevents the gatekeeper from releasing more staff than the vehicle can safely accommodate while previously released staff are still expected to board.

## 4. Release is a batch event

`RELEASE QUEUE` should create a release batch with:

- destination/route
- bus ID
- trip ID
- quantity
- timestamp
- gate/device ID
- operator/device identity where available

The batch then moves staff count from `READY` to `RELEASED`.

This is important for reconciliation when 20 are released and only 18 board.

## 5. Anonymous counting has a known limitation

In V1, a button press represents one staff member but does not identify the person.

Therefore the system cannot reliably prevent one person from pressing twice.

Mitigations for the pilot:

- hardware debounce
- visible/audible confirmation
- short repeat-press protection
- operator correction capability
- audit trail

Future versions can identify staff with RFID, NFC, QR or another staff credential if individual accountability becomes necessary.

## 6. Internet loss must be treated as a distributed-system failure

Because `JSPL IoT` blocks device-to-device communication, the gate cannot obtain live bus state when the cloud path is unavailable.

Each node may continue recording local events, but **cross-device decisions cannot be assumed to remain real-time**.

Pilot-safe rule:

- If the gate has stale/no bus state, show the bus as `STALE / UNKNOWN`.
- Do not automatically authorize a release based on stale capacity or readiness.
- Provide an explicit manual operational fallback for the gatekeeper.
- When connectivity returns, synchronize events using unique event IDs and idempotent processing.

## 7. Power loss must be recoverable

Every confirmed event must be committed to non-volatile local storage before the UI treats it as successful.

Use:

```text
Event created
   -> local durable storage
   -> count/state updated
   -> cloud sync
```

Never depend on RAM counters alone.

## 8. Time must be trustworthy enough for auditing

Events need timestamps. Nodes should synchronize time using NTP when connected and retain a monotonic sequence number locally.

Every event should have:

- event UUID
- device ID
- local sequence number
- event type
- trip ID
- timestamp
- payload/version

The sequence number is useful when the real-time clock is temporarily unavailable or corrected.

## 9. Gate UI must scale beyond three buses

The pilot can use three physical destination buttons, but the reusable hardware should not assume exactly three.

Preferred architecture:

- button expansion connector
- GPIO expander or button matrix
- optional touch display

Normal deployment should configure the number of active destinations/routes rather than require a new PCB.

## 10. Do not electronically lock an emergency exit

The staff-ready point can be adjacent to a controlled operational gate, but the system must not make a fire/emergency egress door dependent on the ESP32, Internet or cloud.

The system can control a non-safety-critical proceed indicator, gate signal, or operator release mechanism where appropriate and legally compliant.

## 11. Bus departure needs an explicit trip transition

`DEPART` should close the current boarding session and create a departure event.

After departure, no new staff should be released to that bus/trip.

A later trip for the same vehicle creates a new trip/session.

## 12. Bus exit counting is a separate feature

If the system later counts staff exiting the bus, that must not be mixed with the boarding count.

Use a separate event/counter:

```text
BOARDING COUNT
BUS EXIT COUNT
```

The same physical button may be configurable by operating mode, but the event types must remain distinct.

## 13. Reusable hardware decision

The common JSPL Transport Node remains the correct direction:

```text
ESP32-S3 controller
+ protected power
+ common connectors
+ configurable peripherals
+ role configuration
```

The role and configuration should be software-defined:

```text
GATE
BUS
DISPLAY
```

No showroom-specific PCB should be manufactured.

## 14. Pilot freeze checklist

Before moving to a custom PCB, validate these physically at Palarivattom:

- destination button press reliability
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

Only after these tests pass should the custom reusable PCB be frozen.
