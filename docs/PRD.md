# Jayalakshmi DRIVE — Product Requirements Document

**Status:** Living document  
**Version:** 0.2.0  
**Last updated:** 2026-09-09  
**Product:** Jayalakshmi DRIVE / JSPL Transport Management  
**Primary domain:** `fleet.jayalakshmi.in`  

> This document is the product source of truth. Requirements, decisions, scope changes and completed capabilities must be reflected here as the project evolves.

## 1. Product vision

Jayalakshmi DRIVE is a private, Uber-style transport and fleet platform for Jayalakshmi. An authorized user should be able to open the Android or iOS app, enter a destination, request a vehicle immediately or schedule one for later, and receive a suitable Jayalakshmi driver through intelligent dispatch.

The same platform will provide:

- Driver Android/iOS app
- Employee/rider Android/iOS app
- Responsive web booking experience
- Operations/control-room web dashboard
- Fleet, vehicle and driver management
- Real-time GPS tracking
- Scheduled and recurring bookings
- Intelligent dispatch and driver queuing
- Trip category and driver incentive management
- OpenStreetMap-first mapping with the ability to switch to Google Maps later
- Integration points for HRONE, staff transport and future business systems
- Self-hosted deployment capability under `fleet.jayalakshmi.in`

## 2. Core user promise

The primary rider journey should feel as simple as:

`Open app → Enter destination → Ride Now / Schedule → Driver found → Track → Complete`

If no driver is immediately free, the system must actively continue looking rather than simply returning "no drivers available".

## 3. Critical dispatch requirement

A booking must be handled by a dispatch engine, not by a simple nearest-driver lookup.

### 3.1 Immediate booking

1. Determine pickup and destination.
2. Validate required vehicle/category and passenger capacity.
3. Find suitable drivers using live availability, location, ETA, vehicle suitability, duty status and configured business rules.
4. Offer the trip to a dispatch wave of best candidates.
5. Use accept/decline/timeout handling.
6. If nobody accepts, expand the candidate pool and/or radius.
7. Search drivers who are currently on another trip but are predicted to become available soon.
8. Allow the user to continue waiting for a driver, accept the next available driver, reschedule, or request dispatcher assistance.
9. Once assigned, stop competing offers and track the assigned driver in real time.

### 3.2 Driver finishing current trip

A driver who is currently on a trip can become an eligible future candidate when the estimated completion time makes the driver suitable for a requested pickup.

The system must support a driver's trip queue:

`Current trip → Next assigned trip → Following scheduled trip`

The rider should see an honest estimated pickup time and the fact that the driver is completing another trip when applicable.

### 3.3 Scheduled booking

Scheduled bookings must enter the dispatch/scheduling engine before the pickup time. The system should progressively seek and confirm an appropriate driver rather than waiting until the last minute.

Priority/guarantee modes may be added for management, VIP, airport and important trips.

## 4. Rider / employee app requirements

### V1 foundation

- Splash / brand experience
- Authentication shell
- Home screen
- Current location
- Destination search UI
- Map screen
- Ride Now / Schedule choices
- Booking confirmation UI
- Booking state screen
- Driver assignment screen
- Active trip screen
- Trip history shell
- Profile/settings shell

### Later

- Saved places
- Recent destinations
- Multi-stop trips
- Ride sharing/trip sharing
- Chat
- Driver call
- SOS
- Ratings
- Recurring bookings
- Vehicle/category selection
- Accessibility/passenger options
- Booking preferences
- Notifications

## 5. Driver app requirements

### V1 foundation

- Driver login shell
- Online/offline control
- Current location permission flow
- Driver home/dashboard
- Incoming booking request UI
- Accept / decline UI
- Assigned trip screen
- Navigation hand-off
- Arrived state
- OTP verification
- Start trip
- End trip
- Basic trip history

### Later

- Upcoming trip queue
- Driver break/duty states
- Earnings/cost view where applicable
- Vehicle selection
- Vehicle inspection/checklist
- Incident/SOS
- Chat
- Ratings
- Documents
- Shift/duty management
- Offline GPS buffering
- Background location reliability improvements

