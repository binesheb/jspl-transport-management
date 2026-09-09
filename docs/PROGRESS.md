# Jayalakshmi DRIVE — Progress Tracker

**Tracker version:** 0.3.0  
**Last updated:** 2026-09-09

> This tracker is deliberately incremental. A phase is not complete merely because code exists; it must meet its exit criteria and have evidence recorded.

## Overall status

**Current phase:** Field PoC acceleration (parallel to Phase 02)  
**Overall completion:** 12% (foundation + field-test implementation)

| Phase | Name | Status | Target outcome |
|---|---|---|---|
| 00 | Product discovery / inspiration | 🟢 Complete | Requirements and reference patterns collected |
| 01 | Product foundation | 🟢 Complete | Living PRD, roadmap, architecture principles, tracker |
| 02 | Base design system | 🟡 In progress | Shared visual language and reusable components |
| 03 | Rider app base UI | 🟡 Field PoC slice active | Manager/booker flow implemented for first real test |
| 04 | Driver app base UI | 🟡 Field PoC slice active | Driver online, offer, accept and trip-state flow implemented |
| 05 | Web booking base UI | ⚪ Not started | Responsive booking journey |
| 06 | Operations dashboard base UI | 🟡 Foundation exists | Existing dashboard foundation plus requirements; full control room pending |
| 07 | Map provider abstraction | ⚪ Not started | OSM-first mapping with swappable provider contract |
| 08 | Booking simulation | 🟡 Field PoC implementation | Real API-backed manager-to-driver booking lifecycle |
| 09 | Backend foundation | 🟡 Field PoC API slice active | API endpoints supporting driver presence, booking and dispatch |
| 10 | Real authentication | ⚪ Not started | Employee/driver/admin identity and roles |
| 11 | Real GPS tracking | 🟡 Field PoC foreground slice active | Driver phone GPS sent to backend while app is active |
| 12 | Dispatch MVP | 🟡 Single-driver slice active | First available driver receives an offer and can accept/decline |
| 13 | Scheduled/future driver dispatch | ⚪ Not started | Driver-after-current-trip and scheduled rides |
| 14 | Notifications / safety | 🟡 Basic alert slice active | In-app offer + vibration; FCM/OTP/SOS later |
| 15 | Fleet/vehicle management | ⚪ Not started | Vehicles, drivers, branches and documents |
| 16 | HRONE / JSPL integrations | ⚪ Not started | Employee and existing transport integration |
| 17 | Advanced dashboard / analytics | ⚪ Not started | Operational intelligence and reporting |
| 18 | AI dispatch intelligence | ⚪ Not started | ETA prediction, demand and positioning |
| 19 | Production deployment | 🟡 Portable foundation | Docker-based Windows/Linux deployment foundation |

## Field PoC milestone

### Implemented on `main`

- [x] Manager/booker mobile screen.
- [x] Driver mobile screen.
- [x] Driver online/offline presence API.
- [x] Driver foreground GPS upload API.
- [x] Manager creates a booking.
- [x] Manager offers booking to an available driver.
- [x] Driver receives the offer through polling and vibration.
- [x] Driver accepts or declines.
- [x] Manager observes assignment state through polling.
- [x] Driver advances trip through en-route, arrived, start and complete states.
- [x] Driver becomes available again after completion.
- [x] Mobile PoC server URL can be configured and health-checked on the phone.
- [x] Android CI workflow builds a release APK artifact.
- [x] Backend lifecycle test added for the manager → driver → completed trip flow.

### Not yet field-test complete

- [ ] Successful CI build verified.
- [ ] APK artifact downloaded and installed on a physical Android phone.
- [ ] Laptop backend deployed and reachable from the phone over LAN/tunnel.
- [ ] Real two-device manager/driver test completed.
- [ ] Foreground GPS verified during movement.
- [ ] Background/locked-screen GPS reliability hardened.
- [ ] Real map tiles/navigation integrated.

## Phase 02 — Base design system

### Completed

- [x] Define Phase 02 design brief.
- [x] Define screen inventory.
- [x] Establish destination-first rider interaction as the primary pattern.
- [x] Establish shared mobile + web component direction.
- [x] Preserve map-provider independence in UI components.

### In progress

- [ ] Brand tokens
- [ ] Typography
- [ ] Color and surface tokens
- [ ] Spacing and sizing
- [ ] Buttons and inputs
- [ ] Cards and bottom sheets
- [ ] Map surfaces and controls
- [ ] Driver cards
- [ ] Booking status cards
- [ ] Loading/empty/error states
- [ ] Rider shell
- [ ] Driver shell
- [ ] Web booking shell
- [ ] Operations dashboard shell
- [ ] Responsive rules
- [ ] Light/dark theme decision

## Deployment

The same `main` repository remains the source of truth. The intended PoC deployment is the Docker stack on the user's Windows laptop, with the same stack later deployable to Linux/VPS/cloud infrastructure. Production target remains `fleet.jayalakshmi.in`.

## Progress update rule

Whenever a phase changes:

1. Update this file.
2. Update `docs/PRD.md` if requirements change.
3. Add/close related GitHub issues.
4. Record test/build evidence where applicable.
5. Keep implementation on `main` while the single-branch development policy is active.

## Current next action

**Finish the Android CI/build verification, then produce the first installable Manager + Driver field-test APK and validate the two-device flow against the laptop PoC backend.**
