from fastapi.testclient import TestClient

from app.main import app, bookings, drivers


def test_manager_driver_field_poc_lifecycle() -> None:
    driver_id = "test-driver-field-poc"
    with TestClient(app) as client:
        drivers.pop(driver_id, None)
        response = client.post(
            "/api/poc/drivers/online",
            json={"driver_id": driver_id, "name": "Test Driver", "vehicle": "Test Vehicle"},
        )
        assert response.status_code == 200
        assert response.json()["status"] == "available"

        response = client.post(
            "/api/poc/bookings",
            json={"pickup": "MG Road", "destination": "Airport"},
        )
        assert response.status_code == 201
        booking_id = response.json()["booking_id"]

        response = client.post(
            f"/api/poc/bookings/{booking_id}/offer",
            json={"driver_id": driver_id},
        )
        assert response.status_code == 200
        assert response.json()["status"] == "offered"

        response = client.post(
            f"/api/poc/bookings/{booking_id}/accept",
            json={"driver_id": driver_id},
        )
        assert response.status_code == 200
        assert response.json()["status"] == "assigned"
        assert response.json()["driver_id"] == driver_id

        response = client.post(
            f"/api/poc/bookings/{booking_id}/status",
            json={"status": "completed"},
        )
        assert response.status_code == 200
        assert response.json()["status"] == "completed"
        assert drivers[driver_id]["status"] == "available"
        bookings.pop(booking_id, None)
        drivers.pop(driver_id, None)