## 6. Web applications

### 6.1 Public/internal booking web app

A responsive browser experience must support the same primary rider journey:

- Enter pickup
- Enter destination
- Ride Now
- Schedule
- View booking status
- View assigned driver
- Live trip tracking
- Trip history

### 6.2 Operations dashboard

The operations dashboard is a first-class product surface and the control center for the entire fleet.

#### Overview

- Live KPI cards
- Online drivers
- Available drivers
- Drivers on trip
- Drivers becoming available soon
- Unassigned bookings
- Scheduled bookings
- Delayed trips
- Active alerts
- Fleet/vehicle count

#### Live fleet map

The dashboard must show live driver/vehicle positions and status using the same provider-independent mapping layer as the mobile applications.

Driver/vehicle status should include at minimum:

- Available
- On trip
- Finishing current trip / available soon
- Reserved
- Offline
- Emergency
- Vehicle issue

Selecting a vehicle should show driver, vehicle, current trip, next assignment, GPS freshness, ETA and relevant operational actions.

#### Dispatch control center

Dispatchers must be able to see:

- Unassigned bookings
- Candidate drivers
- Candidate ETA
- Dispatch wave
- Offer timeout
- Driver accept/decline state
- Current trip conflicts
- Future availability
- Manual assignment
- Manual reassignment
- Dispatch override
- Escalation state

The dashboard should expose enough information to explain why a driver was selected.

#### Driver queue

For drivers with current and future work, display:

`Current trip → Next trip → Following scheduled trip`

The manager must be able to reserve/assign a future trip where business rules permit it.

#### Scheduled trips

Provide calendar/timeline views for:

- Upcoming bookings
- Assigned/unassigned status
- Driver
- Pickup time
- Estimated completion
- Priority
- Category
- Branch

#### Trip categories

Trip category is a first-class configurable object.

Each category supports:

- Name
- Code
- Description
- Default incentive
- Operating window
- After-hours rule
- Vehicle restrictions
- Branch restrictions
- Dispatch priority
- Active/inactive state
- Effective date

Initial configured categories and incentive amounts are maintained in the Trip Category & Incentive configuration section below.

#### Incentive dashboard

Management must be able to see:

- Incentives today/week/month
- Incentives by driver
- Incentives by category
- Incentives by branch
- After-hours incentives
- Completed/approved/pending incentive totals
- Exportable incentive ledger

Every incentive must be traceable to its trip and rule.

#### Driver performance

The dashboard should support:

- Acceptance rate
- Completion rate
- Cancellation rate
- Average pickup time
- Trips completed
- Distance travelled
- Incentives earned
- Current duty state

#### Branch dashboard

Management should be able to compare branches by:

- Drivers
- Vehicles
- Trips
- Unassigned trips
- Distance
- Incentives
- Utilization

#### Alerts

Operational alerts should include, where applicable:

- Unassigned booking
- Scheduled trip without driver
- Driver delay
- Vehicle/GPS offline
- Emergency/SOS
- Missed pickup
- Dispatch timeout
- Booking conflict

### 6.3 Administration

- Drivers
- Vehicles
- Vehicle categories
- Showrooms/branches
- Users
- Roles/permissions
- Dispatch rules
- Trip categories
- Incentive rules
- Map provider
- Notifications
- Geofences
- Audit log
- System settings

## 7. Trip Category & Incentive Engine

Trip category and incentive configuration must be data-driven and editable by authorized management. Values must not be hard-coded into the mobile apps or dispatch service.

### 7.1 Initial incentive configuration

