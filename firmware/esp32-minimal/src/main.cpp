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

// JSPL Transport Counter V3
// HW-724 / ESP32-WROOM-32 / SSD1306 128x64 I2C.
// Normal: short press = ready +1; 10s hold = release mode.
// Release: short press = exited +1; 10s hold = confirm exit.
// Confirm exit: 10s hold = reduce waiting count.
// Counter reset: hold ALL 3 buttons for 15s, then hold one destination
// button for 5s to clear only that destination counter.

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
WebServer server(80);
Preferences prefs;

constexpr uint8_t DESTINATION_COUNT = 3;
const uint8_t BUTTON_PINS[DESTINATION_COUNT] = {PIN_KALOOR, PIN_VYTILLA, PIN_VAZHAKALA};
uint32_t waiting[DESTINATION_COUNT] = {0, 0, 0};
uint32_t exitedCount = 0;
uint8_t selectedDestination = 0;

enum class Mode : uint8_t { NORMAL, RELEASE, CONFIRM_EXIT, CONFIRM_REDUCE, MESSAGE, RESET_SELECT };
Mode mode = Mode::NORMAL;
String messageTitle, messageBody;
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

void beep(uint16_t f = 2200, uint16_t d = 45) {
  if (PIN_BUZZER != 255) tone(PIN_BUZZER, f, d);
}

void centerText(const String &s, int16_t y, uint8_t size = 1) {
  display.setTextSize(size);
  int16_t x1, y1; uint16_t w, h;
  display.getTextBounds(s, 0, y, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - (int16_t)w) / 2, y);
  display.print(s);
}

void header(const String &left, const String &right = "") {
  display.setTextSize(1);
  display.setCursor(0, 0); display.print(left);
  if (right.length()) {
    int16_t x1, y1; uint16_t w, h;
    display.getTextBounds(right, 0, 0, &x1, &y1, &w, &h);
    display.setCursor(SCREEN_WIDTH - w, 0); display.print(right);
  }
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
}

void drawNormal() {
  display.clearDisplay();
  header(settings().deviceName, WiFi.status() == WL_CONNECTED ? "*" : "o");
  for (uint8_t i = 0; i < 3; ++i) {
    int x = i * 42;
    display.setTextSize(1);
    display.setCursor(x + (42 - destinationCode(i).length() * 6) / 2, 14);
    display.print(destinationCode(i));
    String v = String(waiting[i]);
    display.setTextSize(v.length() > 3 ? 2 : 3);
    int w = v.length() > 3 ? v.length() * 12 : v.length() * 18;
    display.setCursor(x + (42 - w) / 2, 25); display.print(v);
  }
  display.drawLine(0, 53, 127, 53, SSD1306_WHITE);
  centerText("READY TO BOARD", 56);
  display.display();
}

void drawRelease() {
  display.clearDisplay(); header(destinationCode(selectedDestination), "RELEASE");
  display.setTextSize(1);
  display.setCursor(3, 15); display.print("WAITING");
  display.setCursor(70, 15); display.print("EXITED");
  display.setTextSize(2);
  display.setCursor(3, 25); display.print(waiting[selectedDestination]);
  display.setCursor(70, 25); display.print(exitedCount);
  display.drawLine(0, 45, 127, 45, SSD1306_WHITE);
  centerText("PRESS = EXIT +1", 49);
  centerText("HOLD = CONFIRM", 57);
  display.display();
}

void drawConfirmExit() {
  display.clearDisplay(); header(destinationCode(selectedDestination), "CONFIRM");
  centerText("STAFF EXITED", 16);
  centerText(String(exitedCount), 27, 2);
  centerText("HOLD TO CONTINUE", 49);
  centerText("10 SEC", 57);
  display.display();
}

void drawConfirmReduce() {
  display.clearDisplay(); header(destinationCode(selectedDestination), "FINAL");
  uint32_t remaining = waiting[selectedDestination] > exitedCount ? waiting[selectedDestination] - exitedCount : 0;
  centerText("RELEASE " + String(exitedCount), 15);
  centerText("REMAIN " + String(remaining), 28, 2);
  centerText("HOLD 10 SEC", 51);
  display.display();
}

