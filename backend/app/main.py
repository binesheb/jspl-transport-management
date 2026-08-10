from datetime import datetime, timezone

from fastapi import Depends, FastAPI, HTTPException, status
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

app = FastAPI(title="JSPL Transport Management API", version="0.1.0")


@app.on_event("startup")
def startup() -> None:
    Base.metadata.create_all(bind=engine)


@app.get("/health")
def health() -> dict[str, str]:
    return {"status": "ok"}


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
    trip = Trip(
        showroom_id=payload.showroom_id,
        bus_id=payload.bus_id,
        route_name=payload.route_name,
    )
    db.add(trip)
    db.commit()
    db.refresh(trip)
    return trip


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
