#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "config.h"
#include "settings.h"

// Palarivattom V1 — three-channel staff availability counter.
// Device configuration is stored locally in ESP32 NVS and editable from /settings.
// NORMAL: short press = +1. Hold 10s = RELEASE mode.
// RELEASE: short press = exited +1. Hold 10s = confirm exit.
// CONFIRM EXIT: hold 10s = confirm queue reduction.

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OLED_DC, OLED_RST, OLED_CS);
WebServer server(80);
Preferences prefs;

constexpr uint8_t HOSTEL_COUNT = 3;
const uint8_t BUTTON_PINS[HOSTEL_COUNT] = {PIN_KALOOR, PIN_VYTILLA, PIN_VAZHAKALA};
uint32_t waiting[HOSTEL_COUNT] = {0, 0, 0};
uint32_t exitedCount = 0;
uint8_t selectedHostel = 0;

enum class Mode : uint8_t { NORMAL, RELEASE, CONFIRM_EXIT, CONFIRM_REDUCE, MESSAGE };
Mode mode = Mode::NORMAL;
String messageTitle, messageBody;
uint32_t messageUntil = 0;

struct ButtonState {
  bool raw = HIGH;
  bool stable = HIGH;
  uint32_t changedAt = 0;
  uint32_t pressedAt = 0;
  bool longActionFired = false;
};
ButtonState buttons[HOSTEL_COUNT];

const String &code(uint8_t i) { return settings().destinationCode[i]; }
const String &name(uint8_t i) { return settings().destinationName[i]; }

void saveState() {
  prefs.putUInt("kal", waiting[0]);
  prefs.putUInt("vyt", waiting[1]);
  prefs.putUInt("vaz", waiting[2]);
}

void loadState() {
  waiting[0] = prefs.getUInt("kal", 0);
  waiting[1] = prefs.getUInt("vyt", 0);
  waiting[2] = prefs.getUInt("vaz", 0);
}

void beep(uint16_t frequency = 2200, uint16_t duration = 55) {
  if (PIN_BUZZER != 255) tone(PIN_BUZZER, frequency, duration);
}

void showMessage(const String &title, const String &body, uint32_t duration = 1800) {
  messageTitle = title;
  messageBody = body;
  messageUntil = millis() + duration;
  mode = Mode::MESSAGE;
  beep();
}

const char *modeName() {
  switch (mode) {
    case Mode::NORMAL: return "NORMAL";
    case Mode::RELEASE: return "RELEASE";
    case Mode::CONFIRM_EXIT: return "CONFIRM EXIT";
    case Mode::CONFIRM_REDUCE: return "CONFIRM REDUCE";
    default: return "";
  }
}

void drawCentered(const String &text, int16_t y, uint8_t size) {
  display.setTextSize(size);
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(text, 0, y, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - (int16_t)w) / 2, y);
  display.print(text);
}

void drawNormal() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(settings().deviceId);
  display.setCursor(111, 0);
  display.print("*");

  for (uint8_t i = 0; i < HOSTEL_COUNT; ++i) {
    int16_t x = 5 + i * 42;
    display.setCursor(x + 9, 14);
    display.print(code(i));
    String value = String(waiting[i]);
    display.setTextSize(2);
    display.setCursor(x + 21 - (int16_t)value.length() * 6, 27);
    display.print(value);
    display.setTextSize(1);
  }
  display.drawLine(0, 49, 127, 49, SSD1306_WHITE);
  drawCentered("READY TO BOARD", 53, 1);
  display.display();
}

void drawRelease() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0); display.print(name(selectedHostel));
  display.setCursor(88, 0); display.print("RELEASE");
  display.setCursor(0, 15); display.print("WAITING");
  display.setCursor(0, 26); display.setTextSize(2); display.print(waiting[selectedHostel]);
  display.setTextSize(1); display.setCursor(67, 15); display.print("EXITED");
  display.setCursor(67, 26); display.setTextSize(2); display.print(exitedCount);
  display.setTextSize(1); display.drawLine(0, 47, 127, 47, SSD1306_WHITE);
  display.setCursor(0, 51); display.print("PRESS = EXIT +1");
  display.display();
}