void drawResetSelect() {
  display.clearDisplay(); header("COUNTER RESET", "ARMED");
  centerText("SELECT COUNTER", 16);
  centerText("HOLD 5 SEC", 29, 2);
  centerText("KAL / VYT / VAZ", 53);
  display.display();
}

void drawMessage() {
  display.clearDisplay(); centerText(messageTitle, 9); centerText(messageBody, 27, 2); display.display();
}

void drawHoldProgress(const String &action, uint32_t held, uint32_t duration) {
  float f = min(1.0f, (float)held / (float)duration);
  uint8_t width = (uint8_t)(f * 118.0f);
  display.clearDisplay(); header("JSPL TRANSPORT", "HOLD");
  centerText(action, 14);
  display.drawRect(4, 30, 120, 13, SSD1306_WHITE);
  if (width > 1) display.fillRect(5, 31, width, 11, SSD1306_WHITE);
  String t = String(held / 1000) + "." + String((held % 1000) / 100) + " / " + String(duration / 1000) + "s";
  centerText(t, 47); centerText("KEEP HOLDING", 57);
  display.display();
}

void drawScreen() {
  switch (mode) {
    case Mode::NORMAL: drawNormal(); break;
    case Mode::RELEASE: drawRelease(); break;
    case Mode::CONFIRM_EXIT: drawConfirmExit(); break;
    case Mode::CONFIRM_REDUCE: drawConfirmReduce(); break;
    case Mode::RESET_SELECT: drawResetSelect(); break;
    default: drawMessage(); break;
  }
}

void showMessage(const String &title, const String &body) {
  messageTitle = title; messageBody = body;
  messageUntil = millis() + settings().messageMs;
  mode = Mode::MESSAGE; beep(2600, 70); drawScreen();
}

void addReady(uint8_t i) {
  if (waiting[i] < MAX_QUEUE_COUNT) { ++waiting[i]; saveCounters(); beep(2300, 35); }
  else beep(700, 120);
  drawScreen();
}

void startRelease(uint8_t i) {
  if (!waiting[i]) { showMessage(destinationCode(i), "QUEUE EMPTY"); return; }
  selectedDestination = i; exitedCount = 0; mode = Mode::RELEASE; beep(1800, 80); drawScreen();
}

void addExit() {
  if (exitedCount < waiting[selectedDestination]) { ++exitedCount; beep(2600, 30); }
  else beep(700, 120);
  drawScreen();
}

void applyReduction() {
  uint32_t released = min(exitedCount, waiting[selectedDestination]);
  waiting[selectedDestination] -= released; saveCounters();
  messageTitle = "RELEASED"; messageBody = String(released) + " STAFF";
  messageUntil = millis() + settings().messageMs; mode = Mode::MESSAGE; beep(3000, 120); drawScreen();
}

void clearCounter(uint8_t i) {
  waiting[i] = 0; saveCounters(); exitedCount = 0;
  showMessage(destinationCode(i), "RESET TO ZERO");
}

bool allThreePressed() {
  return digitalRead(BUTTON_PINS[0]) == LOW && digitalRead(BUTTON_PINS[1]) == LOW && digitalRead(BUTTON_PINS[2]) == LOW;
}

bool anyButtonPressed() {
  return digitalRead(BUTTON_PINS[0]) == LOW || digitalRead(BUTTON_PINS[1]) == LOW || digitalRead(BUTTON_PINS[2]) == LOW;
}

void processResetChord() {
  static bool resetTriggered = false;
  static uint32_t allPressedAt = 0;
  static uint32_t lastProgress = 0;

  if (mode != Mode::NORMAL && mode != Mode::RESET_SELECT) return;

  if (allThreePressed()) {
    if (!allPressedAt) allPressedAt = millis();
    uint32_t held = millis() - allPressedAt;
    if (!resetTriggered && held < 15000 && millis() - lastProgress > 80) {
      lastProgress = millis(); drawHoldProgress("HOLD ALL 3 = RESET", held, 15000);
    }
    if (!resetTriggered && held >= 15000) {
      resetTriggered = true;
      mode = Mode::RESET_SELECT;
      beep(1500, 150); delay(60); beep(2200, 150);
      drawScreen();
    }
  } else {
    allPressedAt = 0;
    resetTriggered = false;
  }
}

