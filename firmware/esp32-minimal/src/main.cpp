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

// JSPL Transport Gate Controller - HW-724 / ESP32-WROOM-32
// OLED: SSD1306 128x64 I2C, SDA GPIO5, SCL GPIO4, address 0x3C.
// All long-press actions use the same configurable duration; default is 5 seconds.
// Normal: short press +1; long press starts release mode.
// Release: short press counts exited; long press confirms; long press again applies reduction.
// Reset: all three buttons held for the long-press duration enters reset-select mode;
//        one destination button held for the same duration clears that counter.
// Web dashboard and settings are hosted entirely by the ESP32.

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
WebServer server(80);
Preferences prefs;
constexpr uint8_t N = 3;
const uint8_t P[N] = {PIN_KALOOR, PIN_VYTILLA, PIN_VAZHAKALA};
uint32_t waiting[N] = {0, 0, 0};
uint32_t exitedCount = 0;
uint8_t selected = 0;

enum class Mode : uint8_t {
  NORMAL,
  RELEASE,
  CONFIRM_EXIT,
  CONFIRM_REDUCE,
  MESSAGE,
  RESET_SELECT
};
Mode mode = Mode::NORMAL;

String msgTitle, msgBody;
uint32_t msgUntil = 0;
uint32_t lastUi = 0;

struct Btn {
  bool raw = HIGH;
  bool stable = HIGH;
  bool longFired = false;
  uint32_t changed = 0;
  uint32_t pressed = 0;
};
Btn btn[N];

const String &code(uint8_t i) { return settings().destinationCode[i]; }
const String &name(uint8_t i) { return settings().destinationName[i]; }
uint32_t holdMs() { return max<uint32_t>(1000, settings().longPressMs); }

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

String esc(String value) {
  value.replace("&", "&amp;");
  value.replace("<", "&lt;");
  value.replace(">", "&gt;");
  value.replace("\"", "&quot;");
  return value;
}

String jsonEsc(String value) {
  value.replace("\\", "\\\\");
  value.replace("\"", "\\\"");
  return value;
}

void centerText(const String &text, int y, int size = 1) {
  display.setTextSize(size);
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(text, 0, y, &x1, &y1, &w, &h);
  display.setCursor((128 - (int)w) / 2, y);
  display.print(text);
}

void header(const String &left, const String &right = "") {
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(left);
  if (right.length()) {
    int16_t x, y;
    uint16_t w, h;
    display.getTextBounds(right, 0, 0, &x, &y, &w, &h);
    display.setCursor(128 - w, 0);
    display.print(right);
  }
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
}

void normalScreen() {
  display.clearDisplay();
  header(settings().deviceName, WiFi.status() == WL_CONNECTED ? "*" : "o");
  for (uint8_t i = 0; i < N; ++i) {
    const int x = i * 42;
    display.setTextSize(1);
    display.setCursor(x + max(0, (42 - (int)code(i).length() * 6) / 2), 14);
    display.print(code(i));

    const String value = String(waiting[i]);
    display.setTextSize(value.length() > 3 ? 2 : 3);
    const int width = value.length() > 3 ? value.length() * 12 : value.length() * 18;
    display.setCursor(x + (42 - width) / 2, 25);
    display.print(value);
  }
  display.drawLine(0, 53, 127, 53, SSD1306_WHITE);
  centerText("READY TO BOARD", 56);
  display.display();
}

void releaseScreen() {
  display.clearDisplay();
  header(code(selected), "RELEASE");
  display.setTextSize(1);
  display.setCursor(3, 15); display.print("WAITING");
  display.setCursor(70, 15); display.print("EXITED");
  display.setTextSize(2);
  display.setCursor(3, 25); display.print(waiting[selected]);
  display.setCursor(70, 25); display.print(exitedCount);
  display.drawLine(0, 45, 127, 45, SSD1306_WHITE);
  centerText("PRESS = EXIT +1", 49);
  centerText("HOLD = CONFIRM", 57);
  display.display();
}