void drawConfirmExit() {
  display.clearDisplay(); display.setTextColor(SSD1306_WHITE);
  drawCentered(name(selectedHostel), 0, 1);
  drawCentered("EXITED", 14, 1);
  drawCentered(String(exitedCount), 26, 2);
  drawCentered("HOLD 10s", 48, 1);
  drawCentered("TO CONFIRM", 57, 1);
  display.display();
}

void drawConfirmReduce() {
  display.clearDisplay(); display.setTextColor(SSD1306_WHITE);
  drawCentered("CONFIRM REDUCE", 0, 1);
  drawCentered(String(exitedCount) + " STAFF", 16, 2);
  uint32_t remaining = waiting[selectedHostel] > exitedCount ? waiting[selectedHostel] - exitedCount : 0;
  drawCentered("WAITING -> " + String(remaining), 39, 1);
  drawCentered("HOLD 10s", 54, 1);
  display.display();
}

void drawMessage() {
  display.clearDisplay(); display.setTextColor(SSD1306_WHITE);
  drawCentered(messageTitle, 7, 1);
  drawCentered(messageBody, 27, 2);
  display.display();
}

void drawHoldProgress(const char *label, uint32_t heldMs) {
  float fraction = min(1.0f, (float)heldMs / (float)settings().longPressMs);
  uint8_t filled = (uint8_t)(fraction * 118.0f);
  display.clearDisplay(); display.setTextColor(SSD1306_WHITE); display.setTextSize(1);
  display.setCursor(0, 0); display.print(name(selectedHostel));
  display.setCursor(0, 13); display.print(label);
  display.drawRect(4, 31, 120, 12, SSD1306_WHITE);
  if (filled > 2) display.fillRect(5, 32, filled, 10, SSD1306_WHITE);
  String seconds = String(heldMs / 1000) + "." + String((heldMs % 1000) / 100);
  drawCentered(seconds + " / " + String(settings().longPressMs / 1000) + "s", 47, 1);
  drawCentered("KEEP HOLDING", 57, 1);
  display.display();
}

void drawDisplay() {
  if (mode == Mode::NORMAL) drawNormal();
  else if (mode == Mode::RELEASE) drawRelease();
  else if (mode == Mode::CONFIRM_EXIT) drawConfirmExit();
  else if (mode == Mode::CONFIRM_REDUCE) drawConfirmReduce();
  else drawMessage();
}

void enterRelease(uint8_t index) {
  if (waiting[index] == 0) { showMessage(code(index), "QUEUE EMPTY"); return; }
  selectedHostel = index;
  exitedCount = 0;
  mode = Mode::RELEASE;
  beep(1800, 80);
  drawDisplay();
}

void registerReady(uint8_t index) {
  if (waiting[index] < 9999) waiting[index]++;
  saveState(); beep(2400, 35); drawDisplay();
}

void registerExit() {
  if (exitedCount < waiting[selectedHostel]) { exitedCount++; beep(2600, 30); }
  else beep(900, 100);
  drawDisplay();
}

void confirmExit() {
  if (exitedCount == 0) { showMessage("NO EXIT COUNT", "NOTHING TO CONFIRM"); return; }
  mode = Mode::CONFIRM_EXIT; beep(1800, 80); drawDisplay();
}

void confirmReduction() { mode = Mode::CONFIRM_REDUCE; beep(1800, 80); drawDisplay(); }

void applyReduction() {
  uint32_t released = min(exitedCount, waiting[selectedHostel]);
  waiting[selectedHostel] -= released;
  saveState();
  mode = Mode::MESSAGE;
  messageTitle = "RELEASED";
  messageBody = String(released) + " STAFF";
  messageUntil = millis() + settings().messageMs;
  beep(2800, 100); drawDisplay();
}

void handleShortPress(uint8_t index) {
  if (mode == Mode::NORMAL) registerReady(index);
  else if (mode == Mode::RELEASE && index == selectedHostel) registerExit();
}

void handleLongPress(uint8_t index) {
  if (mode == Mode::NORMAL) enterRelease(index);
  else if (mode == Mode::RELEASE && index == selectedHostel) confirmExit();
  else if (mode == Mode::CONFIRM_EXIT && index == selectedHostel) confirmReduction();
  else if (mode == Mode::CONFIRM_REDUCE && index == selectedHostel) applyReduction();
}