void shortPress(uint8_t i) {
  if (mode == Mode::NORMAL) addReady(i);
  else if (mode == Mode::RELEASE && i == selectedDestination) addExit();
}

void longAction(uint8_t i) {
  if (mode == Mode::NORMAL) startRelease(i);
  else if (mode == Mode::RELEASE && i == selectedDestination) { mode = Mode::CONFIRM_EXIT; beep(1800, 80); drawScreen(); }
  else if (mode == Mode::CONFIRM_EXIT && i == selectedDestination) { mode = Mode::CONFIRM_REDUCE; beep(1800, 80); drawScreen(); }
  else if (mode == Mode::CONFIRM_REDUCE && i == selectedDestination) applyReduction();
  else if (mode == Mode::RESET_SELECT) clearCounter(i);
}

void processButton(uint8_t i) {
  ButtonState &b = buttons[i];
  uint32_t now = millis(); bool reading = digitalRead(BUTTON_PINS[i]);
  if (reading != b.raw) { b.raw = reading; b.changedAt = now; }
  if (now - b.changedAt >= settings().debounceMs && reading != b.stable) {
    b.stable = reading;
    if (b.stable == LOW) { b.pressedAt = now; b.longActionFired = false; }
    else if (!b.longActionFired && now - b.pressedAt < settings().longPressMs) shortPress(i);
  }

  bool active = mode == Mode::NORMAL ||
                (mode == Mode::RELEASE && i == selectedDestination) ||
                (mode == Mode::CONFIRM_EXIT && i == selectedDestination) ||
                (mode == Mode::CONFIRM_REDUCE && i == selectedDestination) ||
                mode == Mode::RESET_SELECT;
  if (b.stable == LOW && active && !b.longActionFired) {
    uint32_t held = now - b.pressedAt;
    uint32_t duration = mode == Mode::RESET_SELECT ? 5000 : settings().longPressMs;
    if (held >= duration) { b.longActionFired = true; longAction(i); }
    else if (now - lastUiFrame >= 80) {
      lastUiFrame = now;
      String action = mode == Mode::RESET_SELECT ? "RESET " + destinationCode(i) :
                      mode == Mode::NORMAL ? "HOLD TO RELEASE" :
                      mode == Mode::RELEASE ? "HOLD TO CONFIRM" :
                      mode == Mode::CONFIRM_EXIT ? "HOLD TO REDUCE" : "HOLD TO COMPLETE";
      selectedDestination = i;
      drawHoldProgress(action, held, duration);
    }
  }
}

String jsonEscape(String v) { v.replace("\\", "\\\\"); v.replace("\"", "\\\""); return v; }
String htmlEscape(String v) { v.replace("&", "&amp;"); v.replace("<", "&lt;"); v.replace(">", "&gt;"); v.replace("\"", "&quot;"); return v; }

String jsonState() {
  String j = "{";
  j += "\"deviceId\":\"" + jsonEscape(settings().deviceId) + "\",";
  j += "\"deviceName\":\"" + jsonEscape(settings().deviceName) + "\",";
  j += "\"showroom\":\"" + jsonEscape(settings().showroom) + "\",";
  j += "\"mode\":\"" + String((uint8_t)mode) + "\",";
  j += "\"selected\":\"" + jsonEscape(destinationCode(selectedDestination)) + "\",";
  j += "\"waiting\":[" + String(waiting[0]) + "," + String(waiting[1]) + "," + String(waiting[2]) + "],";
  j += "\"exited\":" + String(exitedCount) + ",\"wifi\":\"" + String(WiFi.status() == WL_CONNECTED ? "ONLINE" : "LOCAL") + "\"}";
  return j;
}