void confirmExitScreen() {
  display.clearDisplay();
  header(code(selected), "CONFIRM");
  centerText("STAFF EXITED", 16);
  centerText(String(exitedCount), 27, 2);
  centerText("HOLD TO CONTINUE", 48);
  centerText(String(holdMs() / 1000) + " SEC", 57);
  display.display();
}

void confirmReduceScreen() {
  display.clearDisplay();
  header(code(selected), "FINAL");
  const uint32_t remaining = waiting[selected] > exitedCount ? waiting[selected] - exitedCount : 0;
  centerText("RELEASE " + String(exitedCount), 15);
  centerText("REMAIN " + String(remaining), 28, 2);
  centerText("HOLD " + String(holdMs() / 1000) + " SEC", 51);
  display.display();
}

void resetSelectScreen() {
  display.clearDisplay();
  header("COUNTER RESET", "ARMED");
  centerText("SELECT COUNTER", 16);
  centerText("HOLD " + String(holdMs() / 1000) + " SEC", 29, 2);
  centerText(code(0) + " / " + code(1) + " / " + code(2), 53);
  display.display();
}

void messageScreen() {
  display.clearDisplay();
  centerText(msgTitle, 9);
  centerText(msgBody, 27, 2);
  display.display();
}

void progressScreen(const String &label, uint32_t held, uint32_t total) {
  const float fraction = min(1.0f, (float)held / (float)total);
  const uint8_t width = (uint8_t)(fraction * 118.0f);
  display.clearDisplay();
  header("JSPL TRANSPORT", "HOLD");
  centerText(label, 14);
  display.drawRect(4, 30, 120, 13, SSD1306_WHITE);
  if (width > 1) display.fillRect(5, 31, width, 11, SSD1306_WHITE);
  centerText(String(held / 1000) + "." + String((held % 1000) / 100) + " / " + String(total / 1000) + "s", 47);
  centerText("KEEP HOLDING", 57);
  display.display();
}

void drawScreen() {
  switch (mode) {
    case Mode::NORMAL: normalScreen(); break;
    case Mode::RELEASE: releaseScreen(); break;
    case Mode::CONFIRM_EXIT: confirmExitScreen(); break;
    case Mode::CONFIRM_REDUCE: confirmReduceScreen(); break;
    case Mode::RESET_SELECT: resetSelectScreen(); break;
    default: messageScreen(); break;
  }
}

void showMessage(const String &title, const String &body) {
  msgTitle = title;
  msgBody = body;
  msgUntil = millis() + settings().messageMs;
  mode = Mode::MESSAGE;
  beep(2600, 70);
  messageScreen();
}

void addReady(uint8_t index) {
  if (waiting[index] < MAX_QUEUE_COUNT) {
    ++waiting[index];
    saveCounters();
    beep(2300, 35);
  } else {
    beep(700, 120);
  }
  normalScreen();
}

void startRelease(uint8_t index) {
  if (waiting[index] == 0) {
    showMessage(code(index), "QUEUE EMPTY");
    return;
  }
  selected = index;
  exitedCount = 0;
  mode = Mode::RELEASE;
  beep(1800, 80);
  releaseScreen();
}

void addExit() {
  if (exitedCount < waiting[selected]) {
    ++exitedCount;
    beep(2600, 30);
  } else {
    beep(700, 120);
  }
  releaseScreen();
}

void applyReduction() {
  const uint32_t reduction = min(exitedCount, waiting[selected]);
  waiting[selected] -= reduction;
  saveCounters();
  showMessage("RELEASED", String(reduction) + " STAFF");
}

void clearCounter(uint8_t index) {
  waiting[index] = 0;
  exitedCount = 0;
  saveCounters();
  showMessage(code(index), "RESET TO ZERO");
}

bool allButtonsDown() {
  return digitalRead(P[0]) == LOW && digitalRead(P[1]) == LOW && digitalRead(P[2]) == LOW;
}

bool resetChordActive = false;

