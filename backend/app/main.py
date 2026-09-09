from contextlib import asynccontextmanager
from datetime import datetime, timezone
from typing import Any
from uuid import uuid4

from fastapi import Depends, FastAPI, HTTPException, status
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel, Field
from sqlalchemy import func, select
from sqlalchemy.orm import Session

from .db import Base, engine, get_db
from .models import Bus, EventType, Showroom, TransportEvent, Trip, TripStatus
from .schemas import (
    BusCreate,
    BusRead,
    EventCreate,
    EventRead,
    ShowroomCreate,
    ShowroomRead,
    TripCounts,
    TripCreate,
    TripRead,
)


# The field PoC dispatch store is intentionally lightweight. It will be replaced
# by persistent driver/booking models without changing the mobile contract.
drivers: dict[str, dict[str, Any]] = {}
bookings: dict[str, dict[str, Any]] = {}


class DriverOnlineRequest(BaseModel):
    driver_id: str = Field(min_length=1)
    name: str = Field(min_length=1)
    vehicle: str = "Vehicle"


class DriverLocationRequest(BaseModel):
    driver_id: str
    latitude: float
    longitude: float
    accuracy: float | None = None
    speed: float | None = None
    heading: float | None = None


class BookingCreateRequest(BaseModel):
    pickup: str = Field(min_length=1)
    destination: str = Field(min_length=1)
    requested_for: str = "now"


class BookingActionRequest(BaseModel):
    driver_id: str


class BookingStatusRequest(BaseModel):
    status: str


@asynccontextmanager
async def lifespan(_: FastAPI):
    Base.metadata.create_all(bind=engine)
    yield


app = FastAPI(title="JSPL Transport Management API", version="0.2.0", lifespan=lifespan)
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=False,
    allow_methods=["*"],
    allow_headers=["*"],
)


@app.get("/health")
def health() -> dict[str, str]:
    return {"status": "ok"}


@app.get("/api/poc/drivers")
def poc_drivers() -> list[dict[str, Any]]:
    return list(drivers.values())


@app.post("/api/poc/drivers/online")
def driver_online(payload: DriverOnlineRequest) -> dict[str, Any]:
    driver = drivers.get(payload.driver_id, {})
    driver.update(
        {
            "driver_id": payload.driver_id,
            "name": payload.name,
            "vehicle": payload.vehicle,
            "online": True,
            "status": "available",
            "location": driver.get("location"),
            "updated_at": datetime.now(timezone.utc).isoformat(),
        }
    )
    drivers[payload.driver_id] = driver
    return driver


@app.post("/api/poc/drivers/offline")
def driver_offline(payload: BookingActionRequest) -> dict[str, Any]:
    driver = drivers.get(payload.driver_id)
    if not driver:
        raise HTTPException(404, "Driver not found")
    driver["online"] = False
    driver["status"] = "offline"
    driver["updated_at"] = datetime.now(timezone.utc).isoformat()
    return driver


@app.post("/api/poc/drivers/location")
def driver_location(payload: DriverLocationRequest) -> dict[str, Any]:
    driver = drivers.get(payload.driver_id)
    if not driver:
        raise HTTPException(404, "Driver not online")
    driver["location"] = {
        "latitude": payload.latitude,
        "longitude": payload.longitude,
        "accuracy": payload.accuracy,
        "speed": payload.speed,
        "heading": payload.heading,
        "timestamp": datetime.now(timezone.utc).isoformat(),
    }
    driver["updated_at"] = datetime.now(timezone.utc).isoformat()
    return driver


@app.post("/api/poc/bookings", status_code=status.HTTP_201_CREATED)
def create_poc_booking(payload: BookingCreateRequest) -> dict[str, Any]:
    booking_id = f"JD-{uuid4().hex[:8].upper()}"
    booking = {
        "booking_id": booking_id,
        "pickup": payload.pickup,
        "destination": payload.destination,
        "requested_for": payload.requested_for,
        "status": "searching",
        "driver_id": None,
        "created_at": datetime.now(timezone.utc).isoformat(),
    }
    bookings[booking_id] = booking
    return booking


@app.get("/api/poc/bookings")
def list_poc_bookings() -> list[dict[str, Any]]:
    return sorted(bookings.values(), key=lambda item: item["created_at"], reverse=True)


@app.get("/api/poc/bookings/{booking_id}")
def get_poc_booking(booking_id: str) -> dict[str, Any]:
    booking = bookings.get(booking_id)
    if not booking:
        raise HTTPException(404, "Booking not found")
    return booking


@app.post("/api/poc/bookings/{booking_id}/offer")
def offer_booking(booking_id: str, payload: BookingActionRequest) -> dict[str, Any]:
    booking = bookings.get(booking_id)
    driver = drivers.get(payload.driver_id)
    if not booking:
        raise HTTPException(404, "Booking not found")
    if not driver or not driver.get("online"):
        raise HTTPException(409, "Driver is not available")
    if driver.get("status") != "available":
        raise HTTPException(409, "Driver is not available")
    booking["status"] = "offered"
    booking["offered_to"] = payload.driver_id
    driver["status"] = "offered"
    return booking


@app.post("/api/poc/bookings/{booking_id}/accept")
def accept_booking(booking_id: str, payload: BookingActionRequest) -> dict[str, Any]:
    booking = bookings.get(booking_id)
    driver = drivers.get(payload.driver_id)
    if not booking:
        raise HTTPException(404, "Booking not found")
    if not driver or driver.get("status") != "offered" or booking.get("offered_to") != payload.driver_id:
        raise HTTPException(409, "Booking offer is no longer available")
    booking["status"] = "assigned"
    booking["driver_id"] = payload.driver_id
    driver["status"] = "assigned"
    return booking