void processButton(uint8_t index) {
  ButtonState &b = buttons[index];
  bool reading = digitalRead(BUTTON_PINS[index]);
  uint32_t now = millis();
  if (reading != b.raw) { b.raw = reading; b.changedAt = now; }
  if ((now - b.changedAt) >= settings().debounceMs && reading != b.stable) {
    b.stable = reading;
    if (b.stable == LOW) { b.pressedAt = now; b.longActionFired = false; }
    else if (!b.longActionFired && (now - b.pressedAt) < settings().longPressMs) handleShortPress(index);
  }
  if (b.stable == LOW && !b.longActionFired && (now - b.pressedAt) >= settings().longPressMs) {
    b.longActionFired = true; handleLongPress(index);
  }
  bool relevant = (mode == Mode::NORMAL && selectedHostel == index) ||
                  ((mode == Mode::RELEASE || mode == Mode::CONFIRM_EXIT || mode == Mode::CONFIRM_REDUCE) && index == selectedHostel);
  if (b.stable == LOW && relevant && !b.longActionFired) {
    drawHoldProgress(mode == Mode::NORMAL ? "HOLD TO RELEASE" :
                     mode == Mode::RELEASE ? "HOLD TO CONFIRM EXIT" :
                     mode == Mode::CONFIRM_EXIT ? "HOLD TO CONFIRM REDUCE" : "HOLD TO COMPLETE",
                     now - b.pressedAt);
  }
}

String jsonState() {
  String j = "{\"deviceId\":\"" + settings().deviceId + "\",\"deviceName\":\"" + settings().deviceName + "\",\"showroom\":\"" + settings().showroom + "\",\"mode\":\"" + modeName() + "\"";
  j += ",\"selected\":\"" + code(selectedHostel) + "\",\"kaloor\":" + String(waiting[0]);
  j += ",\"vytilla\":" + String(waiting[1]) + ",\"vazhakala\":" + String(waiting[2]) + ",\"exited\":" + String(exitedCount) + "}";
  return j;
}

String dashboardHtml() {
  String h = R"HTML(<!doctype html><html><head><meta name="viewport" content="width=device-width,initial-scale=1"><title>JSPL Transport</title><style>
:root{color-scheme:dark}body{font-family:system-ui;margin:0;padding:18px;background:#080b12;color:#fff}.wrap{max-width:560px;margin:auto}.head{display:flex;justify-content:space-between;align-items:center}.title{font-size:21px;font-weight:800}.sub{opacity:.65;font-size:12px}.grid{display:grid;grid-template-columns:repeat(3,1fr);gap:8px;margin-top:16px}.card,.panel{background:#151a24;border:1px solid #252c3a;border-radius:14px;padding:16px}.card{text-align:center}.code{font-size:12px;opacity:.7}.count{font-size:42px;font-weight:800}.panel{margin-top:12px}.mode{font-size:18px;font-weight:800}.exit{font-size:32px;font-weight:800}a{display:block;text-align:center;margin-top:12px;padding:12px;border-radius:10px;background:#fff;color:#080b12;text-decoration:none;font-weight:700}
</style></head><body><div class="wrap"><div class="head"><div><div class="title">)HTML";
  h += settings().deviceName;
  h += R"HTML(</div><div class="sub">)HTML" + settings().showroom + " • " + settings().deviceId + R"HTML(</div></div><div class="sub">LOCAL ESP32</div></div>
<div class="grid"><div class="card"><div class="code">)HTML" + code(0) + R"HTML(</div><div class="count" id="kal">0</div></div><div class="card"><div class="code">)HTML" + code(1) + R"HTML(</div><div class="count" id="vyt">0</div></div><div class="card"><div class="code">)HTML" + code(2) + R"HTML(</div><div class="count" id="vaz">0</div></div></div>
<div class="panel"><div class="mode" id="mode">NORMAL</div><div class="sub" id="selected">Ready queue counter</div><div class="sub">Current exit count</div><div class="exit" id="exited">0</div></div>
<a href="/settings">Device Settings</a><script>async function r(){try{let x=await fetch('/api/state'),d=await x.json();kal.textContent=d.kaloor;vyt.textContent=d.vytilla;vaz.textContent=d.vazhakala;mode.textContent=d.mode;selected.textContent=d.mode==='NORMAL'?'Ready queue counter':'Selected: '+d.selected;exited.textContent=d.exited}catch(e){}}setInterval(r,500);r()</script></div></body></html>)HTML";
  return h;
}

