#pragma once

#include <Arduino.h>
#include <Preferences.h>

struct DeviceConfig {
  String deviceId;
  String deviceName;
  String showroom;
  String installation;

  String busRegistration;
  uint16_t busCapacity;
  bool busEnabled;

  String destinationCode[3];
  String destinationName[3];

  uint32_t longPressMs;
  uint32_t debounceMs;
  uint32_t messageMs;

  // Network credentials are stored locally in NVS and never committed to GitHub.
  String wifiSsid;
  String wifiPassword;

  String adminPin;
};

void settingsBegin(Preferences &prefs);
DeviceConfig &settings();
void settingsLoad();
bool settingsSave(const DeviceConfig &incoming);
void settingsFactoryReset();