void processResetChord() {
  static uint32_t started = 0;
  static uint32_t lastProgress = 0;
  static bool completed = false;

  if (mode != Mode::NORMAL && mode != Mode::RESET_SELECT) return;

  if (allButtonsDown()) {
    if (started == 0) started = millis();
    const uint32_t held = millis() - started;
    const uint32_t total = holdMs();

    if (!completed && millis() - lastProgress >= 80) {
      lastProgress = millis();
      progressScreen("HOLD ALL 3 = RESET", held, total);
    }

    if (!completed && held >= total) {
      completed = true;
      resetChordActive = true;
      mode = Mode::RESET_SELECT;
      beep(1500, 150);
      delay(60);
      beep(2200, 150);
      resetSelectScreen();
    }
  } else {
    started = 0;
    completed = false;
    resetChordActive = false;
  }
}

void shortPress(uint8_t index) {
  if (mode == Mode::NORMAL) addReady(index);
  else if (mode == Mode::RELEASE && index == selected) addExit();
}

void longPress(uint8_t index) {
  if (mode == Mode::NORMAL) {
    startRelease(index);
  } else if (mode == Mode::RELEASE && index == selected) {
    mode = Mode::CONFIRM_EXIT;
    beep();
    confirmExitScreen();
  } else if (mode == Mode::CONFIRM_EXIT && index == selected) {
    mode = Mode::CONFIRM_REDUCE;
    beep();
    confirmReduceScreen();
  } else if (mode == Mode::CONFIRM_REDUCE && index == selected) {
    applyReduction();
  } else if (mode == Mode::RESET_SELECT) {
    clearCounter(index);
  }
}

void processButton(uint8_t index) {
  if (resetChordActive || allButtonsDown()) return;

  Btn &button = btn[index];
  const uint32_t now = millis();
  const bool raw = digitalRead(P[index]);

  if (raw != button.raw) {
    button.raw = raw;
    button.changed = now;
  }

  if (now - button.changed >= settings().debounceMs && raw != button.stable) {
    button.stable = raw;
    if (!raw) {
      button.pressed = now;
      button.longFired = false;
    } else if (!button.longFired && now - button.pressed < holdMs()) {
      shortPress(index);
    }
  }

  const bool active =
    mode == Mode::NORMAL ||
    (mode == Mode::RELEASE && index == selected) ||
    (mode == Mode::CONFIRM_EXIT && index == selected) ||
    (mode == Mode::CONFIRM_REDUCE && index == selected) ||
    mode == Mode::RESET_SELECT;

  if (button.stable == LOW && active && !button.longFired) {
    const uint32_t held = now - button.pressed;
    const uint32_t total = holdMs();

    if (held >= total) {
      button.longFired = true;
      longPress(index);
    } else if (now - lastUi >= 80) {
      lastUi = now;
      String label;
      if (mode == Mode::RESET_SELECT) label = "RESET " + code(index);
      else if (mode == Mode::NORMAL) label = "HOLD TO RELEASE";
      else if (mode == Mode::RELEASE) label = "HOLD TO CONFIRM";
      else if (mode == Mode::CONFIRM_EXIT) label = "HOLD TO REDUCE";
      else label = "HOLD TO COMPLETE";
      progressScreen(label, held, total);
    }
  }
}

String stateJson() {
  String json = "{";
  json += "\"deviceId\":\"" + jsonEsc(settings().deviceId) + "\",";
  json += "\"deviceName\":\"" + jsonEsc(settings().deviceName) + "\",";
  json += "\"showroom\":\"" + jsonEsc(settings().showroom) + "\",";
  json += "\"mode\":" + String((uint8_t)mode) + ",";
  json += "\"selected\":\"" + jsonEsc(code(selected)) + "\",";
  json += "\"waiting\":[" + String(waiting[0]) + "," + String(waiting[1]) + "," + String(waiting[2]) + "],";
  json += "\"exited\":" + String(exitedCount) + ",";
  json += "\"holdMs\":" + String(holdMs()) + ",";
  json += "\"wifi\":\"" + String(WiFi.status() == WL_CONNECTED ? "ONLINE" : "LOCAL AP") + "\"";
  json += "}";
  return json;
}