| Trip category | Default incentive |
|---|---:|
| Airport | ₹250 |
| Batch | ₹50 |
| Choice School | ₹150 |
| COK_100km | ₹750 |
| COK_Cherai | ₹200 |
| COK_CLT | ₹1,000 |
| COK_Edapally | ₹100 |
| COK_MLR | ₹1,500 |
| COK_Palarivattom | ₹100 |
| COK_TCR | ₹500 |
| COK_Thodupuzha | ₹300 |
| COK_TVM | ₹1,000 |
| Fish Market | ₹75 |
| Food_1 | ₹50 |
| Food_3 | ₹75 |
| Local_1 | ₹25 |
| Local_2 | ₹50 |
| Local_3 | ₹75 |
| Medicine | ₹50 |
| Railway Station | ₹100 |
| Servant Pickup | ₹100 |
| Staff Drop | ₹50 |
| Staff Drop | ₹100 |
| Vaikom Temple | ₹300 |
| Guruvayoor | ₹500 |
| COK_Kottarakara | ₹500 |
| Thuravoor | ₹250 |
| Kodungaloor | ₹300 |

**Data issue to resolve:** the supplied configuration contains two `Staff Drop` values (₹50 and ₹100). These must be differentiated by a business attribute or confirmed/merged before production configuration.

### 7.2 Operating-window rule

The supplied business rule states that incentive eligibility is affected by travel outside the **09:00–21:00** operating window.

The exact eligibility interpretation must be configurable and confirmed before production:

- Pickup outside operating window
- Any trip segment outside operating window
- Trip starts outside operating window
- Entire trip outside operating window
- Other approved rule

The system must record which rule version produced each incentive.

### 7.3 Incentive calculation and audit

For every completed trip:

1. Determine trip category.
2. Resolve the applicable incentive rule by effective date/time.
3. Evaluate time-window eligibility.
4. Calculate the incentive.
5. Store the rule/version used.
6. Create an immutable incentive ledger entry.
7. Allow authorized approval/adjustment with audit history.

Future payroll/HRONE integration must consume the ledger rather than independently recalculating historical incentives.

## 8. Mapping architecture

The mapping layer must be provider-independent.

### Initial provider

- OpenStreetMap map data
- Leaflet or MapLibre for web where appropriate
- OSRM or another open routing engine
- Nominatim or another configurable geocoder

### Future provider

Google Maps/Routes/Places may be enabled without redesigning the application.

All mapping operations should pass through an internal provider abstraction such as:

- `geocode`
- `reverseGeocode`
- `searchPlaces`
- `route`
- `distanceMatrix`
- `eta`
- `mapTiles`

The selected provider must be configuration-driven.

## 9. GPS and tracking

The system must support two tracking sources:

1. Driver smartphone GPS
2. Dedicated vehicle GPS devices

The tracking layer must not depend on the map provider.

Tracking requirements:

- Live position
- Timestamp
- Accuracy
- Speed
- Heading where available
- Connectivity state
- Trip association
- Route history
- Geofence events
- Offline buffering
- Synchronization after connectivity returns

Traccar or compatible GPS infrastructure may be integrated later for dedicated hardware.

## 10. Dispatch engine

The dispatch engine is a core product component and must remain independently testable.

### Candidate filters

- Driver online/available state
- Live GPS freshness
- Vehicle/category
- Passenger capacity
- Duty state
- Branch/region rules
- Current trip
- Predicted trip completion
- Existing future assignments
- Safety/operational restrictions

### Candidate scoring

Initial scoring should consider:

- Pickup ETA
- Distance
- Availability
- Vehicle suitability
- Scheduled conflicts
- Driver operational score

Weights must be configurable rather than hard-coded permanently.

### Dispatch waves

Example:

`Wave 1: best nearby candidates → Wave 2: wider pool → Wave 3: future-available drivers → Dispatcher escalation`

Wave timing, radius and concurrency must be configurable.

Incentive value may be displayed to authorized dispatchers and may optionally participate in candidate scoring if management explicitly enables incentive-aware dispatch. Incentive alone must never force assignment when safety, suitability or operational rules fail.

## 11. Booking state model

The system should use explicit state transitions rather than loosely interpreted status strings.

Example rider states:

`DRAFT → SEARCHING → OFFERED → ASSIGNED → DRIVER_EN_ROUTE → ARRIVED → OTP_VERIFIED → IN_TRIP → COMPLETED`

Possible alternate states:

`CANCELLED`, `EXPIRED`, `NO_DRIVER`, `REASSIGNING`, `DISPUTED`

