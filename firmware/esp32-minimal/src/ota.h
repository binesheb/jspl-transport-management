#pragma once

// Automatic boot-time OTA updater.
// The updater runs in its own task so the counter remains independent of OTA.
void otaStart();
