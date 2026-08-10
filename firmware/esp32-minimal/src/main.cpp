#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "config.h"

// Palarivattom V1 — three-channel staff availability counter.
//
// NORMAL MODE
//   Short press on a hostel button: +1 waiting staff.
//   Hold the same button for 10 seconds: enter RELEASE MODE.
//
// RELEASE MODE
//   Short press on the selected hostel button: +1 staff exited.
//   Hold for 10 seconds: confirm the physical exit count.
//
// CONFIRM EXIT
//   Hold for 10 seconds: confirm reduction of the waiting queue.
//
// The final action is: waiting -= exited, then return to NORMAL MODE.
// The ESP32 hosts its own Wi-Fi AP and web dashboard. No cloud is required.

Adafruit_SSD1306 display(
    SCREEN_WIDTH,
    SCREEN_HEIGHT,
    &SPI,
    OLED_DC,
    OLED_RST,
    OLED_CS);

WebServer server(80);
Preferences prefs;

constexpr uint8_t HOSTEL_COUNT = 3;
const char *HOSTEL_NAMES[HOSTEL_COUNT] = {"KAL", "VYT", "VAZ"};
const char *HOSTEL_FULL_NAMES[HOSTEL_COUNT] = {"KALOOR", "VYTILLA", "VAZHAKALA"};
const uint8_t BUTTON_PINS[HOSTEL_COUNT] = {PIN_KALOOR, PIN_VYTILLA, PIN_VAZHAKALA};

uint32_t waiting[HOSTEL_COUNT] = {0, 0, 0};
uint32_t exitedCount = 0;
uint8_t selectedHostel = 0;

enum class Mode : uint8_t {
  NORMAL,
  RELEASE,
  CONFIRM_EXIT,
  CONFIRM_REDUCE,
  MESSAGE
};

Mode mode = Mode::NORMAL;
String messageTitle;
String messageBody;
uint32_t messageUntil = 0;

struct ButtonState {
  bool raw = HIGH;
  bool stable = HIGH;
  uint32_t changedAt = 0;
  uint32_t pressedAt = 0;
  bool longActionFired = false;
};

ButtonState buttons[HOSTEL_COUNT];

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
  if (PIN_BUZZER == 255) return;
  tone(PIN_BUZZER, frequency, duration);
}

void showMessage(const String &title, const String &body, uint32_t duration = UI_MESSAGE_MS) {
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
    case Mode::MESSAGE: return "";
  }
  return "";
}

void drawCentered(const String &text, int16_t y, uint8_t size) {
  display.setTextSize(size);
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(text, 0, y, &x1, &y1, &w, &h);
  int16_t x = (SCREEN_WIDTH - static_cast<int16_t>(w)) / 2;
  display.setCursor(x, y);
  display.print(text);
}

void drawNormal() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("PVM-GATE-01");
  display.setCursor(101, 0);
  display.print("*");

  // Three compact columns. Large numbers are the main information.
  for (uint8_t i = 0; i < HOSTEL_COUNT; ++i) {
    int16_t x = 5 + i * 42;
    display.setTextSize(1);
    display.setCursor(x + 9, 14);
    display.print(HOSTEL_NAMES[i]);

    String value = String(waiting[i]);
    display.setTextSize(2);
    int16_t bx = x + 21 - static_cast<int16_t>(value.length()) * 6;
    display.setCursor(bx, 27);
    display.print(value);
  }

  display.drawLine(0, 49, 127, 49, SSD1306_WHITE);
  drawCentered("READY TO BOARD", 53, 1);
  display.display();
}

void drawRelease() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(HOSTEL_FULL_NAMES[selectedHostel]);
  display.setCursor(88, 0);
  display.print("RELEASE");

  display.setTextSize(1);
  display.setCursor(0, 15);
  display.print("WAITING");
  display.setCursor(0, 26);
  display.setTextSize(2);
  display.print(waiting[selectedHostel]);

  display.setTextSize(1);
  display.setCursor(67, 15);
  display.print("EXITED");
  display.setCursor(67, 26);
  display.setTextSize(2);
  display.print(exitedCount);

  display.drawLine(0, 47, 127, 47, SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 51);
  display.print("PRESS = EXIT +1");
  display.display();
}

void drawConfirmExit() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(HOSTEL_FULL_NAMES[selectedHostel]);
  display.setCursor(79, 0);
  display.print("CONFIRM");

  drawCentered("EXITED", 16, 1);
  drawCentered(String(exitedCount), 28, 2);
  drawCentered("HOLD 10s", 48, 1);
  drawCentered("TO CONFIRM", 57, 1);
  display.display();
}

void drawConfirmReduce() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  drawCentered("CONFIRM REDUCE", 0, 1);
  drawCentered(String(exitedCount) + " STAFF", 16, 2);
  drawCentered("WAITING -> " + String(waiting[selectedHostel] - exitedCount), 39, 1);
  drawCentered("HOLD 10s", 54, 1);
  display.display();
}