String dashboardPage() {
  String html = R"HTML(<!doctype html><html><head><meta name="viewport" content="width=device-width,initial-scale=1"><title>JSPL Transport Gate Controller</title><style>*{box-sizing:border-box}body{margin:0;background:#080b12;color:#fff;font-family:system-ui}.wrap{max-width:760px;margin:auto;padding:16px}.top{display:flex;justify-content:space-between;gap:12px}.brand{font-size:22px;font-weight:800}.sub,.small{font-size:12px;opacity:.65}.grid{display:grid;grid-template-columns:repeat(3,1fr);gap:10px;margin:18px 0}.card,.panel{background:#151a24;border:1px solid #293142;border-radius:16px;padding:16px}.card{text-align:center}.code{opacity:.65}.name{font-size:12px;white-space:nowrap;overflow:hidden;text-overflow:ellipsis}.num{font-size:46px;font-weight:850}.panel{margin-top:10px}.mode{font-size:20px;font-weight:800}.big{font-size:36px;font-weight:850}.reset{display:grid;grid-template-columns:repeat(3,1fr);gap:8px;margin-top:10px}.btn,.link{display:block;width:100%;padding:12px;border:0;border-radius:10px;background:#fff;color:#080b12;font-weight:800;text-align:center;text-decoration:none}.danger{background:#d33;color:#fff}.link{margin-top:12px}@media(max-width:480px){.num{font-size:38px}.card{padding:10px}}</style></head><body><div class="wrap"><div class="top"><div><div class="brand">)HTML";
  html += esc(settings().deviceName);
  html += R"HTML(</div><div class="sub">)HTML";
  html += esc(settings().showroom) + " • " + esc(settings().deviceId);
  html += R"HTML(</div></div><div class="small">● )HTML";
  html += WiFi.status() == WL_CONNECTED ? "ONLINE" : "LOCAL AP";
  html += R"HTML(</div></div><div class="grid">)HTML";

  for (uint8_t i = 0; i < N; ++i) {
    html += "<div class='card'><div class='code'>" + esc(code(i)) + "</div><div class='name'>" + esc(name(i)) + "</div><div class='num' id='c" + String(i) + "'>" + String(waiting[i]) + "</div></div>";
  }

  html += R"HTML(</div><div class="panel"><div class="small">CURRENT MODE</div><div class="mode" id="mode">NORMAL</div><div class="small">EXITED IN CURRENT RELEASE</div><div class="big" id="exit">0</div><div class="small" id="sel">—</div></div><div class="panel"><div class="small">COUNTER RESET</div><p>Administrator password is required.</p><div class="reset">)HTML";

  for (uint8_t i = 0; i < N; ++i) {
    html += "<button class='btn danger' onclick='webReset(" + String(i) + ")'>RESET " + esc(code(i)) + "</button>";
  }

  html += R"HTML(</div></div><a class="link" href="/settings">CONFIGURE DEVICE</a><a class="link" href="/network">NETWORK & OTA</a><script>async function poll(){try{let d=await(await fetch('/api/state',{cache:'no-store'})).json();d.waiting.forEach((v,i)=>document.getElementById('c'+i).textContent=v);document.getElementById('mode').textContent=['NORMAL','RELEASE','CONFIRM EXIT','CONFIRM REDUCE','MESSAGE','RESET SELECT'][d.mode]||'NORMAL';document.getElementById('exit').textContent=d.exited;document.getElementById('sel').textContent=d.selected||'—'}catch(e){}}async function webReset(i){let pin=prompt('Administrator password');if(!pin)return;let r=await fetch('/api/counter/reset',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'pin='+encodeURIComponent(pin)+'&index='+i});alert(await r.text());poll()}poll();setInterval(poll,500)</script></div></body></html>)HTML";
  return html;
}