String settingsHtml(const String &notice = "") {
  DeviceConfig &c = settings();
  String h = R"HTML(<!doctype html><html><head><meta name="viewport" content="width=device-width,initial-scale=1"><title>Device Settings</title><style>
:root{color-scheme:dark}body{font-family:system-ui;margin:0;padding:18px;background:#080b12;color:#fff}.wrap{max-width:560px;margin:auto}.panel{background:#151a24;border:1px solid #252c3a;border-radius:14px;padding:16px;margin-bottom:12px}h1{font-size:22px}h2{font-size:15px;margin:0 0 12px}label{display:block;font-size:12px;opacity:.7;margin-top:10px}input{box-sizing:border-box;width:100%;padding:11px;margin-top:5px;border-radius:9px;border:1px solid #333c4c;background:#0d1119;color:#fff}button,a{display:block;width:100%;box-sizing:border-box;padding:12px;margin-top:12px;border:0;border-radius:9px;background:#fff;color:#080b12;text-align:center;text-decoration:none;font-weight:800} .danger{background:#ef4444;color:#fff}.note{padding:10px;border-radius:9px;background:#10241f;color:#8ff0d5;font-size:13px}
</style></head><body><div class="wrap"><h1>Device Settings</h1>)HTML";
  if (notice.length()) h += "<div class='note'>" + notice + "</div>";
  h += "<form method='POST' action='/api/settings/save'>";
  h += "<div class='panel'><h2>Device</h2>";
  h += "<label>Admin PIN</label><input name='pin' type='password' inputmode='numeric' required>";
  h += "<label>Device ID (stable)</label><input name='deviceId' value='" + c.deviceId + "' required>";
  h += "<label>Device Name</label><input name='deviceName' value='" + c.deviceName + "' required>";
  h += "<label>Showroom</label><input name='showroom' value='" + c.showroom + "' required>";
  h += "<label>Installation</label><input name='installation' value='" + c.installation + "'>";
  h += "</div><div class='panel'><h2>Bus (reserved for future bus controller)</h2>";
  h += "<label>Registration Number</label><input name='busRegistration' value='" + c.busRegistration + "'>";
  h += "<label>Capacity</label><input name='busCapacity' type='number' min='1' max='100' value='" + String(c.busCapacity) + "'>";
  h += "<label>Bus Enabled</label><input name='busEnabled' type='checkbox' " + String(c.busEnabled ? "checked" : "") + ">";
  h += "</div><div class='panel'><h2>Destinations</h2>";
  for (uint8_t i=0;i<3;i++) {
    h += "<label>Button " + String(i+1) + " Code</label><input name='d" + String(i+1) + "code' value='" + c.destinationCode[i] + "' required>";
    h += "<label>Button " + String(i+1) + " Name</label><input name='d" + String(i+1) + "name' value='" + c.destinationName[i] + "' required>";
  }
  h += "</div><div class='panel'><h2>Operation</h2>";
  h += "<label>Long press (ms)</label><input name='longPressMs' type='number' min='3000' max='30000' value='" + String(c.longPressMs) + "'>";
  h += "<label>Debounce (ms)</label><input name='debounceMs' type='number' min='10' max='200' value='" + String(c.debounceMs) + "'>";
  h += "<label>Message display (ms)</label><input name='messageMs' type='number' min='500' max='10000' value='" + String(c.messageMs) + "'>";
  h += "</div><div class='panel'><h2>Security</h2><label>New Admin PIN</label><input name='newPin' type='password' inputmode='numeric' placeholder='Leave blank to keep current'></div>";
  h += "<button type='submit'>SAVE SETTINGS</button></form><a href='/'>BACK TO DASHBOARD</a>";
  h += "<form method='POST' action='/api/settings/reset' onsubmit=\"return confirm('Factory reset configuration?')\"><input name='pin' type='password' placeholder='Admin PIN' required><button class='danger' type='submit'>FACTORY RESET CONFIGURATION</button></form>";
  h += "</div></body></html>";
  return h;
}

bool adminPinOK() {
  return server.hasArg("pin") && server.arg("pin") == settings().adminPin;
}

String cleanValue(String value, size_t maxLen) {
  value.trim();
  if (value.length() > maxLen) value.remove(maxLen);
  value.replace("<", ""); value.replace(">", "");
  return value;
}