Driver states must separately represent:

`OFFLINE`, `AVAILABLE`, `OFFERED`, `ASSIGNED`, `EN_ROUTE`, `ARRIVED`, `ON_TRIP`, `AVAILABLE_SOON`, `BREAK`, `EMERGENCY`, `VEHICLE_ISSUE`.

## 12. Safety

- OTP before trip start
- SOS for rider and driver
- Live trip location
- Emergency contact capability
- Trip sharing
- Audit trail
- Driver identity and vehicle display
- Cancellation/incident reasons

## 13. Jayalakshmi-specific requirements

The system must support:

- Multiple Jayalakshmi showrooms/branches
- Internal employees as riders
- Management/VIP bookings
- Company vehicles and drivers
- Staff transport use cases
- Scheduled showroom/hostel movements
- Recurring transport
- Future integration with HRONE
- Future integration with existing JSPL transport/ESP32 hardware
- Vehicle registration as a primary vehicle identifier
- Configurable branch/vehicle/driver pools

Existing ESP32 transport functionality in this repository remains supported; the new booking/fleet platform must not break existing boarding/counting workflows.

## 14. Platform requirements

### Mobile

Flutter is the preferred cross-platform framework for Android and iOS, allowing a shared codebase while retaining platform-specific capabilities for notifications and background location.

### Web

Responsive web applications for rider booking and operations/admin.

### Backend

Preferred architecture:

- API service
- PostgreSQL
- PostGIS
- Redis
- WebSockets/realtime transport
- Push notification service
- Background workers for dispatch/scheduling

Exact framework choices may evolve during architecture validation.

## 15. Deployment

Target public hostname:

`https://fleet.jayalakshmi.in`

The public endpoint should normally terminate TLS on standard HTTPS port 443 and reverse-proxy to internal application ports. The actual backend/dashboard service ports remain internal and configurable.

Initial deployment should be Docker-based and compatible with the existing JSPL server/deployment structure.

## 16. Security

- No secrets in Git
- Environment-based configuration
- HTTPS in production
- Secure authentication
- Role-based authorization
- Audit logging
- Rate limiting
- Secure location APIs
- Minimal location retention policy to be defined
- Secure mobile token storage
- Signed production firmware for ESP32 fleet use

## 17. Product design principle: progressive delivery

Do **not** implement the whole product in one flow.

We will progress in visible, testable layers:

1. Product foundation and information architecture
2. Base visual system
3. Rider app base UI
4. Driver app base UI
5. Web booking base UI
6. Operations dashboard base UI
7. Navigation and map UI
8. Booking state simulation
9. Real backend contracts
10. Real authentication
11. Real GPS
12. Real dispatch
13. Scheduling and driver queue
14. Safety/notifications
15. Integrations
16. Advanced analytics and AI
17. Production hardening

Each phase must have a clear exit criterion before the next phase is considered complete.

## 18. Progress tracking

The companion `docs/PROGRESS.md` is the implementation tracker. Every meaningful phase should record:

- Status
- Scope
- Completed items
- Remaining items
- Known issues
- Evidence/test result
- Next phase

The PRD is updated whenever a requirement or product decision changes.

## 19. Initial non-goals

Until explicitly added to the PRD, the following are not required for the first UI phases:

- Public consumer ride marketplace
- External driver onboarding marketplace
- Cash/card payment settlement
- Surge pricing
- Commission engine
- Public driver earnings
- External customer advertising

The initial product is a private Jayalakshmi transport platform.

## 20. Definition of success

The product is successful when an authorized user can:

1. Open the app or web booking page.
2. Enter a destination.
3. Request a ride now or schedule one.
4. Receive a driver through intelligent dispatch.
5. If nobody is immediately free, remain in an active search or receive the next suitable driver after their current trip.
6. See the assigned driver's live location.
7. Verify the trip with OTP.
8. Complete the trip with a complete audit trail.

The operations team must be able to observe and intervene in the entire lifecycle from the web dashboard, including dispatch, scheduling, driver queues, trip categories and incentives.
