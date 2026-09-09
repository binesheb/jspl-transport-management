# Jayalakshmi DRIVE — Initial Design Tokens

**Status:** Phase 02 working draft

These tokens are intentionally a foundation, not a final brand guideline. They should be refined against approved Jayalakshmi brand assets before Phase 02 completion.

## Design principles

- Premium but operational
- Fast and calm
- High information clarity
- Familiar ride-hailing interactions without copying Uber branding
- Large, reliable touch targets
- Strong distinction between booking state and driver state
- Accessible contrast and readable typography

## Semantic colors

Use semantic names in implementation. Do not scatter raw hex values through screens.

- `brand.primary`
- `brand.primaryStrong`
- `surface.canvas`
- `surface.card`
- `surface.elevated`
- `text.primary`
- `text.secondary`
- `text.muted`
- `state.success`
- `state.warning`
- `state.danger`
- `state.info`
- `map.driverAvailable`
- `map.driverOnTrip`
- `map.driverOffline`

Exact brand values require validation against Jayalakshmi's approved logo/brand assets.

## Typography

Primary typeface should prioritize Android/iOS/web availability and excellent Malayalam/English rendering. Candidate: Inter for Latin UI with a compatible Noto Sans family for multilingual fallback.

Suggested hierarchy:

- Display: 32/40
- Heading 1: 24/32
- Heading 2: 20/28
- Title: 18/24
- Body: 16/24
- Body small: 14/20
- Caption: 12/16
- Numeric KPI: 28/34

## Spacing

Base unit: 4px.

Common values: 4, 8, 12, 16, 20, 24, 32, 40.

## Radius

- Small controls: 8
- Cards: 12
- Sheets: 20
- Full pills: 999

## Touch targets

Minimum interactive target: 44×44 logical pixels; prefer 48×48 for primary mobile controls.

## Elevation

Use elevation sparingly. Maps should remain visually dominant; sheets/cards should separate from maps through surface contrast and subtle shadow rather than heavy decoration.

## Component conventions

### Primary action
One dominant action per screen where practical.

### Search field
Destination-first and optimized for minimal typing.

### Bottom sheet
Used for pickup/destination confirmation, ride options, booking status and driver details.

### Driver card
Must consistently expose driver name, vehicle, ETA and relevant status.

### Booking status
Must use both text and visual state; color alone must never communicate a critical state.

### Map controls
Keep controls compact and accessible without covering the pickup/drop-off context.

## Themes

Light theme is the initial default. Dark theme remains a design-system capability and will be evaluated before Phase 02 completion.
