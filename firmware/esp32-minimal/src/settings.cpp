#include "settings.h"

namespace {
Preferences *gPrefs = nullptr;
DeviceConfig gConfig;

void setDefaults() {
  gConfig.deviceId = "PVM-GATE-01";
  gConfig.deviceName = "Palarivattom Gate";
  gConfig.showroom = "Palarivattom";
  gConfig.installation = "Staff Exit";

  gConfig.busRegistration = "KL-XX-XX-0000";
  gConfig.busCapacity = 27;
  gConfig.busEnabled = false;

  gConfig.destinationCode[0] = "KAL";
  gConfig.destinationCode[1] = "VYT";
  gConfig.destinationCode[2] = "VAZ";
  gConfig.destinationName[0] = "Kaloor";
  gConfig.destinationName[1] = "Vytilla";
  gConfig.destinationName[2] = "Vazhakala";

  // All press-and-hold actions use 5 seconds by default.
  gConfig.longPressMs = 5000;
  gConfig.debounceMs = 35;
  gConfig.messageMs = 1800;

  // Network credentials are intentionally blank until configured locally.
  gConfig.wifiSsid = "";
  gConfig.wifiPassword = "";

  // Pilot default. Can be changed from the protected settings page.
  gConfig.adminPin = "binesheb@16";
}

void loadString(const char *key, String &target, const String &fallback) {
  target = gPrefs->getString(key, fallback);
}
}

void settingsBegin(Preferences &prefs) {
  gPrefs = &prefs;
  setDefaults();
  settingsLoad();
}

DeviceConfig &settings() {
  return gConfig;
}

void settingsLoad() {
  if (!gPrefs) return;

  loadString("device_id", gConfig.deviceId, gConfig.deviceId);
  loadString("device_name", gConfig.deviceName, gConfig.deviceName);
  loadString("showroom", gConfig.showroom, gConfig.showroom);
  loadString("install", gConfig.installation, gConfig.installation);

  loadString("bus_reg", gConfig.busRegistration, gConfig.busRegistration);
  gConfig.busCapacity = gPrefs->getUShort("bus_cap", gConfig.busCapacity);
  gConfig.busEnabled = gPrefs->getBool("bus_en", gConfig.busEnabled);

  loadString("d1_code", gConfig.destinationCode[0], gConfig.destinationCode[0]);
  loadString("d2_code", gConfig.destinationCode[1], gConfig.destinationCode[1]);
  loadString("d3_code", gConfig.destinationCode[2], gConfig.destinationCode[2]);
  loadString("d1_name", gConfig.destinationName[0], gConfig.destinationName[0]);
  loadString("d2_name", gConfig.destinationName[1], gConfig.destinationName[1]);
  loadString("d3_name", gConfig.destinationName[2], gConfig.destinationName[2]);

  gConfig.longPressMs = gPrefs->getUInt("long_ms", gConfig.longPressMs);
  gConfig.debounceMs = gPrefs->getUInt("debounce", gConfig.debounceMs);
  gConfig.messageMs = gPrefs->getUInt("message_ms", gConfig.messageMs);

  loadString("wifi_ssid", gConfig.wifiSsid, gConfig.wifiSsid);
  loadString("wifi_pass", gConfig.wifiPassword, gConfig.wifiPassword);
  loadString("admin_pin", gConfig.adminPin, gConfig.adminPin);
}

bool settingsSave(const DeviceConfig &incoming) {
  if (!gPrefs) return false;

  gConfig = incoming;

  gPrefs->putString("device_id", gConfig.deviceId);
  gPrefs->putString("device_name", gConfig.deviceName);
  gPrefs->putString("showroom", gConfig.showroom);
  gPrefs->putString("install", gConfig.installation);

  gPrefs->putString("bus_reg", gConfig.busRegistration);
  gPrefs->putUShort("bus_cap", gConfig.busCapacity);
  gPrefs->putBool("bus_en", gConfig.busEnabled);

  gPrefs->putString("d1_code", gConfig.destinationCode[0]);
  gPrefs->putString("d2_code", gConfig.destinationCode[1]);
  gPrefs->putString("d3_code", gConfig.destinationCode[2]);
  gPrefs->putString("d1_name", gConfig.destinationName[0]);
  gPrefs->putString("d2_name", gConfig.destinationName[1]);
  gPrefs->putString("d3_name", gConfig.destinationName[2]);

  gPrefs->putUInt("long_ms", gConfig.longPressMs);
  gPrefs->putUInt("debounce", gConfig.debounceMs);
  gPrefs->putUInt("message_ms", gConfig.messageMs);

  gPrefs->putString("wifi_ssid", gConfig.wifiSsid);
  gPrefs->putString("wifi_pass", gConfig.wifiPassword);
  gPrefs->putString("admin_pin", gConfig.adminPin);

  return true;
}

void settingsFactoryReset() {
  if (gPrefs) gPrefs->clear();
  setDefaults();
}
