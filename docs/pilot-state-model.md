# Pilot State Model

The application should treat counts as state derived from events. The core operational fields for each bus are:

- `capacity` — maximum safe passenger count.
- `ready_queue` — staff who selected this destination but have not been released.
- `released` — staff released from the showroom toward this bus for the active boarding group.
- `boarded` — released staff who have actually boarded.
- `available_seats` — `capacity - boarded`.
- `status` — current bus operating state.

## Gate release rule

When the gatekeeper selects **Release Queue**:

```text
release_count = min(ready_queue, available_seats)
```

This is a safety ceiling only. There is no minimum release count and no requirement to fill the bus.

The gatekeeper may release a queue containing 1 person, 20 people, or any other number up to the available-seat limit. A future partial-release control can allow the operator to release less than the full queue.

## Example

```text
capacity       = 27
ready_queue    = 20
boarded        = 0
available      = 27
```

The operator releases the queue:

```text
released       = 20
ready_queue    = 0
```

After all 20 board:

```text
boarded        = 20
available      = 7
```

The bus may depart even though 7 seats remain.

## Not-ready bus

If a bus is not ready for boarding, `Release Queue` is disabled. Staff may continue accumulating in that bus's destination queue.

## Departure

A normal departure is allowed when:

```text
released == boarded
```

The pilot should show an explicit warning if departure is attempted while released staff remain unboarded. An override can be introduced later if operationally required.