String settingsPage(const String &note = "") {
  const DeviceConfig &c = settings();
  String html = R"HTML(<!doctype html><html><head><meta name="viewport" content="width=device-width,initial-scale=1"><title>JSPL Transport Configuration</title><style>body{margin:0;background:#080b12;color:#fff;font-family:system-ui}.wrap{max-width:700px;margin:auto;padding:16px}.p{background:#151a24;border:1px solid #293142;border-radius:16px;padding:16px;margin:10px 0}label{display:block;font-size:12px;opacity:.7;margin-top:10px}input{width:100%;padding:11px;box-sizing:border-box;border-radius:9px;background:#0d1119;color:#fff;border:1px solid #394255}button,a{display:block;width:100%;padding:12px;margin-top:10px;border:0;border-radius:10px;text-align:center;text-decoration:none;font-weight:800;background:#fff;color:#080b12}.note{padding:10px;background:#10241f;border-radius:10px}.row{display:grid;grid-template-columns:1fr 1fr;gap:10px}.check{display:flex;align-items:center;gap:8px;margin-top:12px}.check input{width:auto}@media(max-width:520px){.row{grid-template-columns:1fr}}</style></head><body><div class="wrap"><h1>Device Configuration</h1>)HTML";
  if (note.length()) html += "<div class='note'>" + esc(note) + "</div>";

  html += "<form method='POST' action='/api/settings/save'><div class='p'><b>IDENTITY & SECURITY</b>";
  html += "<label>Current Administrator Password</label><input name='pin' type='password' required>";
  html += "<label>Device ID</label><input name='deviceId' value='" + esc(c.deviceId) + "'>";
  html += "<label>Device Name</label><input name='deviceName' value='" + esc(c.deviceName) + "'>";
  html += "<label>Showroom</label><input name='showroom' value='" + esc(c.showroom) + "'>";
  html += "<label>Installation Point</label><input name='installation' value='" + esc(c.installation) + "'>";
  html += "<label>New Administrator Password</label><input name='newPin' type='password' placeholder='Leave blank to keep current password'></div>";

  html += "<div class='p'><b>BUS</b><label>Registration Number</label><input name='busReg' value='" + esc(c.busRegistration) + "'><div class='row'><div><label>Capacity</label><input name='busCap' type='number' min='1' max='100' value='" + String(c.busCapacity) + "'></div><div class='check'><input name='busEnabled' type='checkbox' " + String(c.busEnabled ? "checked" : "") + "><span>Bus controller enabled</span></div></div></div>";

  html += "<div class='p'><b>DESTINATIONS</b>";
  for (uint8_t i = 0; i < N; ++i) {
    html += "<label>Button " + String(i + 1) + " Code</label><input name='c" + String(i) + "' value='" + esc(code(i)) + "'>";
    html += "<label>Destination Name</label><input name='n" + String(i) + "' value='" + esc(name(i)) + "'>";
  }
  html += "</div>";

  html += "<div class='p'><b>BUTTON & UI BEHAVIOUR</b><label>All press-and-hold duration (milliseconds)</label><input name='longMs' type='number' min='1000' max='30000' value='" + String(c.longPressMs) + "'><label>Button debounce (milliseconds)</label><input name='debounce' type='number' min='10' max='500' value='" + String(c.debounceMs) + "'><label>Message duration (milliseconds)</label><input name='messageMs' type='number' min='500' max='10000' value='" + String(c.messageMs) + "'></div>";

  html += "<div class='p'><b>NETWORK</b><label>JSPL IoT SSID</label><input name='ssid' value='" + esc(c.wifiSsid) + "'><label>JSPL IoT Password</label><input name='pass' type='password' value='" + esc(c.wifiPassword) + "'></div>";
  html += "<button>SAVE CONFIGURATION</button></form><a href='/'>BACK TO DASHBOARD</a><a href='/network'>NETWORK & OTA</a></div></body></html>";
  return html;
}

bool adminPinOK() {
  return server.hasArg("pin") && server.arg("pin") == settings().adminPin;
}

String cleanValue(String value, size_t maxLength) {
  value.trim();
  if (value.length() > maxLength) value.remove(maxLength);
  value.replace("<", "");
  value.replace(">", "");
  return value;
}

