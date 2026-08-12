#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "config.h"
#include "settings.h"
#include "ota.h"

// JSPL Transport Counter V2
// Hardware: HW-724 / ESP32-WROOM-32 / integrated SSD1306 128x64 OLED.
// NORMAL: short press = ready +1. Hold = enter release mode.
// RELEASE: short press = exited +1. Hold = confirm exit count.
// CONFIRM EXIT: hold = confirm reduction. Counters are persistent in NVS.

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
WebServer server(80);
Preferences prefs;

constexpr uint8_t DESTINATION_COUNT = 3;
const uint8_t BUTTON_PINS[DESTINATION_COUNT] = {PIN_KALOOR, PIN_VYTILLA, PIN_VAZHAKALA};
uint32_t waiting[DESTINATION_COUNT] = {0, 0, 0};
uint32_t exitedCount = 0;
uint8_t selectedDestination = 0;

// A single selected destination is used for release operations. This prevents
// accidental cross-destination changes while people are physically leaving.
enum class Mode : uint8_t { NORMAL, RELEASE, CONFIRM_EXIT, CONFIRM_REDUCE, MESSAGE };
Mode mode = Mode::NORMAL;
String messageTitle;
String messageBody;
uint32_t messageUntil = 0;
uint32_t lastUiFrame = 0;

struct ButtonState {
  bool raw = HIGH;
  bool stable = HIGH;
  uint32_t changedAt = 0;
  uint32_t pressedAt = 0;
  bool longActionFired = false;
};
ButtonState buttons[DESTINATION_COUNT];

const String &destinationCode(uint8_t i) { return settings().destinationCode[i]; }
const String &destinationName(uint8_t i) { return settings().destinationName[i]; }

void saveCounters() {
  prefs.putUInt("d1", waiting[0]);
  prefs.putUInt("d2", waiting[1]);
  prefs.putUInt("d3", waiting[2]);
}

void loadCounters() {
  waiting[0] = prefs.getUInt("d1", 0);
  waiting[1] = prefs.getUInt("d2", 0);
  waiting[2] = prefs.getUInt("d3", 0);
}

void beep(uint16_t frequency = 2200, uint16_t duration = 45) {
  if (PIN_BUZZER != 255) tone(PIN_BUZZER, frequency, duration);
}

void centerText(const String &text, int16_t y, uint8_t size = 1) {
  display.setTextSize(size);
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(text, 0, y, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - (int16_t)w) / 2, y);
  display.print(text);
}

void drawHeader(const String &left, const String &right = "") {
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(left);
  if (right.length()) {
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(right, 0, 0, &x1, &y1, &w, &h);
    display.setCursor(SCREEN_WIDTH - w, 0);
    display.print(right);
  }
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
}

void drawNormal() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  drawHeader(settings().deviceName, WiFi.status() == WL_CONNECTED ? "*" : "o");

  // Three equal columns: destination + large count. This uses almost every
  // useful pixel while retaining immediate readability at the gate.
  for (uint8_t i = 0; i < DESTINATION_COUNT; ++i) {
    const int x = i * 42;
    display.setTextSize(1);
    String code = destinationCode(i);
    display.setCursor(x + (42 - code.length() * 6) / 2, 15);
    display.print(code);

    String value = String(waiting[i]);
    display.setTextSize(value.length() > 3 ? 2 : 3);
    int width = value.length() * (value.length() > 3 ? 12 : 18);
    display.setCursor(x + (42 - width) / 2, 25);
    display.print(value);
  }

  display.drawLine(0, 53, 127, 53, SSD1306_WHITE);
  centerText("READY TO BOARD", 56, 1);
  display.display();
}

void drawRelease() {
  display.clearDisplay();
  drawHeader(destinationCode(selectedDestination), "RELEASE");

  display.setTextSize(1);
  display.setCursor(3, 15); display.print("WAITING");
  display.setCursor(70, 15); display.print("EXITED");

  display.setTextSize(2);
  display.setCursor(3, 25); display.print(waiting[selectedDestination]);
  display.setCursor(70, 25); display.print(exitedCount);

  display.drawLine(0, 45, 127, 45, SSD1306_WHITE);
  centerText("PRESS = EXIT +1", 49, 1);
  centerText("HOLD = CONFIRM", 57, 1);
  display.display();
}

void drawConfirmExit() {
  display.clearDisplay();
  drawHeader(destinationCode(selectedDestination), "CONFIRM");
  centerText("STAFF EXITED", 16, 1);
  centerText(String(exitedCount), 27, 2);
  centerText("HOLD TO CONTINUE", 49, 1);
  centerText("10 SEC", 57, 1);
  display.display();
}

