# Jayalakshmi DRIVE — Dashboard Specification

## Purpose
The dashboard is the transport control center for operations and management. It is not merely an administration page.

## Primary navigation

- Overview
- Live Fleet
- Dispatch
- Bookings
- Scheduled Trips
- Drivers
- Vehicles
- Trip Categories
- Incentives
- Branches
- Alerts
- Reports
- Settings
- Audit Log

## Overview KPIs

- Online drivers
- Available drivers
- On-trip drivers
- Available-soon drivers
- Unassigned bookings
- Scheduled bookings
- Delayed trips
- Active alerts
- Active vehicles

## Live Fleet

Map-first view with driver/vehicle markers. Selecting a marker opens a detail panel containing:

- Driver
- Vehicle
- Status
- Current location
- GPS freshness
- Current trip
- Next trip
- ETA
- Today's trip count
- Actions

## Dispatch

For every unassigned booking show:

- Pickup
- Destination
- Requested time
- Category
- Priority
- Candidate drivers
- ETA to pickup
- Current/future assignment conflict
- Dispatch wave
- Offer status

Actions:

- Auto dispatch
- Manual assign
- Reassign
- Escalate
- Cancel

## Scheduled Trips

Provide timeline/calendar and list views. Show assigned/unassigned state and driver availability forecast.

## Driver queue

Show current and future driver commitments:

`Current → Next → Following`

Allow authorized manual reservation subject to conflict validation.

## Trip Categories

CRUD/configuration screen for category code, name, incentive, operating window, eligibility rule, vehicle/branch restrictions, priority and effective dates.

## Incentives

Dashboard with totals by date range, driver, branch and category. Every amount links back to a trip and incentive rule version.

## Alerts

Prioritize unassigned, delayed, emergency, GPS-offline and scheduled-without-driver conditions.

## Permissions

Use role-based visibility and actions for Super Admin, Transport Manager, Branch Manager, Dispatcher and Finance.

## Responsive behaviour

Desktop is the primary operations target. Tablet should remain usable. Mobile dashboard is a compact operational view rather than a full desktop clone.

## Phase 02 limitation

This specification describes UI and interaction requirements. Phase 02 uses mock data. Real GPS, dispatch, authentication and incentive calculation are later phases.