void drawMessage() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  drawCentered(messageTitle, 7, 1);
  drawCentered(messageBody, 27, 2);
  display.display();
}

void drawHoldProgress(const char *actionLabel, uint32_t heldMs) {
  float fraction = min(1.0f, static_cast<float>(heldMs) / static_cast<float>(LONG_PRESS_MS));
  uint8_t filled = static_cast<uint8_t>(fraction * 118.0f);

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(HOSTEL_FULL_NAMES[selectedHostel]);
  display.setCursor(0, 13);
  display.print(actionLabel);

  display.drawRect(4, 31, 120, 12, SSD1306_WHITE);
  if (filled > 2) display.fillRect(5, 32, filled, 10, SSD1306_WHITE);

  String seconds = String(heldMs / 1000) + "." + String((heldMs % 1000) / 100);
  drawCentered(seconds + " / 10s", 47, 1);
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
  if (waiting[index] == 0) {
    showMessage(HOSTEL_NAMES[index], "QUEUE EMPTY");
    return;
  }
  selectedHostel = index;
  exitedCount = 0;
  mode = Mode::RELEASE;
  beep(1800, 80);
  drawDisplay();
}

void registerReady(uint8_t index) {
  if (waiting[index] < 9999) waiting[index]++;
  saveState();
  beep(2400, 35);
  drawDisplay();
}

void registerExit() {
  if (exitedCount < waiting[selectedHostel]) {
    exitedCount++;
    beep(2600, 30);
  } else {
    beep(900, 100);
  }
  drawDisplay();
}

void confirmExit() {
  if (exitedCount == 0) {
    showMessage("NO EXIT COUNT", "NOTHING TO CONFIRM");
    return;
  }
  mode = Mode::CONFIRM_EXIT;
  beep(1800, 80);
  drawDisplay();
}

void confirmReduction() {
  mode = Mode::CONFIRM_REDUCE;
  beep(1800, 80);
  drawDisplay();
}

void applyReduction() {
  uint32_t released = exitedCount;
  if (released > waiting[selectedHostel]) released = waiting[selectedHostel];
  waiting[selectedHostel] -= released;
  saveState();

  String title = "RELEASED";
  String body = String(released) + " STAFF";
  mode = Mode::MESSAGE;
  messageTitle = title;
  messageBody = body;
  messageUntil = millis() + 1800;
  beep(2800, 100);
  drawDisplay();
}

void handleShortPress(uint8_t index) {
  if (mode == Mode::NORMAL) {
    registerReady(index);
  } else if (mode == Mode::RELEASE && index == selectedHostel) {
    registerExit();
  }
}

void handleLongPress(uint8_t index) {
  if (mode == Mode::NORMAL) {
    enterRelease(index);
  } else if (mode == Mode::RELEASE && index == selectedHostel) {
    confirmExit();
  } else if (mode == Mode::CONFIRM_EXIT && index == selectedHostel) {
    confirmReduction();
  } else if (mode == Mode::CONFIRM_REDUCE && index == selectedHostel) {
    applyReduction();
  }
}

void processButton(uint8_t index) {
  ButtonState &b = buttons[index];
  bool reading = digitalRead(BUTTON_PINS[index]);
  uint32_t now = millis();

  if (reading != b.raw) {
    b.raw = reading;
    b.changedAt = now;
  }

  if ((now - b.changedAt) >= BUTTON_DEBOUNCE_MS && reading != b.stable) {
    b.stable = reading;
    if (b.stable == LOW) {
      b.pressedAt = now;
      b.longActionFired = false;
    } else {
      // Only a release without a completed long action is a short press.
      if (!b.longActionFired && (now - b.pressedAt) < LONG_PRESS_MS) {
        handleShortPress(index);
      }
    }
  }

  if (b.stable == LOW && !b.longActionFired && (now - b.pressedAt) >= LONG_PRESS_MS) {
    b.longActionFired = true;
    handleLongPress(index);
  }

  // Draw the progress bar while the active button is held for a long action.
  bool relevant = (mode == Mode::NORMAL && selectedHostel == index) ||
                  ((mode == Mode::RELEASE || mode == Mode::CONFIRM_EXIT || mode == Mode::CONFIRM_REDUCE) && index == selectedHostel);
  if (b.stable == LOW && relevant && !b.longActionFired) {
    drawHoldProgress(
        mode == Mode::NORMAL ? "HOLD TO RELEASE" :
        mode == Mode::RELEASE ? "HOLD TO CONFIRM EXIT" :
        mode == Mode::CONFIRM_EXIT ? "HOLD TO CONFIRM REDUCE" :
        "HOLD TO COMPLETE",
        now - b.pressedAt);
  }
}

String jsonState() {
  String json = "{\"device\":\"" + String(DEVICE_NAME) + "\",\"mode\":\"" + modeName() + "\"";
  json += ",\"selected\":\"" + String(HOSTEL_NAMES[selectedHostel]) + "\"";
  json += ",\"kaloor\":" + String(waiting[0]);
  json += ",\"vytilla\":" + String(waiting[1]);
  json += ",\"vazhakala\":" + String(waiting[2]);
  json += ",\"exited\":" + String(exitedCount);
  json += "}";
  return json;
}

