from .db import Base, SessionLocal, engine
from .models import Bus, Showroom


PVM_BUSES = [
    "KL-01-AB-1001",
    "KL-01-AB-1002",
    "KL-01-AB-1003",
]


def seed() -> None:
    Base.metadata.create_all(bind=engine)
    db = SessionLocal()
    try:
        showroom = next((item for item in db.query(Showroom).all() if item.code == "PVM"), None)
        if showroom is None:
            db.add(Showroom(code="PVM", name="Palarivattom"))

        existing = {item.registration_number for item in db.query(Bus).all()}
        for registration in PVM_BUSES:
            if registration not in existing:
                db.add(Bus(registration_number=registration))
        db.commit()
    finally:
        db.close()


if __name__ == "__main__":
    seed()
    print("PVM development data seeded")
