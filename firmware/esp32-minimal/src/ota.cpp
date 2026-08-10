#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Update.h>
#include "settings.h"
#include "version.h"
#include "ota.h"

extern WebServer server;

namespace {
constexpr char VERSION_URL[] = "https://github.com/binesheb/jspl-transport-management/releases/latest/download/version.txt";
constexpr char FIRMWARE_URL[] = "https://github.com/binesheb/jspl-transport-management/releases/latest/download/firmware.bin";
constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 8000;
constexpr uint32_t BOOT_WAIT_MS = 3500;
constexpr uint32_t OTA_HTTP_TIMEOUT_MS = 15000;

bool versionNewer(const String &remote, const String &local) {
  int r[3] = {0, 0, 0};
  int l[3] = {0, 0, 0};
  if (sscanf(remote.c_str(), "%d.%d.%d", &r[0], &r[1], &r[2]) != 3) return false;
  if (sscanf(local.c_str(), "%d.%d.%d", &l[0], &l[1], &l[2]) != 3) return false;
  for (int i = 0; i < 3; ++i) {
    if (r[i] != l[i]) return r[i] > l[i];
  }
  return false;
}

String htmlEscape(String s) {
  s.replace("&", "&amp;");
  s.replace("<", "&lt;");
  s.replace(">", "&gt;");
  s.replace("\"", "&quot;");
  return s;
}

bool connectInternet() {
  if (settings().wifiSsid.isEmpty()) {
    Serial.println("[OTA] No Internet Wi-Fi configured; skipping OTA.");
    return false;
  }

  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(settings().wifiSsid.c_str(), settings().wifiPassword.c_str());
  Serial.print("[OTA] Connecting to configured Wi-Fi");

  const uint32_t started = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - started < WIFI_CONNECT_TIMEOUT_MS) {
    Serial.print('.');
    delay(250);
  }
  Serial.println();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[OTA] Wi-Fi unavailable; continuing with current firmware.");
    return false;
  }

  Serial.print("[OTA] Internet Wi-Fi connected. IP: ");
  Serial.println(WiFi.localIP());
  return true;
}

String fetchText(const char *url) {
  WiFiClientSecure client;
  client.setInsecure(); // Prototype only; production will use certificate verification.

  HTTPClient http;
  http.setConnectTimeout(OTA_HTTP_TIMEOUT_MS);
  http.setTimeout(OTA_HTTP_TIMEOUT_MS);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  if (!http.begin(client, url)) return String();

  int code = http.GET();
  String body;
  if (code == HTTP_CODE_OK) body = http.getString();
  http.end();
  body.trim();
  return body;
}

bool performUpdate(const String &remoteVersion) {
  WiFiClientSecure client;
  client.setInsecure(); // Prototype only; production will use certificate verification.

  HTTPClient http;
  http.setConnectTimeout(OTA_HTTP_TIMEOUT_MS);
  http.setTimeout(OTA_HTTP_TIMEOUT_MS);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  if (!http.begin(client, FIRMWARE_URL)) return false;

  int code = http.GET();
  if (code != HTTP_CODE_OK) {
    Serial.printf("[OTA] Firmware download HTTP error: %d\n", code);
    http.end();
    return false;
  }

  int total = http.getSize();
  if (total <= 0) {
    Serial.println("[OTA] Firmware size missing; refusing update.");
    http.end();
    return false;
  }

  if (!Update.begin(total)) {
    Serial.printf("[OTA] Update.begin failed: %s\n", Update.errorString());
    http.end();
    return false;
  }

  Serial.printf("[OTA] Installing %s (%d bytes)\n", remoteVersion.c_str(), total);
  WiFiClient *stream = http.getStreamPtr();
  uint8_t buffer[2048];
  size_t written = 0;

  while (http.connected() && written < (size_t)total) {
    size_t available = stream->available();
    if (available) {
      size_t toRead = min(available, sizeof(buffer));
      int readNow = stream->readBytes(buffer, toRead);
      if (readNow > 0) {
        size_t result = Update.write(buffer, readNow);
        if (result != (size_t)readNow) {
          Serial.printf("[OTA] Flash write failed: %s\n", Update.errorString());
          Update.abort();
          http.end();
          return false;
        }
        written += result;
      }
    } else {
      delay(1);
    }
  }

  http.end();

  if (written != (size_t)total || !Update.end(true)) {
    Serial.printf("[OTA] Update validation failed: %s\n", Update.errorString());
    return false;
  }

  Serial.println("[OTA] Firmware verified. Rebooting into new release...");
  delay(500);
  ESP.restart();
  return true;
}

