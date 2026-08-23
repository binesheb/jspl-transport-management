# Changelog

All notable changes to JSPL Transport Management are documented here.

The project follows Semantic Versioning (`MAJOR.MINOR.PATCH`).

## [Unreleased]

## [0.1.2] - 2026-08-23

### Added
- Added a FastAPI `/health` smoke test as a regression guard for the backend service lifecycle.

## [0.1.1] - 2026-08-20

### Fixed
- Aligned the documented Docker configuration with `.env` so deployments can change database and MQTT settings without editing `docker-compose.yml`.

### Documentation
- Established a canonical changelog and release baseline for incremental releases.

## [0.1.0]

### Added
- Initial Palarivattom ESP32 staff-readiness counter prototype.
- Local device configuration, event-based transport workflow, and OTA-capable firmware foundation.
- Initial Docker deployment structure for the future backend, MQTT, database, and dashboard stack.