String dashboardHtml() {
  String h = R"HTML(<!doctype html><html><head><meta name="viewport" content="width=device-width,initial-scale=1"><title>JSPL Transport</title><style>
*{box-sizing:border-box}body{margin:0;background:#080b12;color:#fff;font-family:system-ui,-apple-system,sans-serif}.wrap{max-width:700px;margin:auto;padding:16px}.top{display:flex;justify-content:space-between;align-items:center}.brand{font-size:22px;font-weight:800}.sub,.small{font-size:12px;opacity:.65}.grid{display:grid;grid-template-columns:repeat(3,1fr);gap:10px;margin:18px 0}.card,.panel{background:#151a24;border:1px solid #293142;border-radius:16px;padding:16px}.card{text-align:center}.code{font-size:13px;opacity:.65}.name{font-size:12px;white-space:nowrap;overflow:hidden;text-overflow:ellipsis}.num{font-size:46px;line-height:1.05;font-weight:850;margin-top:6px}.panel{margin-top:10px}.mode{font-size:20px;font-weight:800;margin:4px 0}.big{font-size:36px;font-weight:850}.reset{display:grid;grid-template-columns:1fr 1fr 1fr;gap:8px;margin-top:10px}.btn{border:0;border-radius:10px;padding:12px;font-weight:800;background:#fff;color:#080b12;cursor:pointer}.danger{background:#d33;color:#fff}.link{display:block;text-align:center;margin-top:12px;padding:12px;border-radius:10px;background:#fff;color:#080b12;text-decoration:none;font-weight:800}@media(max-width:480px){.grid{gap:6px}.card{padding:10px}.num{font-size:38px}}</style></head><body><div class="wrap"><div class="top"><div><div class="brand">)HTML";
  h += htmlEscape(settings().deviceName);
  h += R"HTML(</div><div class="sub">)HTML" + htmlEscape(settings().showroom) + " • " + htmlEscape(settings().deviceId) + R"HTML(</div></div><div class="small">● )HTML";
  h += WiFi.status() == WL_CONNECTED ? "ONLINE" : "LOCAL AP";
  h += R"HTML(</div></div><div class="grid">)HTML";
  for (uint8_t i=0;i<3;++i) h += "<div class='card'><div class='code'>"+htmlEscape(destinationCode(i))+"</div><div class='name'>"+htmlEscape(destinationName(i))+"</div><div class='num' id='c"+String(i)+"'>0</div></div>";
  h += R"HTML(</div><div class="panel"><div class="small">CURRENT MODE</div><div class="mode" id="mode">NORMAL</div><div class="small">EXITED IN CURRENT RELEASE</div><div class="big" id="exit">0</div><div class="small" id="sel">—</div></div><div class="panel"><div class="small">COUNTER RESET</div><div>Hold the physical 3-button chord for 15 seconds, then hold a destination for 5 seconds.</div><div class="reset">)HTML";
  for(uint8_t i=0;i<3;++i){h += "<button class='btn danger' onclick='resetCounter("+String(i)+")'>RESET "+htmlEscape(destinationCode(i))+"</button>";}
  h += R"HTML(</div></div><a class="link" href="/settings">DEVICE SETTINGS</a><script>async function r(){try{let d=await (await fetch('/api/state',{cache:'no-store'})).json();d.waiting.forEach((v,i)=>document.getElementById('c'+i).textContent=v);document.getElementById('mode').textContent=['NORMAL','RELEASE','CONFIRM EXIT','CONFIRM REDUCE','MESSAGE','RESET SELECT'][d.mode]||'NORMAL';document.getElementById('exit').textContent=d.exited;document.getElementById('sel').textContent=d.selected||'—'}catch(e){}}async function resetCounter(i){let pin=prompt('Administrator PIN');if(!pin)return;let r=await fetch('/api/counter/reset',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'pin='+encodeURIComponent(pin)+'&index='+i});alert(await r.text());r&&location.reload()}r();setInterval(r,500)</script></div></body></html>)HTML";
  return h;
}

String settingsHtml(const String &notice="") {
  const DeviceConfig &c=settings(); String h=R"HTML(<!doctype html><html><head><meta name="viewport" content="width=device-width,initial-scale=1"><title>JSPL Settings</title><style>*{box-sizing:border-box}body{margin:0;background:#080b12;color:#fff;font-family:system-ui}.wrap{max-width:650px;margin:auto;padding:16px}.panel{background:#151a24;border:1px solid #293142;border-radius:16px;padding:16px;margin-bottom:10px}h1{font-size:22px}h2{font-size:14px}label{display:block;font-size:12px;opacity:.65;margin-top:10px}input{width:100%;padding:11px;border-radius:9px;border:1px solid #394255;background:#0d1119;color:#fff;margin-top:4px}button,a{display:block;width:100%;padding:12px;border:0;border-radius:10px;margin-top:10px;text-align:center;text-decoration:none;font-weight:800;background:#fff;color:#080b12}.danger{background:#d33;color:#fff}.note{background:#10241f;padding:10px;border-radius:10px;margin-bottom:10px}</style></head><body><div class="wrap"><h1>Device Settings</h1>)HTML";
  if(notice.length()) h+="<div class='note'>"+htmlEscape(notice)+"</div>";
  h+="<form method='POST' action='/api/settings/save'><div class='panel'><h2>IDENTITY</h2><label>Administrator PIN</label><input name='pin' type='password' required><label>Device ID</label><input name='deviceId' value='"+htmlEscape(c.deviceId)+"'><label>Device Name</label><input name='deviceName' value='"+htmlEscape(c.deviceName)+"'><label>Showroom</label><input name='showroom' value='"+htmlEscape(c.showroom)+"'><label>Installation</label><input name='installation' value='"+htmlEscape(c.installation)+"'></div>";
  h+="<div class='panel'><h2>BUS</h2><label>Registration Number</label><input name='busRegistration' value='"+htmlEscape(c.busRegistration)+"'><label>Capacity</label><input name='busCapacity' type='number' value='"+String(c.busCapacity)+"'><label>Enabled</label><input name='busEnabled' type='checkbox' "+String(c.busEnabled?"checked":"")+"></div>";
  h+="<div class='panel'><h2>DESTINATIONS</h2>"; for(uint8_t i=0;i<3;++i){h+="<label>Button "+String(i+1)+" Code</label><input name='d"+String(i+1)+"code' value='"+htmlEscape(c.destinationCode[i])+"'><label>Name</label><input name='d"+String(i+1)+"name' value='"+htmlEscape(c.destinationName[i])+"'>";} h+="</div>";
  h+="<div class='panel'><h2>OPERATION</h2><label>Long press ms</label><input name='longPressMs' type='number' value='"+String(c.longPressMs)+"'><label>Debounce ms</label><input name='debounceMs' type='number' value='"+String(c.debounceMs)+"'><label>Message ms</label><input name='messageMs' type='number' value='"+String(c.messageMs)+"'></div><div class='panel'><h2>NETWORK</h2><label>Wi-Fi SSID</label><input name='wifiSsid' value='"+htmlEscape(c.wifiSsid)+"'><label>Wi-Fi Password</label><input name='wifiPassword' type='password' value='"+htmlEscape(c.wifiPassword)+"'></div><div class='panel'><h2>SECURITY</h2><label>New Admin PIN</label><input name='newPin' type='password'></div><button type='submit'>SAVE SETTINGS</button></form><a href='/'>BACK TO DASHBOARD</a></div></body></html>";
  return h;
}

bool adminPinOK(){return server.hasArg("pin") && server.arg("pin")==settings().adminPin;}
String clean(String v,size_t n){v.trim();if(v.length()>n)v.remove(n);v.replace("<","");v.replace(">","");return v;}

void handleSettingsSave(){
  if(!adminPinOK()){server.send(403,"text/plain","Invalid administrator PIN");return;}
  DeviceConfig c=settings(); c.deviceId=clean(server.arg("deviceId"),31); c.deviceName=clean(server.arg("deviceName"),40); c.showroom=clean(server.arg("showroom"),40); c.installation=clean(server.arg("installation"),40); c.busRegistration=clean(server.arg("busRegistration"),24); c.busCapacity=constrain(server.arg("busCapacity").toInt(),1,100); c.busEnabled=server.hasArg("busEnabled");
  for(uint8_t i=0;i<3;++i){c.destinationCode[i]=clean(server.arg("d"+String(i+1)+"code"),8);c.destinationName[i]=clean(server.arg("d"+String(i+1)+"name"),24);} c.longPressMs=constrain(server.arg("longPressMs").toInt(),3000,30000); c.debounceMs=constrain(server.arg("debounceMs").toInt(),10,200); c.messageMs=constrain(server.arg("messageMs").toInt(),500,10000); c.wifiSsid=clean(server.arg("wifiSsid"),64); c.wifiPassword=clean(server.arg("wifiPassword"),64); String np=clean(server.arg("newPin"),12); if(np.length())c.adminPin=np; settingsSave(c); WiFi.disconnect(); if(!c.wifiSsid.isEmpty())WiFi.begin(c.wifiSsid.c_str(),c.wifiPassword.c_str()); server.send(200,"text/html",settingsHtml("Settings saved. Reconnecting Wi-Fi.")); drawScreen();
}

void handleCounterResetWeb(){
  if(!adminPinOK()){server.send(403,"text/plain","Invalid administrator PIN");return;} int i=server.arg("index").toInt(); if(i<0||i>=3){server.send(400,"text/plain","Invalid counter");return;} waiting[i]=0; saveCounters(); server.send(200,"text/plain",destinationName(i)+" counter reset to zero."); drawScreen();
}

void setupWebServer(){
  server.on("/",HTTP_GET,[]{server.send(200,"text/html",dashboardHtml());});
  server.on("/dashboard",HTTP_GET,[]{server.send(200,"text/html",dashboardHtml());});
  server.on("/api/state",HTTP_GET,[]{server.send(200,"application/json",jsonState());});
  server.on("/api/counter/reset",HTTP_POST,handleCounterResetWeb);
  server.on("/settings",HTTP_GET,[]{server.send(200,"text/html",settingsHtml());});
  server.on("/api/settings/save",HTTP_POST,handleSettingsSave);
  server.begin();
}

void startNetwork(){
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(DEFAULT_AP_NAME,DEFAULT_AP_PASSWORD);
  if(!settings().wifiSsid.isEmpty())WiFi.begin(settings().wifiSsid.c_str(),settings().wifiPassword.c_str());
  Serial.println("=== JSPL TRANSPORT COUNTER V3 ===");
  Serial.print("LOCAL DASHBOARD: http://");Serial.println(WiFi.softAPIP());
  Serial.print("AP SSID: ");Serial.println(DEFAULT_AP_NAME);
  if(!settings().wifiSsid.isEmpty())Serial.println("STA Wi-Fi connection started.");
}

void setup(){
  Serial.begin(115200); delay(100);
  prefs.begin("jsplcounter",false); settingsBegin(prefs); loadCounters();
  for(uint8_t i=0;i<3;++i){pinMode(BUTTON_PINS[i],INPUT_PULLUP);buttons[i].raw=digitalRead(BUTTON_PINS[i]);buttons[i].stable=buttons[i].raw;}
  pinMode(PIN_BUZZER,OUTPUT);
  Wire.begin(OLED_SDA,OLED_SCL);
  if(!display.begin(SSD1306_SWITCHCAPVCC,OLED_ADDR)){Serial.println("OLED init failed");}
  else{display.clearDisplay();display.setTextColor(SSD1306_WHITE);centerText("JSPL TRANSPORT",8);centerText("HW-724",25,2);centerText(settings().deviceId,47);display.display();}
  startNetwork(); setupWebServer(); drawScreen(); otaStart();
}

void loop(){
  server.handleClient();
  processResetChord();
  for(uint8_t i=0;i<3;++i)processButton(i);
  if(mode==Mode::MESSAGE && millis()>=messageUntil){mode=Mode::NORMAL;exitedCount=0;drawScreen();}
  if(mode==Mode::NORMAL && millis()-lastUiFrame>3000){lastUiFrame=millis();drawNormal();}
}