void checkForUpdate() {
  if (!connectInternet()) return;

  Serial.printf("[OTA] Current firmware: %s\n", JSPL_FW_VERSION);
  Serial.println("[OTA] Checking latest GitHub release...");

  String remoteVersion = fetchText(VERSION_URL);
  if (remoteVersion.isEmpty()) {
    Serial.println("[OTA] No release version found; continuing normally.");
    return;
  }

  if (!versionNewer(remoteVersion, JSPL_FW_VERSION)) {
    Serial.printf("[OTA] Firmware is current (%s).\n", JSPL_FW_VERSION);
    return;
  }

  Serial.printf("[OTA] New release available: %s\n", remoteVersion.c_str());
  performUpdate(remoteVersion);
}

String networkPage(const String &notice = "") {
  String h = R"HTML(<!doctype html><html><head><meta name="viewport" content="width=device-width,initial-scale=1"><title>Network & OTA</title><style>
:root{color-scheme:dark}body{font-family:system-ui;margin:0;padding:18px;background:#080b12;color:#fff}.wrap{max-width:560px;margin:auto}.panel{background:#151a24;border:1px solid #252c3a;border-radius:14px;padding:16px;margin-bottom:12px}h1{font-size:22px}label{display:block;font-size:12px;opacity:.7;margin-top:12px}input{box-sizing:border-box;width:100%;padding:11px;margin-top:5px;border-radius:9px;border:1px solid #333c4c;background:#0d1119;color:#fff}button,a{display:block;width:100%;box-sizing:border-box;padding:12px;margin-top:12px;border:0;border-radius:9px;background:#fff;color:#080b12;text-align:center;text-decoration:none;font-weight:800}.note{padding:10px;border-radius:9px;background:#10241f;color:#8ff0d5;font-size:13px}</style></head><body><div class="wrap"><h1>Network & OTA</h1>)HTML";
  if (!notice.isEmpty()) h += "<div class='note'>" + htmlEscape(notice) + "</div>";
  h += "<div class='panel'><form method='POST' action='/api/network/save'>";
  h += "<label>Wi-Fi SSID</label><input name='ssid' value='" + htmlEscape(settings().wifiSsid) + "'>";
  h += "<label>Wi-Fi Password</label><input type='password' name='password' value='" + htmlEscape(settings().wifiPassword) + "'>";
  h += "<label>Administrator PIN</label><input type='password' name='pin' required>";
  h += "<button type='submit'>SAVE NETWORK</button></form></div>";
  h += "<div class='panel'><b>Firmware</b><p>Current: " + String(JSPL_FW_VERSION) + "</p>";
  h += "<form method='POST' action='/api/ota/check'><label>Administrator PIN</label><input type='password' name='pin' required><button type='submit'>CHECK FOR UPDATE NOW</button></form>";
  h += "<p>Automatic update check runs after every boot when Internet Wi-Fi is configured.</p></div>";
  h += "<a href='/settings'>Device Settings</a><a href='/'>Dashboard</a></div></body></html>";
  return h;
}

bool adminOK() {
  return server.hasArg("pin") && server.arg("pin") == settings().adminPin;
}

void registerRoutes() {
  server.on("/network", HTTP_GET, []() { server.send(200, "text/html", networkPage()); });

  server.on("/api/network/save", HTTP_POST, []() {
    if (!adminOK()) { server.send(403, "text/plain", "Invalid admin PIN"); return; }
    DeviceConfig c = settings();
    c.wifiSsid = server.arg("ssid");
    c.wifiPassword = server.arg("password");
    c.wifiSsid.trim();
    c.wifiPassword.trim();
    settingsSave(c);
    server.send(200, "text/html", networkPage("Network credentials saved. Reboot the device to run the boot-time OTA check with the new network."));
  });

  server.on("/api/ota/check", HTTP_POST, []() {
    if (!adminOK()) { server.send(403, "text/plain", "Invalid admin PIN"); return; }
    server.send(200, "text/plain", "OTA check started. The device will reboot only if a newer release is available.");
    delay(100);
    checkForUpdate();
  });
}

void otaTask(void *) {
  delay(BOOT_WAIT_MS);
  registerRoutes();
  checkForUpdate();
  for (;;) vTaskDelay(pdMS_TO_TICKS(60000));
}
}

void otaStart() {
  static bool started = false;
  if (started) return;
  started = true;
  xTaskCreate(otaTask, "ota_task", 8192, nullptr, 1, nullptr);
}

struct OtaAutoStart {
  OtaAutoStart() { otaStart(); }
};
OtaAutoStart autoStart;