void drawConfirmReduce() {
  display.clearDisplay();
  drawHeader(destinationCode(selectedDestination), "FINAL");
  uint32_t remaining = waiting[selectedDestination] > exitedCount ? waiting[selectedDestination] - exitedCount : 0;
  centerText("RELEASE " + String(exitedCount), 15, 1);
  centerText("REMAIN " + String(remaining), 28, 2);
  centerText("HOLD 10 SEC", 51, 1);
  display.display();
}

void drawMessage() {
  display.clearDisplay();
  centerText(messageTitle, 9, 1);
  centerText(messageBody, 26, 2);
  display.display();
}

void drawHoldProgress(const String &action, uint32_t heldMs) {
  const float fraction = min(1.0f, (float)heldMs / (float)settings().longPressMs);
  const uint8_t width = (uint8_t)(fraction * 118.0f);

  display.clearDisplay();
  drawHeader(destinationCode(selectedDestination), "HOLD");
  centerText(action, 14, 1);
  display.drawRect(4, 30, 120, 13, SSD1306_WHITE);
  if (width > 1) display.fillRect(5, 31, width, 11, SSD1306_WHITE);

  String seconds = String(heldMs / 1000) + "." + String((heldMs % 1000) / 100);
  centerText(seconds + " / " + String(settings().longPressMs / 1000) + "s", 47, 1);
  centerText("KEEP HOLDING", 57, 1);
  display.display();
}

void drawScreen() {
  if (mode == Mode::NORMAL) drawNormal();
  else if (mode == Mode::RELEASE) drawRelease();
  else if (mode == Mode::CONFIRM_EXIT) drawConfirmExit();
  else if (mode == Mode::CONFIRM_REDUCE) drawConfirmReduce();
  else drawMessage();
}

void showMessage(const String &title, const String &body) {
  messageTitle = title;
  messageBody = body;
  messageUntil = millis() + settings().messageMs;
  mode = Mode::MESSAGE;
  beep(2600, 70);
  drawScreen();
}

void addReady(uint8_t index) {
  if (waiting[index] < MAX_QUEUE_COUNT) {
    ++waiting[index];
    saveCounters();
    beep(2300, 35);
  } else {
    beep(700, 120);
  }
  drawScreen();
}

void startRelease(uint8_t index) {
  if (waiting[index] == 0) {
    showMessage(destinationCode(index), "QUEUE EMPTY");
    return;
  }
  selectedDestination = index;
  exitedCount = 0;
  mode = Mode::RELEASE;
  beep(1800, 80);
  drawScreen();
}

void addExit() {
  if (exitedCount < waiting[selectedDestination]) {
    ++exitedCount;
    beep(2600, 30);
  } else {
    beep(700, 120);
  }
  drawScreen();
}

void enterConfirmExit() {
  if (exitedCount == 0) {
    showMessage("NO EXIT COUNT", "NOTHING TO CONFIRM");
    return;
  }
  mode = Mode::CONFIRM_EXIT;
  beep(1800, 80);
  drawScreen();
}

void enterConfirmReduce() {
  mode = Mode::CONFIRM_REDUCE;
  beep(1800, 80);
  drawScreen();
}

void applyReduction() {
  const uint32_t released = min(exitedCount, waiting[selectedDestination]);
  waiting[selectedDestination] -= released;
  saveCounters();

  messageTitle = "RELEASED";
  messageBody = String(released) + " STAFF";
  messageUntil = millis() + settings().messageMs;
  mode = Mode::MESSAGE;
  beep(3000, 120);
  drawScreen();
}

void shortPress(uint8_t index) {
  if (mode == Mode::NORMAL) addReady(index);
  else if (mode == Mode::RELEASE && index == selectedDestination) addExit();
}

void longPress(uint8_t index) {
  if (mode == Mode::NORMAL) startRelease(index);
  else if (mode == Mode::RELEASE && index == selectedDestination) enterConfirmExit();
  else if (mode == Mode::CONFIRM_EXIT && index == selectedDestination) enterConfirmReduce();
  else if (mode == Mode::CONFIRM_REDUCE && index == selectedDestination) applyReduction();
}

