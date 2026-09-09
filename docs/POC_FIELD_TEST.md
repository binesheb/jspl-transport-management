# Jayalakshmi DRIVE — First Field PoC

## Goal

Prove the complete minimum real-world loop with one Manager phone, one Driver Android phone and the PoC server.

## Test setup

- Manager phone: Android initially; used to create a booking.
- Driver phone: Android; installed with Driver app.
- Server: Windows laptop running the same Docker deployment intended for later production migration.
- Mapping: OpenStreetMap-first.
- GPS: real driver-phone GPS.
- Dispatch: first available driver, then progressively expanded dispatch in later phases.

## Acceptance flow

1. Driver opens the Driver app.
2. Driver signs in to the demo account.
3. Driver grants location permission.
4. Driver switches to **Online**.
5. Manager opens the Manager/Rider app.
6. Manager enters a destination.
7. Manager selects **Ride Now**.
8. Server creates the booking.
9. Driver receives a prominent incoming-trip alert with ringing/vibration where platform policy permits.
10. Driver accepts.
11. Manager sees the driver assignment.
12. Driver location appears on the Manager map and updates while moving.
13. Driver transitions through Arrived → OTP → Start Trip → End Trip.
14. Manager sees the same trip-state changes.
15. Trip is stored in the backend and visible in history.

## First PoC non-goals

- Payments
- Production-grade multi-wave dispatch
- HRONE integration
- Payroll posting
- Dedicated GPS hardware
- Production push-notification infrastructure
- Full analytics

## Quality gates

- App does not crash during the field flow.
- GPS updates are timestamped and visibly move on the manager side.
- Booking state is consistent between manager and driver.
- A driver cannot accept the same booking twice.
- Completing a trip produces one persisted completed trip.
- Server restart does not corrupt persisted trips.
- The PoC can be redeployed from GitHub using the documented deployment path.

## Next expansion after successful field test

- Multiple drivers
- Dispatch waves
- Decline/timeout
- Driver finishing-current-trip queue
- Scheduled booking
- Real push notifications
- Background GPS hardening
- Incentive calculation
