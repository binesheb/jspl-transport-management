/*
 * JSPL Palarivattom V1 - Arduino IDE entry point
 *
 * This sketch reuses the same firmware source used by the PlatformIO build.
 * Open THIS file in Arduino IDE:
 *
 *   firmware/arduino/JSPL_PVM_Gate/JSPL_PVM_Gate.ino
 *
 * The implementation remains in firmware/esp32-minimal/src so there is only
 * one source of truth for the firmware logic.
 */

// The source files are included here deliberately so Arduino IDE can build
// the same firmware without converting the project to a separate codebase.
#include "../../esp32-minimal/src/main.cpp"
#include "../../esp32-minimal/src/settings.cpp"
#include "../../esp32-minimal/src/ota.cpp"