void processButton(uint8_t index) {
  ButtonState &b = buttons[index];
  const uint32_t now = millis();
  const bool reading = digitalRead(BUTTON_PINS[index]);

  if (reading != b.raw) {
    b.raw = reading;
    b.changedAt = now;
  }

  if ((now - b.changedAt) >= settings().debounceMs && reading != b.stable) {
    b.stable = reading;
    if (b.stable == LOW) {
      b.pressedAt = now;
      b.longActionFired = false;
    } else if (!b.longActionFired && (now - b.pressedAt) < settings().longPressMs) {
      shortPress(index);
    }
  }

  const bool activeButton =
      (mode == Mode::NORMAL) ||
      ((mode == Mode::RELEASE || mode == Mode::CONFIRM_EXIT || mode == Mode::CONFIRM_REDUCE) && index == selectedDestination);

  if (b.stable == LOW && activeButton && !b.longActionFired) {
    const uint32_t held = now - b.pressedAt;
    if (held >= settings().longPressMs) {
      b.longActionFired = true;
      longPress(index);
    } else if (now - lastUiFrame >= 80) {
      lastUiFrame = now;
      String action = mode == Mode::NORMAL ? "HOLD TO RELEASE" :
                      mode == Mode::RELEASE ? "HOLD TO CONFIRM" :
                      mode == Mode::CONFIRM_EXIT ? "HOLD TO REDUCE" : "HOLD TO COMPLETE";
      selectedDestination = index;
      drawHoldProgress(action, held);
    }
  }
}

String jsonEscape(String value) {
  value.replace("\\", "\\\\");
  value.replace("\"", "\\\"");
  return value;
}

String jsonState() {
  String j = "{";
  j += "\"deviceId\":\"" + jsonEscape(settings().deviceId) + "\",";
  j += "\"deviceName\":\"" + jsonEscape(settings().deviceName) + "\",";
  j += "\"showroom\":\"" + jsonEscape(settings().showroom) + "\",";
  j += "\"mode\":\"" + String((uint8_t)mode) + "\",";
  j += "\"selected\":\"" + jsonEscape(destinationCode(selectedDestination)) + "\",";
  j += "\"waiting\":[" + String(waiting[0]) + "," + String(waiting[1]) + "," + String(waiting[2]) + "],";
  j += "\"exited\":" + String(exitedCount) + ",";
  j += "\"wifi\":\"" + String(WiFi.status() == WL_CONNECTED ? "ONLINE" : "LOCAL") + "\"}";
  return j;
}

String htmlEscape(String value) {
  value.replace("&", "&amp;");
  value.replace("<", "&lt;");
  value.replace(">", "&gt;");
  value.replace("\"", "&quot;");
  return value;
}