void saveSettingsFromWeb() {
  if (!adminPinOK()) {
    server.send(403, "text/plain", "Invalid administrator password");
    return;
  }

  DeviceConfig c = settings();
  c.deviceId = cleanValue(server.arg("deviceId"), 31);
  c.deviceName = cleanValue(server.arg("deviceName"), 40);
  c.showroom = cleanValue(server.arg("showroom"), 40);
  c.installation = cleanValue(server.arg("installation"), 40);
  c.busRegistration = cleanValue(server.arg("busReg"), 24);
  c.busCapacity = constrain(server.arg("busCap").toInt(), 1, 100);
  c.busEnabled = server.hasArg("busEnabled");

  for (uint8_t i = 0; i < N; ++i) {
    c.destinationCode[i] = cleanValue(server.arg("c" + String(i)), 8);
    c.destinationName[i] = cleanValue(server.arg("n" + String(i)), 24);
  }

  c.longPressMs = constrain(server.arg("longMs").toInt(), 1000, 30000);
  c.debounceMs = constrain(server.arg("debounce").toInt(), 10, 500);
  c.messageMs = constrain(server.arg("messageMs").toInt(), 500, 10000);
  c.wifiSsid = cleanValue(server.arg("ssid"), 64);
  c.wifiPassword = cleanValue(server.arg("pass"), 64);

  const String newPin = cleanValue(server.arg("newPin"), 64);
  if (newPin.length()) c.adminPin = newPin;

  settingsSave(c);

  WiFi.disconnect();
  if (c.wifiSsid.length()) WiFi.begin(c.wifiSsid.c_str(), c.wifiPassword.c_str());
  drawScreen();
  server.send(200, "text/html", settingsPage("Configuration saved successfully."));
}

void webResetCounter() {
  if (!adminPinOK()) {
    server.send(403, "text/plain", "Invalid administrator password");
    return;
  }
  const int index = server.arg("index").toInt();
  if (index < 0 || index >= N) {
    server.send(400, "text/plain", "Invalid counter");
    return;
  }
  clearCounter((uint8_t)index);
  server.send(200, "text/plain", name(index) + " counter reset to zero.");
}

void setupWebServer() {
  server.on("/", HTTP_GET, []() { server.send(200, "text/html", dashboardPage()); });
  server.on("/dashboard", HTTP_GET, []() { server.send(200, "text/html", dashboardPage()); });
  server.on("/api/state", HTTP_GET, []() { server.send(200, "application/json", stateJson()); });
  server.on("/api/counter/reset", HTTP_POST, webResetCounter);
  server.on("/settings", HTTP_GET, []() { server.send(200, "text/html", settingsPage()); });
  server.on("/api/settings/save", HTTP_POST, saveSettingsFromWeb);
  server.begin();
}

void startNetwork() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(DEFAULT_AP_NAME, DEFAULT_AP_PASSWORD);
  if (settings().wifiSsid.length()) {
    WiFi.begin(settings().wifiSsid.c_str(), settings().wifiPassword.c_str());
  }

  Serial.println("=== JSPL TRANSPORT GATE CONTROLLER ===");
  Serial.print("Dashboard: http://");
  Serial.println(WiFi.softAPIP());
  Serial.print("Local AP SSID: ");
  Serial.println(DEFAULT_AP_NAME);
}

void setup() {
  Serial.begin(115200);
  delay(100);

  prefs.begin("jsplcounter", false);
  settingsBegin(prefs);
  loadCounters();

  for (uint8_t i = 0; i < N; ++i) {
    pinMode(P[i], INPUT_PULLUP);
    btn[i].raw = digitalRead(P[i]);
    btn[i].stable = btn[i].raw;
  }

  if (PIN_BUZZER != 255) pinMode(PIN_BUZZER, OUTPUT);

  Wire.begin(OLED_SDA, OLED_SCL);
  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  centerText("JSPL TRANSPORT", 8);
  centerText("GATE CONTROLLER", 25, 1);
  centerText(settings().deviceId, 47);
  display.display();

  startNetwork();
  setupWebServer();
  delay(250);
  drawScreen();
  otaStart();
}

void loop() {
  server.handleClient();
  processResetChord();
  for (uint8_t i = 0; i < N; ++i) processButton(i);

  if (mode == Mode::MESSAGE && millis() >= msgUntil) {
    mode = Mode::NORMAL;
    exitedCount = 0;
    normalScreen();
  }

  if (mode == Mode::NORMAL && millis() - lastUi > 3000) {
    lastUi = millis();
    normalScreen();
  }
}
