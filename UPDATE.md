# Updating JSPL Transport Management

The project has two distinct update paths: **ESP32 firmware** and the **server/dashboard stack**. Keep configuration and production secrets outside the Git checkout before updating.

## ESP32 firmware

### Automatic OTA

After the first USB installation, a compatible gate device can check GitHub Releases and install a newer firmware build into the inactive OTA slot. The device verifies the published SHA-256 before installation. If no update or network connection is available, the running firmware continues normally.

For production fleet use, unsigned release assets are not sufficient by themselves. Authenticated firmware, certificate validation, and post-update health/rollback confirmation remain required before broad deployment.

### Manual USB update

Use the Arduino IDE with the versions documented in `Readme.md`, open:

`firmware/arduino/JSPL_PVM_Gate/JSPL_PVM_Gate.ino`

Select the correct ESP32 board and serial port, compile, then upload. Confirm the device starts and the displayed firmware version/configuration are correct before returning it to service.

## Server and dashboard

Use a safe Git-based update so local changes are never silently overwritten:

```text
git fetch origin main
git status --short
git diff --exit-code || stop and review local changes
git merge --ff-only origin/main
docker compose pull
docker compose up -d --remove-orphans
```

Run the repository's documented database migrations or schema updates, if applicable, before declaring the deployment healthy. Verify the API, dashboard, and any required device connectivity after the update.

### Rollback

Keep the previous known-good Git commit and container images available. If an update fails health checks, return to the previous commit and redeploy rather than force-resetting over unexplained local changes. For firmware, keep the previous validated release available and use the ESP32 OTA rollback strategy where supported.

## Release policy

Firmware and deployable changes use semantic versions in the form `MAJOR.MINOR.PATCH` with Git tags such as `v0.1.0`. Patch releases are for compatible fixes, minor releases for backward-compatible features, and major releases for breaking protocol/configuration changes.