@app.post("/api/poc/bookings/{booking_id}/decline")
def decline_booking(booking_id: str, payload: BookingActionRequest) -> dict[str, Any]:
    booking = bookings.get(booking_id)
    driver = drivers.get(payload.driver_id)
    if not booking or not driver:
        raise HTTPException(404, "Booking or driver not found")
    if booking.get("offered_to") == payload.driver_id and booking.get("status") == "offered":
        driver["status"] = "available"
        booking["status"] = "searching"
        booking.pop("offered_to", None)
    return booking


@app.post("/api/poc/bookings/{booking_id}/status")
def update_poc_booking(booking_id: str, payload: BookingStatusRequest) -> dict[str, Any]:
    booking = bookings.get(booking_id)
    if not booking:
        raise HTTPException(404, "Booking not found")
    allowed = {"assigned", "driver_en_route", "arrived", "in_trip", "completed", "cancelled"}
    if payload.status not in allowed:
        raise HTTPException(422, "Unsupported booking status")
    booking["status"] = payload.status
    if payload.status == "completed" and booking.get("driver_id") in drivers:
        drivers[booking["driver_id"]]["status"] = "available"
    return booking


@app.post("/api/showrooms", response_model=ShowroomRead, status_code=status.HTTP_201_CREATED)
def create_showroom(payload: ShowroomCreate, db: Session = Depends(get_db)) -> Showroom:
    existing = db.scalar(select(Showroom).where(Showroom.code == payload.code))
    if existing:
        raise HTTPException(status_code=409, detail="Showroom code already exists")
    showroom = Showroom(code=payload.code, name=payload.name)
    db.add(showroom)
    db.commit()
    db.refresh(showroom)
    return showroom


@app.get("/api/showrooms", response_model=list[ShowroomRead])
def list_showrooms(db: Session = Depends(get_db)) -> list[Showroom]:
    return list(db.scalars(select(Showroom).order_by(Showroom.code)))


@app.post("/api/buses", response_model=BusRead, status_code=status.HTTP_201_CREATED)
def create_bus(payload: BusCreate, db: Session = Depends(get_db)) -> Bus:
    existing = db.scalar(select(Bus).where(Bus.registration_number == payload.registration_number))
    if existing:
        raise HTTPException(status_code=409, detail="Bus registration already exists")
    bus = Bus(registration_number=payload.registration_number)
    db.add(bus)
    db.commit()
    db.refresh(bus)
    return bus


@app.get("/api/buses", response_model=list[BusRead])
def list_buses(db: Session = Depends(get_db)) -> list[Bus]:
    return list(db.scalars(select(Bus).order_by(Bus.registration_number)))


@app.post("/api/trips", response_model=TripRead, status_code=status.HTTP_201_CREATED)
def create_trip(payload: TripCreate, db: Session = Depends(get_db)) -> Trip:
    if not db.get(Showroom, payload.showroom_id):
        raise HTTPException(status_code=404, detail="Showroom not found")
    if not db.get(Bus, payload.bus_id):
        raise HTTPException(status_code=404, detail="Bus not found")
    trip = Trip(showroom_id=payload.showroom_id, bus_id=payload.bus_id, route_name=payload.route_name)
    db.add(trip)
    db.commit()
    db.refresh(trip)
    return trip


@app.get("/api/trips", response_model=list[TripRead])
def list_trips(db: Session = Depends(get_db)) -> list[Trip]:
    return list(db.scalars(select(Trip).order_by(Trip.started_at.desc().nullslast(), Trip.id.desc())))


@app.post("/api/trips/{trip_id}/start", response_model=TripRead)
def start_trip(trip_id: str, db: Session = Depends(get_db)) -> Trip:
    trip = db.get(Trip, trip_id)
    if not trip:
        raise HTTPException(status_code=404, detail="Trip not found")
    if trip.status != TripStatus.PLANNED.value:
        raise HTTPException(status_code=409, detail="Trip is not in planned state")
    trip.status = TripStatus.BOARDING.value
    trip.started_at = datetime.now(timezone.utc)
    db.commit()
    db.refresh(trip)
    return trip


@app.post("/api/events", response_model=EventRead, status_code=status.HTTP_201_CREATED)
def ingest_event(payload: EventCreate, db: Session = Depends(get_db)) -> TransportEvent:
    if not db.get(Trip, payload.trip_id):
        raise HTTPException(status_code=404, detail="Trip not found")
    existing = db.scalar(select(TransportEvent).where(TransportEvent.event_id == payload.event_id))
    if existing:
        return existing
    event = TransportEvent(**payload.model_dump())
    db.add(event)
    db.commit()
    db.refresh(event)
    return event


@app.get("/api/trips/{trip_id}/counts", response_model=TripCounts)
def trip_counts(trip_id: str, db: Session = Depends(get_db)) -> TripCounts:
    trip = db.get(Trip, trip_id)
    if not trip:
        raise HTTPException(status_code=404, detail="Trip not found")
    rows = db.execute(
        select(TransportEvent.event_type, func.count(TransportEvent.id))
        .where(TransportEvent.trip_id == trip_id)
        .group_by(TransportEvent.event_type)
    ).all()
    counts = {event_type: count for event_type, count in rows}
    expected = int(counts.get(EventType.STAFF_EXIT.value, 0))
    boarded = int(counts.get(EventType.STAFF_BOARD.value, 0))
    exited = int(counts.get(EventType.STAFF_EXIT_BUS.value, 0))
    return TripCounts(
        trip_id=trip_id,
        expected=expected,
        boarded=boarded,
        exited=exited,
        remaining=max(expected - boarded, 0),
    )