String dashboardHtml() {
  String h = R"HTML(<!doctype html><html><head><meta name="viewport" content="width=device-width,initial-scale=1"><title>JSPL Transport</title><style>
:root{color-scheme:dark}*{box-sizing:border-box}body{margin:0;background:#080b12;color:#fff;font-family:system-ui,-apple-system,sans-serif}.wrap{max-width:620px;margin:auto;padding:16px}.top{display:flex;justify-content:space-between;align-items:center}.brand{font-size:20px;font-weight:800}.sub{font-size:12px;opacity:.6}.dot{font-size:12px}.grid{display:grid;grid-template-columns:repeat(3,1fr);gap:8px;margin:18px 0 10px}.card{background:#151a24;border:1px solid #293142;border-radius:16px;padding:14px;text-align:center}.code{font-size:12px;opacity:.65}.name{font-size:11px;white-space:nowrap;overflow:hidden;text-overflow:ellipsis}.num{font-size:44px;line-height:1.05;font-weight:850;margin-top:5px}.panel{background:#151a24;border:1px solid #293142;border-radius:16px;padding:16px;margin-top:10px}.mode{font-size:18px;font-weight:800}.big{font-size:34px;font-weight:850;margin-top:3px}.btn{display:block;text-align:center;background:#fff;color:#080b12;text-decoration:none;font-weight:800;border-radius:11px;padding:12px;margin-top:12px}.warn{border-color:#806b2b}.small{font-size:12px;opacity:.6}</style></head><body><div class="wrap"><div class="top"><div><div class="brand">)HTML";
  h += htmlEscape(settings().deviceName);
  h += R"HTML(</div><div class="sub">)HTML" + htmlEscape(settings().showroom) + " • " + htmlEscape(settings().deviceId) + R"HTML(</div></div><div class="dot">● )HTML";
  h += WiFi.status() == WL_CONNECTED ? "ONLINE" : "LOCAL";
  h += R"HTML(</div></div><div class="grid">)HTML";
  for (uint8_t i = 0; i < DESTINATION_COUNT; ++i) {
    h += "<div class='card'><div class='code'>" + htmlEscape(destinationCode(i)) + "</div><div class='name'>" + htmlEscape(destinationName(i)) + "</div><div class='num' id='c" + String(i) + "'>0</div></div>";
  }
  h += R"HTML(</div><div class="panel"><div class="small">CURRENT MODE</div><div class="mode" id="mode">NORMAL</div><div class="small">EXITED</div><div class="big" id="exit">0</div></div><a class="btn" href="/settings">DEVICE SETTINGS</a><script>async function r(){try{let d=await (await fetch('/api/state')).json();d.waiting.forEach((v,i)=>document.getElementById('c'+i).textContent=v);document.getElementById('mode').textContent=['NORMAL','RELEASE','CONFIRM EXIT','CONFIRM REDUCE','MESSAGE'][d.mode]||'NORMAL';document.getElementById('exit').textContent=d.exited}catch(e){}}r();setInterval(r,500)</script></div></body></html>)HTML";
  return h;
}

String settingsHtml(const String &notice = "") {
  const DeviceConfig &c = settings();
  String h = R"HTML(<!doctype html><html><head><meta name="viewport" content="width=device-width,initial-scale=1"><title>JSPL Device Settings</title><style>
:root{color-scheme:dark}*{box-sizing:border-box}body{margin:0;background:#080b12;color:#fff;font-family:system-ui}.wrap{max-width:620px;margin:auto;padding:16px}.panel{background:#151a24;border:1px solid #293142;border-radius:16px;padding:16px;margin-bottom:10px}h1{font-size:22px}h2{font-size:14px;margin:0 0 10px}label{display:block;font-size:12px;opacity:.65;margin-top:10px}input{width:100%;padding:11px;border-radius:9px;border:1px solid #394255;background:#0d1119;color:#fff;margin-top:4px}button,a{display:block;width:100%;padding:12px;border:0;border-radius:10px;margin-top:10px;text-align:center;text-decoration:none;font-weight:800;background:#fff;color:#080b12}.danger{background:#d33;color:#fff}.note{background:#10241f;padding:10px;border-radius:10px;margin-bottom:10px;font-size:13px}</style></head><body><div class="wrap"><h1>Device Settings</h1>)HTML";
  if (notice.length()) h += "<div class='note'>" + htmlEscape(notice) + "</div>";
  h += "<form method='POST' action='/api/settings/save'><div class='panel'><h2>IDENTITY</h2>";
  h += "<label>Administrator PIN</label><input name='pin' type='password' required>";
  h += "<label>Device ID</label><input name='deviceId' value='" + htmlEscape(c.deviceId) + "' required>";
  h += "<label>Device Name</label><input name='deviceName' value='" + htmlEscape(c.deviceName) + "' required>";
  h += "<label>Showroom</label><input name='showroom' value='" + htmlEscape(c.showroom) + "' required>";
  h += "<label>Installation</label><input name='installation' value='" + htmlEscape(c.installation) + "'>";
  h += "</div><div class='panel'><h2>BUS</h2><label>Registration Number</label><input name='busRegistration' value='" + htmlEscape(c.busRegistration) + "'><label>Capacity</label><input name='busCapacity' type='number' min='1' max='100' value='" + String(c.busCapacity) + "'><label>Enable bus association</label><input name='busEnabled' type='checkbox' " + String(c.busEnabled ? "checked" : "") + "></div>";
  h += "<div class='panel'><h2>DESTINATIONS</h2>";
  for (uint8_t i=0; i<DESTINATION_COUNT; ++i) {
    h += "<label>Button " + String(i+1) + " code</label><input name='d" + String(i+1) + "code' value='" + htmlEscape(c.destinationCode[i]) + "' required>";
    h += "<label>Button " + String(i+1) + " name</label><input name='d" + String(i+1) + "name' value='" + htmlEscape(c.destinationName[i]) + "' required>";
  }
  h += "</div><div class='panel'><h2>OPERATION</h2><label>Long press (ms)</label><input name='longPressMs' type='number' min='3000' max='30000' value='" + String(c.longPressMs) + "'><label>Debounce (ms)</label><input name='debounceMs' type='number' min='10' max='200' value='" + String(c.debounceMs) + "'><label>Message display (ms)</label><input name='messageMs' type='number' min='500' max='10000' value='" + String(c.messageMs) + "'></div>";
  h += "<div class='panel'><h2>SECURITY</h2><label>New Administrator PIN</label><input name='newPin' type='password' inputmode='numeric' placeholder='Leave blank to keep current'></div><button type='submit'>SAVE SETTINGS</button></form><a href='/'>BACK TO DASHBOARD</a>";
  h += "<form method='POST' action='/api/settings/reset'><div class='panel'><h2>DANGER ZONE</h2><label>Administrator PIN</label><input name='pin' type='password' required><button class='danger' type='submit'>FACTORY RESET CONFIGURATION</button></div></form></div></body></html>";
  return h;
}

bool adminPinOK() { return server.hasArg("pin") && server.arg("pin") == settings().adminPin; }

String cleanValue(String value, size_t maxLen) {
  value.trim();
  if (value.length() > maxLen) value.remove(maxLen);
  value.replace("<", "");
  value.replace(">", "");
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
  for (uint8_t i=0; i<DESTINATION_COUNT; ++i) {
    c.destinationCode[i] = cleanValue(server.arg("d" + String(i+1) + "code"), 8);
    c.destinationName[i] = cleanValue(server.arg("d" + String(i+1) + "name"), 24);
  }
  c.longPressMs = constrain(server.arg("longPressMs").toInt(), 3000, 30000);
  c.debounceMs = constrain(server.arg("debounceMs").toInt(), 10, 200);
  c.messageMs = constrain(server.arg("messageMs").toInt(), 500, 10000);
  const String newPin = cleanValue(server.arg("newPin"), 12);
  if (newPin.length()) c.adminPin = newPin;
  settingsSave(c);
  server.send(200, "text/html", settingsHtml("Settings saved."));
  drawScreen();
}

void handleFactoryReset() {
  if (!adminPinOK()) { server.send(403, "text/plain", "Invalid admin PIN"); return; }
  settingsFactoryReset();
  server.send(200, "text/html", settingsHtml("Configuration restored. Counters were not reset."));
  drawScreen();
}

void setupWebServer() {
  server.on("/", HTTP_GET, [](){ server.send(200, "text/html", dashboardHtml()); });
  server.on("/api/state", HTTP_GET, [](){ server.send(200, "application/json", jsonState()); });
  server.on("/settings", HTTP_GET, [](){ server.send(200, "text/html", settingsHtml()); });
  server.on("/api/settings/save", HTTP_POST, handleSettingsSave);
  server.on("/api/settings/reset", HTTP_POST, handleFactoryReset);
  server.begin();
}

void startNetwork() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(DEFAULT_AP_NAME, DEFAULT_AP_PASSWORD);

  if (!settings().wifiSsid.isEmpty()) {
    WiFi.begin(settings().wifiSsid.c_str(), settings().wifiPassword.c_str());
  }

  Serial.println("=== JSPL TRANSPORT COUNTER V2 ===");
  Serial.print("AP: http://"); Serial.println(WiFi.softAPIP());
  if (!settings().wifiSsid.isEmpty()) Serial.println("Internet Wi-Fi connection started.");
}

void setup() {
  Serial.begin(115200);
  delay(100);

  prefs.begin("jsplcounter", false);
  settingsBegin(prefs);
  loadCounters();

  for (uint8_t i=0; i<DESTINATION_COUNT; ++i) {
    pinMode(BUTTON_PINS[i], INPUT_PULLUP);
    buttons[i].raw = digitalRead(BUTTON_PINS[i]);
    buttons[i].stable = buttons[i].raw;
  }
  pinMode(PIN_BUZZER, OUTPUT);

  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("OLED initialization failed. Check HW-724 OLED wiring.");
  } else {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    centerText("JSPL TRANSPORT", 8, 1);
    centerText("HW-724", 25, 2);
    centerText(settings().deviceId, 47, 1);
    display.display();
  }

  startNetwork();
  setupWebServer();
  drawScreen();

  // OTA starts only after NVS, display, network and WebServer are initialized.
  // This avoids the previous static-constructor race during boot.
  otaStart();
}

void loop() {
  server.handleClient();
  for (uint8_t i=0; i<DESTINATION_COUNT; ++i) processButton(i);

  if (mode == Mode::MESSAGE && millis() >= messageUntil) {
    mode = Mode::NORMAL;
    exitedCount = 0;
    drawScreen();
  }

  // Refresh the normal screen occasionally so Wi-Fi status changes are visible.
  if (mode == Mode::NORMAL && millis() - lastUiFrame > 3000) {
    lastUiFrame = millis();
    drawNormal();
  }
}