void handleSettingsSave() {
  if (!adminPinOK()) { server.send(403, "text/plain", "Invalid admin PIN"); return; }
  DeviceConfig c = settings();
  c.deviceId = cleanValue(server.arg("deviceId"), 31);
  c.deviceName = cleanValue(server.arg("deviceName"), 40);
  c.showroom = cleanValue(server.arg("showroom"), 40);
  c.installation = cleanValue(server.arg("installation"), 40);
  c.busRegistration = cleanValue(server.arg("busRegistration"), 24);
  c.busCapacity = constrain(server.arg("busCapacity").toInt(), 1, 100);
  c.busEnabled = server.hasArg("busEnabled");
  c.destinationCode[0] = cleanValue(server.arg("d1code"), 8);
  c.destinationCode[1] = cleanValue(server.arg("d2code"), 8);
  c.destinationCode[2] = cleanValue(server.arg("d3code"), 8);
  c.destinationName[0] = cleanValue(server.arg("d1name"), 24);
  c.destinationName[1] = cleanValue(server.arg("d2name"), 24);
  c.destinationName[2] = cleanValue(server.arg("d3name"), 24);
  c.longPressMs = constrain(server.arg("longPressMs").toInt(), 3000, 30000);
  c.debounceMs = constrain(server.arg("debounceMs").toInt(), 10, 200);
  c.messageMs = constrain(server.arg("messageMs").toInt(), 500, 10000);
  String newPin = cleanValue(server.arg("newPin"), 12);
  if (newPin.length()) c.adminPin = newPin;
  settingsSave(c);
  server.send(200, "text/html", settingsHtml("Settings saved. Restart the device if you changed hardware-independent configuration that should be reloaded immediately."));
  drawDisplay();
}

void handleFactoryReset() {
  if (!adminPinOK()) { server.send(403, "text/plain", "Invalid admin PIN"); return; }
  settingsFactoryReset();
  server.send(200, "text/html", settingsHtml("Configuration restored to factory defaults. Counters were not reset."));
  drawDisplay();
}

void setupWebServer() {
  server.on("/", HTTP_GET, [](){ server.send(200, "text/html", dashboardHtml()); });
  server.on("/api/state", HTTP_GET, [](){ server.send(200, "application/json", jsonState()); });
  server.on("/settings", HTTP_GET, [](){ server.send(200, "text/html", settingsHtml()); });
  server.on("/api/settings/save", HTTP_POST, handleSettingsSave);
  server.on("/api/settings/reset", HTTP_POST, handleFactoryReset);
  server.begin();
}

void setup() {
  Serial.begin(115200);
  prefs.begin("pvmcounter", false);
  settingsBegin(prefs);
  loadState();

  for (uint8_t i=0;i<HOSTEL_COUNT;i++) {
    pinMode(BUTTON_PINS[i], INPUT_PULLUP);
    buttons[i].raw = digitalRead(BUTTON_PINS[i]);
    buttons[i].stable = buttons[i].raw;
  }
  if (PIN_BUZZER != 255) pinMode(PIN_BUZZER, OUTPUT);

  SPI.begin(OLED_SCK, -1, OLED_MOSI, OLED_CS);
  if (!display.begin(SSD1306_SWITCHCAPVCC)) Serial.println("OLED init failed. Check SPI wiring/pins.");

  display.clearDisplay(); display.setTextColor(SSD1306_WHITE);
  drawCentered("JSPL TRANSPORT", 10, 1);
  drawCentered(settings().deviceId, 27, 2);
  drawCentered("STARTING", 51, 1); display.display();

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_NAME, AP_PASSWORD);
  Serial.println("=== JSPL PVM COUNTER ===");
  Serial.print("Wi-Fi: "); Serial.println(AP_NAME);
  Serial.print("Password: "); Serial.println(AP_PASSWORD);
  Serial.print("Web: http://"); Serial.println(WiFi.softAPIP());

  setupWebServer();
  delay(500);
  drawDisplay();
}

void loop() {
  server.handleClient();
  for (uint8_t i=0;i<HOSTEL_COUNT;i++) processButton(i);
  if (mode == Mode::MESSAGE && millis() >= messageUntil) {
    mode = Mode::NORMAL; exitedCount = 0; drawDisplay();
  }
}
