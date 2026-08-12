#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Update.h>
#include <mbedtls/sha256.h>
#include "settings.h"
#include "version.h"
#include "ota.h"

extern WebServer server;

namespace {
constexpr char VERSION_URL[] = "https://github.com/binesheb/jspl-transport-management/releases/latest/download/ota-version.txt";
constexpr char FIRMWARE_URL[] = "https://github.com/binesheb/jspl-transport-management/releases/latest/download/firmware.bin";
constexpr char CHECKSUM_URL[] = "https://github.com/binesheb/jspl-transport-management/releases/latest/download/firmware.sha256";
constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 8000;
constexpr uint32_t BOOT_WAIT_MS = 2500;
constexpr uint32_t OTA_HTTP_TIMEOUT_MS = 15000;

bool versionNewer(const String &remote, const String &local) {
  int r[3] = {0,0,0}, l[3] = {0,0,0};
  if (sscanf(remote.c_str(), "%d.%d.%d", &r[0], &r[1], &r[2]) != 3) return false;
  if (sscanf(local.c_str(), "%d.%d.%d", &l[0], &l[1], &l[2]) != 3) return false;
  for (int i=0; i<3; ++i) {
    if (r[i] != l[i]) return r[i] > l[i];
  }
  return false;
}

String htmlEscape(String s) {
  s.replace("&", "&amp;"); s.replace("<", "&lt;"); s.replace(">", "&gt;"); s.replace("\"", "&quot;");
  return s;
}

bool connectInternet() {
  if (settings().wifiSsid.isEmpty()) return false;
  if (WiFi.status() == WL_CONNECTED) return true;

  WiFi.begin(settings().wifiSsid.c_str(), settings().wifiPassword.c_str());
  const uint32_t started = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - started < WIFI_CONNECT_TIMEOUT_MS) delay(200);
  return WiFi.status() == WL_CONNECTED;
}

String fetchText(const char *url) {
  WiFiClientSecure client;
  client.setInsecure(); // Prototype; production will use certificate verification.
  HTTPClient http;
  http.setConnectTimeout(OTA_HTTP_TIMEOUT_MS);
  http.setTimeout(OTA_HTTP_TIMEOUT_MS);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  if (!http.begin(client, url)) return String();
  const int status = http.GET();
  String body;
  if (status == HTTP_CODE_OK) body = http.getString();
  http.end();
  body.trim();
  return body;
}

String checksumToken(String value) {
  value.trim();
  const int space = value.indexOf(' ');
  if (space > 0) value = value.substring(0, space);
  const int tab = value.indexOf('\t');
  if (tab > 0) value = value.substring(0, tab);
  value.trim();
  value.toLowerCase();
  return value;
}

String bytesToHex(const uint8_t *bytes, size_t length) {
  const char hex[] = "0123456789abcdef";
  String result;
  result.reserve(length * 2);
  for (size_t i=0; i<length; ++i) {
    result += hex[(bytes[i] >> 4) & 0x0F];
    result += hex[bytes[i] & 0x0F];
  }
  return result;
}

bool performUpdate(const String &remoteVersion, const String &expectedChecksum) {
  WiFiClientSecure client;
  client.setInsecure(); // Prototype only.
  HTTPClient http;
  http.setConnectTimeout(OTA_HTTP_TIMEOUT_MS);
  http.setTimeout(OTA_HTTP_TIMEOUT_MS);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  if (!http.begin(client, FIRMWARE_URL)) return false;

  const int status = http.GET();
  if (status != HTTP_CODE_OK) {
    http.end();
    return false;
  }

  const int total = http.getSize();
  if (total <= 0 || !Update.begin((size_t)total)) {
    http.end();
    return false;
  }

  mbedtls_sha256_context sha;
  mbedtls_sha256_init(&sha);
  // Arduino-ESP32 3.x ships a newer mbedTLS API; use the non-_ret names.
  if (mbedtls_sha256_starts(&sha, 0) != 0) {
    mbedtls_sha256_free(&sha);
    Update.abort();
    http.end();
    return false;
  }

  WiFiClient *stream = http.getStreamPtr();
  uint8_t buffer[2048];
  size_t written = 0;

  while (http.connected() && written < (size_t)total) {
    const size_t available = stream->available();
    if (!available) { delay(1); continue; }
    const size_t toRead = min(available, sizeof(buffer));
    const int readNow = stream->readBytes(buffer, toRead);
    if (readNow <= 0) continue;

    if (mbedtls_sha256_update(&sha, buffer, (size_t)readNow) != 0) {
      mbedtls_sha256_free(&sha); Update.abort(); http.end(); return false;
    }
    const size_t result = Update.write(buffer, (size_t)readNow);
    if (result != (size_t)readNow) {
      mbedtls_sha256_free(&sha); Update.abort(); http.end(); return false;
    }
    written += result;
  }

  http.end();

  uint8_t digest[32];
  const bool hashOK = written == (size_t)total && mbedtls_sha256_finish(&sha, digest) == 0;
  mbedtls_sha256_free(&sha);
  if (!hashOK) { Update.abort(); return false; }

  const String actualChecksum = bytesToHex(digest, sizeof(digest));
  if (expectedChecksum.length() != 64 || actualChecksum != expectedChecksum) {
    Serial.println("[OTA] SHA-256 mismatch; update rejected.");
    Update.abort();
    return false;
  }

  if (!Update.end(true)) {
    Serial.printf("[OTA] Update validation failed: %s\n", Update.errorString());
    return false;
  }

  Serial.printf("[OTA] Installed %s; rebooting.\n", remoteVersion.c_str());
  delay(500);
  ESP.restart();
  return true;
}

