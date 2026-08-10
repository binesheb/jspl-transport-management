#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Minimal standalone transport counter.
// No cloud, database, MQTT or Internet is required in this prototype.

constexpr uint8_t OLED_SDA = 21;
constexpr uint8_t OLED_SCL = 22;
constexpr uint8_t OLED_ADDR = 0x3C;
constexpr uint8_t SCREEN_WIDTH = 128;
constexpr uint8_t SCREEN_HEIGHT = 64;
constexpr uint8_t BTN_EXPECTED = 25;
constexpr uint8_t BTN_BOARDED = 26;
constexpr uint8_t BTN_EXITED = 27;
constexpr uint8_t BTN_RESET = 32;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
WebServer server(80);
Preferences prefs;

uint32_t expectedCount = 0;
uint32_t boardedCount = 0;
uint32_t exitedCount = 0;
String busId = "KL-01-AB-1001";
const char *AP_PASSWORD = "jsplbus1";

struct ButtonState {
  uint8_t pin;
  bool lastReading = HIGH;
  bool stableState = HIGH;
  uint32_t changedAt = 0;
};
ButtonState buttons[] = {{BTN_EXPECTED}, {BTN_BOARDED}, {BTN_EXITED}, {BTN_RESET}};

void saveState() {
  prefs.putUInt("expected", expectedCount);
  prefs.putUInt("boarded", boardedCount);
  prefs.putUInt("exited", exitedCount);
  prefs.putString("bus", busId);
}

void loadState() {
  expectedCount = prefs.getUInt("expected", 0);
  boardedCount = prefs.getUInt("boarded", 0);
  exitedCount = prefs.getUInt("exited", 0);
  busId = prefs.getString("bus", "KL-01-AB-1001");
}

void resetTrip() {
  expectedCount = 0;
  boardedCount = 0;
  exitedCount = 0;
  saveState();
}

String statusText() {
  if (expectedCount == 0) return "WAITING";
  if (boardedCount < expectedCount) return "BOARDING";
  return "READY";
}

void drawDisplay() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("JSPL TRANSPORT");
  display.setCursor(0, 10);
  display.print(busId);
  display.setCursor(0, 22);
  display.print("EXPECTED  ");
  display.print(expectedCount);
  display.setCursor(0, 32);
  display.print("BOARDED   ");
  display.print(boardedCount);
  display.setCursor(0, 42);
  display.print("REMAINING ");
  display.print(expectedCount > boardedCount ? expectedCount - boardedCount : 0);
  display.setCursor(0, 54);
  display.print(statusText());
  display.print(" EXIT ");
  display.print(exitedCount);
  display.display();
}

void sendJson() {
  String json = "{\"bus_id\":\"" + busId + "\",\"expected\":" + String(expectedCount) +
                ",\"boarded\":" + String(boardedCount) + ",\"exited\":" + String(exitedCount) +
                ",\"remaining\":" + String(expectedCount > boardedCount ? expectedCount - boardedCount : 0) +
                ",\"status\":\"" + statusText() + "\"}";
  server.send(200, "application/json", json);
}

String htmlPage() {
  return R"HTML(<!doctype html><html><head><meta name="viewport" content="width=device-width,initial-scale=1">
<title>JSPL Transport</title><style>
body{font-family:system-ui;margin:0;padding:20px;background:#111827;color:#fff;text-align:center}.card{max-width:420px;margin:auto;background:#1f2937;border-radius:18px;padding:24px}h1{font-size:22px}.row{display:flex;justify-content:space-between;margin:14px 0}button{font-size:18px;padding:14px 20px;margin:6px;border:0;border-radius:10px}.reset{background:#991b1b;color:white}
</style></head><body><div class="card"><h1>JSPL TRANSPORT</h1><div id="bus"></div><h2 id="status"></h2>
<div class="row"><span>Expected</span><strong id="expected">-</strong></div><div class="row"><span>Boarded</span><strong id="boarded">-</strong></div><div class="row"><span>Remaining</span><strong id="remaining">-</strong></div><div class="row"><span>Exited</span><strong id="exited">-</strong></div>
<button onclick="act('expected')">STAFF LEFT +1</button><button onclick="act('boarded')">BOARD +1</button><button onclick="act('exited')">EXIT BUS +1</button><br><button class="reset" onclick="act('reset')">RESET TRIP</button></div>
<script>async function refresh(){const r=await fetch('/api/state');const d=await r.json();document.getElementById('bus').textContent=d.bus_id;document.getElementById('status').textContent=d.status;for(const k of ['expected','boarded','remaining','exited'])document.getElementById(k).textContent=d[k]}async function act(a){await fetch('/api/'+a,{method:'POST'});refresh()}setInterval(refresh,1000);refresh();</script>
</body></html>)HTML";
}

void handleState() { sendJson(); }
void handleExpected() { expectedCount++; saveState(); drawDisplay(); sendJson(); }
void handleBoarded() { if (boardedCount < expectedCount) boardedCount++; saveState(); drawDisplay(); sendJson(); }
void handleExited() { if (exitedCount < boardedCount) exitedCount++; saveState(); drawDisplay(); sendJson(); }
void handleReset() { resetTrip(); drawDisplay(); sendJson(); }

void setupWebServer() {
  server.on("/", HTTP_GET, []() { server.send(200, "text/html", htmlPage()); });
  server.on("/api/state", HTTP_GET, handleState);
  server.on("/api/expected", HTTP_POST, handleExpected);
  server.on("/api/boarded", HTTP_POST, handleBoarded);
  server.on("/api/exited", HTTP_POST, handleExited);
  server.on("/api/reset", HTTP_POST, handleReset);
  server.begin();
}

void handleButton(ButtonState &button, void (*action)()) {
  bool reading = digitalRead(button.pin);
  if (reading != button.lastReading) button.changedAt = millis();
  if ((millis() - button.changedAt) > 35 && reading != button.stableState) {
    button.stableState = reading;
    if (button.stableState == LOW) action();
  }
  button.lastReading = reading;
}

void setup() {
  Serial.begin(115200);
  prefs.begin("transport", false);
  loadState();
  for (auto &button : buttons) pinMode(button.pin, INPUT_PULLUP);
  Wire.begin(OLED_SDA, OLED_SCL);
  if (display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    display.clearDisplay(); display.setTextColor(SSD1306_WHITE); display.setTextSize(1);
    display.setCursor(0, 0); display.println("JSPL TRANSPORT"); display.println("Starting..."); display.display();
  }
  String suffix = busId.substring(busId.length() > 4 ? busId.length() - 4 : 0);
  String apName = "JSPL-BUS-" + suffix;
  WiFi.mode(WIFI_AP);
  WiFi.softAP(apName.c_str(), AP_PASSWORD);
  Serial.print("Connect to Wi-Fi: "); Serial.println(apName);
  Serial.print("Open: http://"); Serial.println(WiFi.softAPIP());
  setupWebServer();
  drawDisplay();
}

void loop() {
  server.handleClient();
  handleButton(buttons[0], handleExpected);
  handleButton(buttons[1], handleBoarded);
  handleButton(buttons[2], handleExited);
  handleButton(buttons[3], handleReset);
}
