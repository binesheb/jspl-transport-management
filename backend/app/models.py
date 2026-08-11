from datetime import datetime
from enum import StrEnum
from uuid import uuid4

from sqlalchemy import DateTime, ForeignKey, String, Text, UniqueConstraint
from sqlalchemy.orm import Mapped, mapped_column

from .db import Base


class DeviceType(StrEnum):
    EXIT_COUNTER = "exit_counter"
    BUS_UNIT = "bus_unit"


class TripStatus(StrEnum):
    PLANNED = "planned"
    BOARDING = "boarding"
    BOARDING_COMPLETE = "boarding_complete"
    DEPARTED = "departed"
    COMPLETED = "completed"
    CANCELLED = "cancelled"


class EventType(StrEnum):
    STAFF_EXIT = "staff_exit"
    STAFF_BOARD = "staff_board"
    STAFF_EXIT_BUS = "staff_exit_bus"
    TRIP_STARTED = "trip_started"
    BOARDING_CLOSED = "boarding_closed"
    BUS_DEPARTED = "bus_departed"


class Showroom(Base):
    __tablename__ = "showrooms"

    id: Mapped[str] = mapped_column(String(36), primary_key=True, default=lambda: str(uuid4()))
    code: Mapped[str] = mapped_column(String(20), unique=True, nullable=False)
    name: Mapped[str] = mapped_column(String(120), nullable=False)
    active: Mapped[bool] = mapped_column(default=True, nullable=False)


class Bus(Base):
    __tablename__ = "buses"

    id: Mapped[str] = mapped_column(String(36), primary_key=True, default=lambda: str(uuid4()))
    registration_number: Mapped[str] = mapped_column(String(30), unique=True, nullable=False)
    active: Mapped[bool] = mapped_column(default=True, nullable=False)


class Device(Base):
    __tablename__ = "devices"

    id: Mapped[str] = mapped_column(String(36), primary_key=True, default=lambda: str(uuid4()))
    device_code: Mapped[str] = mapped_column(String(80), unique=True, nullable=False)
    device_type: Mapped[str] = mapped_column(String(30), nullable=False)
    showroom_id: Mapped[str | None] = mapped_column(ForeignKey("showrooms.id"), nullable=True)
    bus_id: Mapped[str | None] = mapped_column(ForeignKey("buses.id"), nullable=True)
    last_seen_at: Mapped[datetime | None] = mapped_column(DateTime(timezone=True))
    active: Mapped[bool] = mapped_column(default=True, nullable=False)


class Trip(Base):
    __tablename__ = "trips"

    id: Mapped[str] = mapped_column(String(36), primary_key=True, default=lambda: str(uuid4()))
    showroom_id: Mapped[str] = mapped_column(ForeignKey("showrooms.id"), nullable=False)
    bus_id: Mapped[str] = mapped_column(ForeignKey("buses.id"), nullable=False)
    status: Mapped[str] = mapped_column(String(30), default=TripStatus.PLANNED.value, nullable=False)
    route_name: Mapped[str | None] = mapped_column(String(160))
    started_at: Mapped[datetime | None] = mapped_column(DateTime(timezone=True))
    departed_at: Mapped[datetime | None] = mapped_column(DateTime(timezone=True))
    completed_at: Mapped[datetime | None] = mapped_column(DateTime(timezone=True))


class TransportEvent(Base):
    __tablename__ = "transport_events"
    __table_args__ = (UniqueConstraint("event_id", name="uq_transport_events_event_id"),)

    id: Mapped[str] = mapped_column(String(36), primary_key=True, default=lambda: str(uuid4()))
    event_id: Mapped[str] = mapped_column(String(80), nullable=False)
    trip_id: Mapped[str] = mapped_column(ForeignKey("trips.id"), nullable=False)
    event_type: Mapped[str] = mapped_column(String(40), nullable=False)
    device_code: Mapped[str] = mapped_column(String(80), nullable=False)
    occurred_at: Mapped[datetime] = mapped_column(DateTime(timezone=True), nullable=False)
    payload: Mapped[str | None] = mapped_column(Text)