void checkForUpdate() {
  if (!connectInternet()) {
    Serial.println("[OTA] Internet unavailable; continuing with current firmware.");
    return;
  }

  const String remoteVersion = checksumToken(fetchText(VERSION_URL));
  if (remoteVersion.isEmpty()) {
    Serial.println("[OTA] No GitHub release available; continuing normally.");
    return;
  }

  Serial.printf("[OTA] Current %s / Latest %s\n", JSPL_FW_VERSION, remoteVersion.c_str());
  if (!versionNewer(remoteVersion, JSPL_FW_VERSION)) return;

  const String checksum = checksumToken(fetchText(CHECKSUM_URL));
  if (checksum.length() != 64) {
    Serial.println("[OTA] Invalid release checksum; update rejected.");
    return;
  }
  performUpdate(remoteVersion, checksum);
}

String networkPage(const String &notice = "") {
  String h = R"HTML(<!doctype html><html><head><meta name="viewport" content="width=device-width,initial-scale=1"><title>JSPL Network & OTA</title><style>
:root{color-scheme:dark}body{font-family:system-ui;margin:0;padding:18px;background:#080b12;color:#fff}.wrap{max-width:560px;margin:auto}.panel{background:#151a24;border:1px solid #293142;border-radius:14px;padding:16px;margin-bottom:12px}h1{font-size:22px}label{display:block;font-size:12px;opacity:.7;margin-top:12px}input{box-sizing:border-box;width:100%;padding:11px;margin-top:5px;border-radius:9px;border:1px solid #333c4c;background:#0d1119;color:#fff}button,a{display:block;width:100%;box-sizing:border-box;padding:12px;margin-top:12px;border:0;border-radius:9px;background:#fff;color:#080b12;text-align:center;text-decoration:none;font-weight:800}.note{padding:10px;border-radius:9px;background:#10241f;color:#8ff0d5;font-size:13px}</style></head><body><div class="wrap"><h1>Network & OTA</h1>)HTML";
  if (notice.length()) h += "<div class='note'>" + htmlEscape(notice) + "</div>";
  h += "<div class='panel'><form method='POST' action='/api/network/save'><label>Wi-Fi SSID</label><input name='ssid' value='" + htmlEscape(settings().wifiSsid) + "'><label>Wi-Fi Password</label><input name='password' type='password' value='" + htmlEscape(settings().wifiPassword) + "'><label>Administrator PIN</label><input name='pin' type='password' required><button>SAVE NETWORK</button></form></div>";
  h += "<div class='panel'><b>Firmware " + String(JSPL_FW_VERSION) + "</b><p>Automatic check runs once after boot. No Internet means no update and normal operation continues.</p><form method='POST' action='/api/ota/check'><label>Administrator PIN</label><input name='pin' type='password' required><button>CHECK FOR UPDATE NOW</button></form></div><a href='/settings'>Device Settings</a><a href='/'>Dashboard</a></div></body></html>";
  return h;
}

bool adminOK() { return server.hasArg("pin") && server.arg("pin") == settings().adminPin; }

void registerRoutes() {
  server.on("/network", HTTP_GET, [](){ server.send(200, "text/html", networkPage()); });
  server.on("/api/network/save", HTTP_POST, [](){
    if (!adminOK()) { server.send(403, "text/plain", "Invalid admin PIN"); return; }
    DeviceConfig c = settings();
    c.wifiSsid = server.arg("ssid");
    c.wifiPassword = server.arg("password");
    c.wifiSsid.trim(); c.wifiPassword.trim();
    settingsSave(c);
    server.send(200, "text/html", networkPage("Network saved. Reboot to perform the boot-time OTA check with the new credentials."));
  });
  server.on("/api/ota/check", HTTP_POST, [](){
    if (!adminOK()) { server.send(403, "text/plain", "Invalid admin PIN"); return; }
    server.send(200, "text/plain", "Checking GitHub. The device will reboot only if a newer verified release is found.");
    delay(100);
    checkForUpdate();
  });
}

void otaTask(void *) {
  delay(BOOT_WAIT_MS);
  registerRoutes();
  checkForUpdate();
  vTaskDelete(nullptr);
}
}

void otaStart() {
  static bool started = false;
  if (started) return;
  started = true;
  xTaskCreate(otaTask, "ota_task", 8192, nullptr, 1, nullptr);
}
