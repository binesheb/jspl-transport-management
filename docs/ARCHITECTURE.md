# Jayalakshmi DRIVE — Architecture Direction

## 1. Target product surfaces

```text
                    Jayalakshmi DRIVE
                           |
          +----------------+----------------+
          |                |                |
      Rider App        Driver App        Web Apps
      Android/iOS      Android/iOS      Booking/Admin
          |                |                |
          +----------------+----------------+
                           |
                     API / Realtime
                           |
              +------------+------------+
              |                         |
        Core Backend             Dispatch Engine
              |                         |
        PostgreSQL/PostGIS            Redis
              |
      +-------+-------+-------+
      |       |       |       |
   Booking  Users   Fleet   Tracking
                           |
                      Map Provider
                           |
                  +--------+--------+
                  |                 |
                 OSM             Google
              (initial)          (later)
```

## 2. Architectural principles

### Progressive delivery

The UI must be built and reviewed before full backend integration. Mock repositories/data are acceptable during UI phases.

### Provider independence

Maps, routing, geocoding, notifications and GPS hardware are integrations, not business logic.

### Dispatch is a domain service

The booking API must not contain all dispatch rules. Dispatch needs an independently testable service with deterministic candidate filtering, scoring and wave progression.

### Realtime is separate from persistence

WebSockets/streaming are used for current state. PostgreSQL/PostGIS remains the durable source of record.

### Auditability

Important booking/dispatch/trip changes are event/audit records rather than destructive status changes only.

### Offline tolerance

Mobile location and critical trip state must tolerate temporary connectivity loss.

## 3. Map provider contract

Business logic calls an internal map interface rather than an SDK directly.

Example operations:

```text
Geocode(query)
ReverseGeocode(latitude, longitude)
SearchPlaces(query, proximity)
Route(origin, destination, waypoints, options)
DistanceMatrix(origins, destinations)
EstimateETA(route/request)
```

Providers:

- OSM + OSRM + Nominatim initially
- Google Maps/Routes/Places later
- Additional providers may be added without changing booking/dispatch logic

## 4. Tracking model

The tracking subsystem accepts normalized location updates:

```text
subject_id
source
latitude
longitude
accuracy
speed
heading
timestamp
received_at
trip_id
```

Sources may include:

- Driver mobile app
- Dedicated GPS/Traccar-compatible device
- Future IoT/ESP32 transport devices where appropriate

## 5. Dispatch model

The dispatch engine receives a booking intent and returns a sequence of offers/assignment decisions.

```text
Booking Intent
     |
     v
Eligibility filters
     |
     v
Candidate search
     |
     v
Candidate scoring
     |
     v
Dispatch wave
     |
  +--+--+
  |     |
Accept  timeout/decline
  |     |
  |     v
  |   next wave
  |     |
  |     v
  |   wider/future pool
  |     |
  +-----+
        |
        v
Assignment or escalation
```

The engine must understand current trips and predicted completion times so a driver can be reserved for a future booking.

## 6. Public deployment

Target:

`https://fleet.jayalakshmi.in`

Recommended production topology:

```text
Internet
   |
HTTPS :443
   |
Reverse proxy / TLS
   |
+--+----------------------+
|                         |
Web UI                 API / WS
                           |
                      internal ports
                           |
                 Docker service network
```

The backend/dashboard may listen on internal ports such as 8000/3000/8080, but these should not be exposed directly to the public Internet when a reverse proxy is available.

## 7. Existing repository compatibility

The repository already contains ESP32 transport counting/boarding functionality. The new Jayalakshmi DRIVE platform must preserve those workflows and expose integration boundaries rather than replacing working device firmware prematurely.

## 8. Proposed module boundaries

```text
apps/
  rider/
  driver/
  web-booking/
  operations/

services/
  api/
  dispatch/
  tracking/
  notifications/

packages/
  domain/
  map-provider/
  ui/

infrastructure/
  postgres/
  redis/
  osm/
  monitoring/

docs/
```

The exact repository layout may evolve after the first UI prototype; architecture changes must be documented rather than silently mixed into feature work.
