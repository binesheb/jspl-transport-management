# Jayalakshmi DRIVE — Phase 02 Design Brief

## Objective
Create the shared visual foundation before implementing functional booking or dispatch.

## Product surfaces
- Rider mobile: Android + iOS
- Driver mobile: Android + iOS
- Responsive web booking
- Operations/control-room web

## Design direction
Jayalakshmi DRIVE should feel premium, calm, fast and trustworthy. The interaction model may be familiar to modern ride-hailing products, but visual identity must remain distinctly Jayalakshmi.

## Base UI to design first
1. App shell and navigation
2. Brand/header treatment
3. Location and destination search
4. Primary action button
5. Map container and map controls
6. Bottom sheet
7. Driver/vehicle card
8. Booking status card
9. Empty/loading/error states
10. Confirmation and cancellation surfaces
11. Web dashboard sidebar/top bar
12. Live driver status indicators

## Interaction principles
- Destination-first booking
- One obvious primary action per screen
- Minimal typing
- Large touch targets
- Clear booking state
- Clear driver ETA/status
- No ambiguous disabled states
- Accessibility-aware contrast and typography
- Responsive layouts for mobile, tablet and desktop

## Progressive rule
Phase 02 does not implement real GPS, dispatch, authentication, payments or backend integration. Use representative mock data only.

## Map abstraction
The UI must not assume Google Maps. Map surfaces should work with an abstract map component so OpenStreetMap can be used initially and Google Maps later.

## Exit criteria
- Shared design tokens documented
- Core components implemented/reviewable
- Rider base screens designed
- Driver base screens designed
- Web booking base screens designed
- Operations dashboard base screens designed
- Responsive behaviour defined
- Light/dark decision documented
- No real backend dependency