String htmlPage() {
  return R"HTML(<!doctype html><html><head><meta name="viewport" content="width=device-width,initial-scale=1">
<title>JSPL Palarivattom Counter</title>
<style>
:root{color-scheme:dark}body{font-family:system-ui,-apple-system,sans-serif;background:#080b12;color:#fff;margin:0;padding:18px}.wrap{max-width:520px;margin:auto}.head{display:flex;justify-content:space-between;align-items:center;margin-bottom:16px}.title{font-size:20px;font-weight:700}.online{font-size:12px;padding:5px 9px;border:1px solid #2dd4bf;border-radius:99px;color:#5eead4}.grid{display:grid;grid-template-columns:repeat(3,1fr);gap:8px}.card{background:#151a24;border:1px solid #252c3a;border-radius:14px;padding:16px;text-align:center}.code{font-size:12px;opacity:.7}.count{font-size:42px;font-weight:800;margin-top:4px}.panel{margin-top:12px;background:#151a24;border:1px solid #252c3a;border-radius:14px;padding:16px}.mode{font-weight:700;font-size:18px}.small{opacity:.7;font-size:13px;margin-top:5px}.exit{font-size:32px;font-weight:800;margin-top:8px}.bar{height:8px;background:#2b3240;border-radius:9px;overflow:hidden;margin-top:14px}.fill{height:100%;width:0;background:#fff}button{width:100%;padding:13px;margin-top:10px;border:0;border-radius:10px;font-weight:700;font-size:15px;background:#fff;color:#080b12}
</style></head><body><div class="wrap"><div class="head"><div class="title">PVM-GATE-01</div><div class="online">LOCAL ESP32</div></div>
<div class="grid"><div class="card"><div class="code">KAL</div><div class="count" id="kal">0</div></div><div class="card"><div class="code">VYT</div><div class="count" id="vyt">0</div></div><div class="card"><div class="code">VAZ</div><div class="count" id="vaz">0</div></div></div>
<div class="panel"><div class="mode" id="mode">NORMAL</div><div class="small" id="selected">Ready queue counter</div><div class="small">Current exit count</div><div class="exit" id="exited">0</div><div class="bar"><div class="fill" id="fill"></div></div></div>
<div class="panel"><div class="small">Physical buttons are the primary control. The web page is a live monitor for this V1 counter.</div></div>
<script>async function refresh(){try{const r=await fetch('/api/state');const d=await r.json();kal.textContent=d.kaloor;vyt.textContent=d.vytilla;vaz.textContent=d.vazhakala;mode.textContent=d.mode;selected.textContent=d.mode==='NORMAL'?'Ready queue counter':'Selected: '+d.selected;exited.textContent=d.exited}catch(e){}}setInterval(refresh,500);refresh();</script>
</div></body></html>)HTML";
}

void setupWebServer() {
  server.on("/", HTTP_GET, []() { server.send(200, "text/html", htmlPage()); });
  server.on("/api/state", HTTP_GET, []() { server.send(200, "application/json", jsonState()); });
  server.begin();
}

void setup() {
  Serial.begin(115200);
  prefs.begin("pvmcounter", false);
  loadState();

  for (uint8_t i = 0; i < HOSTEL_COUNT; ++i) {
    pinMode(BUTTON_PINS[i], INPUT_PULLUP);
    buttons[i].raw = digitalRead(BUTTON_PINS[i]);
    buttons[i].stable = buttons[i].raw;
  }
  if (PIN_BUZZER != 255) pinMode(PIN_BUZZER, OUTPUT);

  SPI.begin(OLED_SCK, -1, OLED_MOSI, OLED_CS);
  if (!display.begin(SSD1306_SWITCHCAPVCC)) {
    Serial.println("OLED init failed. Check SPI wiring/pins.");
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  drawCentered("JSPL TRANSPORT", 10, 1);
  drawCentered("PVM-GATE-01", 27, 2);
  drawCentered("STARTING", 51, 1);
  display.display();

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_NAME, AP_PASSWORD);
  Serial.println();
  Serial.println("=== JSPL PVM COUNTER ===");
  Serial.print("Wi-Fi: ");
  Serial.println(AP_NAME);
  Serial.print("Password: ");
  Serial.println(AP_PASSWORD);
  Serial.print("Web: http://");
  Serial.println(WiFi.softAPIP());

  setupWebServer();
  delay(500);
  drawDisplay();
}

void loop() {
  server.handleClient();

  for (uint8_t i = 0; i < HOSTEL_COUNT; ++i) {
    processButton(i);
  }

  if (mode == Mode::MESSAGE && millis() >= messageUntil) {
    mode = Mode::NORMAL;
    exitedCount = 0;
    drawDisplay();
  }
}
