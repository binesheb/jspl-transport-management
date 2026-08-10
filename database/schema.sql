CREATE EXTENSION IF NOT EXISTS pgcrypto;

CREATE TABLE showrooms (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    code VARCHAR(20) NOT NULL UNIQUE,
    name VARCHAR(120) NOT NULL,
    active BOOLEAN NOT NULL DEFAULT TRUE,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE TABLE buses (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    registration_number VARCHAR(30) NOT NULL UNIQUE,
    capacity INTEGER NOT NULL DEFAULT 40 CHECK (capacity > 0),
    active BOOLEAN NOT NULL DEFAULT TRUE,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE TABLE devices (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    device_code VARCHAR(100) NOT NULL UNIQUE,
    device_type VARCHAR(30) NOT NULL CHECK (device_type IN ('EXIT_COUNTER', 'BUS_UNIT')),
    showroom_id UUID REFERENCES showrooms(id),
    bus_id UUID REFERENCES buses(id),
    firmware_version VARCHAR(40),
    last_seen_at TIMESTAMPTZ,
    active BOOLEAN NOT NULL DEFAULT TRUE,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    CHECK (
        (device_type = 'EXIT_COUNTER' AND showroom_id IS NOT NULL AND bus_id IS NULL)
        OR
        (device_type = 'BUS_UNIT' AND bus_id IS NOT NULL AND showroom_id IS NULL)
    )
);

CREATE TABLE trips (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    showroom_id UUID NOT NULL REFERENCES showrooms(id),
    bus_id UUID NOT NULL REFERENCES buses(id),
    service_date DATE NOT NULL,
    route_name VARCHAR(200),
    status VARCHAR(30) NOT NULL DEFAULT 'READY'
        CHECK (status IN ('READY', 'BOARDING', 'BOARDING_COMPLETE', 'DEPARTED', 'COMPLETED', 'CANCELLED')),
    boarding_opened_at TIMESTAMPTZ,
    departed_at TIMESTAMPTZ,
    completed_at TIMESTAMPTZ,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE TABLE transport_events (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    event_id UUID NOT NULL UNIQUE,
    trip_id UUID NOT NULL REFERENCES trips(id),
    device_id UUID NOT NULL REFERENCES devices(id),
    event_type VARCHAR(40) NOT NULL
        CHECK (event_type IN ('STAFF_EXIT', 'BOARD', 'STAFF_EXIT_BUS', 'TRIP_DEPARTED', 'TRIP_COMPLETED', 'CORRECTION')),
    occurred_at TIMESTAMPTZ NOT NULL,
    sequence_number BIGINT,
    metadata JSONB NOT NULL DEFAULT '{}'::jsonb,
    received_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX idx_devices_last_seen ON devices(last_seen_at);
CREATE INDEX idx_trips_service_date ON trips(service_date);
CREATE INDEX idx_trips_bus_status ON trips(bus_id, status);
CREATE INDEX idx_events_trip_type ON transport_events(trip_id, event_type);
CREATE INDEX idx_events_occurred_at ON transport_events(occurred_at);

INSERT INTO showrooms (code, name)
VALUES ('PVM', 'Palarivattom')
ON CONFLICT (code) DO NOTHING;

INSERT INTO buses (registration_number, capacity)
VALUES
    ('KL-01-AB-1001', 40),
    ('KL-01-AB-1002', 40),
    ('KL-01-AB-1003', 40)
ON CONFLICT (registration_number) DO NOTHING;
