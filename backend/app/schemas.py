from datetime import datetime

from pydantic import BaseModel, ConfigDict


class ShowroomCreate(BaseModel):
    code: str
    name: str


class ShowroomRead(ShowroomCreate):
    id: str
    active: bool
    model_config = ConfigDict(from_attributes=True)


class BusCreate(BaseModel):
    registration_number: str


class BusRead(BusCreate):
    id: str
    active: bool
    model_config = ConfigDict(from_attributes=True)


class TripCreate(BaseModel):
    showroom_id: str
    bus_id: str
    route_name: str | None = None


class TripRead(TripCreate):
    id: str
    status: str
    started_at: datetime | None
    departed_at: datetime | None
    completed_at: datetime | None
    model_config = ConfigDict(from_attributes=True)


class EventCreate(BaseModel):
    event_id: str
    trip_id: str
    event_type: str
    device_code: str
    occurred_at: datetime
    payload: str | None = None


class EventRead(EventCreate):
    id: str
    model_config = ConfigDict(from_attributes=True)


class TripCounts(BaseModel):
    trip_id: str
    expected: int
    boarded: int
    exited: int
    remaining: int
