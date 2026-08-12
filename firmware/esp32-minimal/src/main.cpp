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

// JSPL Transport Counter V3 - HW-724 / ESP32-WROOM-32
// OLED: SSD1306 128x64 I2C, SDA 5, SCL 4, address 0x3C.
// Normal: short press +1; 10s hold enters release mode.
// Release: short press counts exited; 10s hold confirms; next 10s hold applies reduction.
// Reset: ALL 3 buttons held 15s -> reset-select mode; one button held 5s -> clear that counter.
// Web dashboard: protected per-counter reset using the administrator password.

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
WebServer server(80);
Preferences prefs;
constexpr uint8_t N=3;
const uint8_t P[N]={PIN_KALOOR,PIN_VYTILLA,PIN_VAZHAKALA};
uint32_t waiting[N]={0,0,0}, exitedCount=0;
uint8_t selected=0;
enum class Mode:uint8_t{NORMAL,RELEASE,CONFIRM_EXIT,CONFIRM_REDUCE,MESSAGE,RESET_SELECT};
Mode mode=Mode::NORMAL;
String msgTitle,msgBody; uint32_t msgUntil=0,lastUi=0;
struct Btn{bool raw=HIGH,stable=HIGH,longFired=false;uint32_t changed=0,pressed=0;};
Btn btn[N];

const String& code(uint8_t i){return settings().destinationCode[i];}
const String& name(uint8_t i){return settings().destinationName[i];}
void saveCounters(){prefs.putUInt("d1",waiting[0]);prefs.putUInt("d2",waiting[1]);prefs.putUInt("d3",waiting[2]);}
void loadCounters(){waiting[0]=prefs.getUInt("d1",0);waiting[1]=prefs.getUInt("d2",0);waiting[2]=prefs.getUInt("d3",0);}
void beep(uint16_t f=2200,uint16_t d=45){if(PIN_BUZZER!=255)tone(PIN_BUZZER,f,d);}
void center(const String&s,int y,int z=1){display.setTextSize(z);int16_t x1,y1;uint16_t w,h;display.getTextBounds(s,0,y,&x1,&y1,&w,&h);display.setCursor((128-(int)w)/2,y);display.print(s);}
void head(const String&a,const String&b=""){display.setTextSize(1);display.setCursor(0,0);display.print(a);if(b.length()){int16_t x,y;uint16_t w,h;display.getTextBounds(b,0,0,&x,&y,&w,&h);display.setCursor(128-w,0);display.print(b);}display.drawLine(0,10,127,10,SSD1306_WHITE);}
void normalScreen(){display.clearDisplay();head(settings().deviceName,WiFi.status()==WL_CONNECTED?"*":"o");for(uint8_t i=0;i<N;i++){int x=i*42;display.setTextSize(1);display.setCursor(x+(42-code(i).length()*6)/2,14);display.print(code(i));String v=String(waiting[i]);display.setTextSize(v.length()>3?2:3);int w=v.length()>3?v.length()*12:v.length()*18;display.setCursor(x+(42-w)/2,25);display.print(v);}display.drawLine(0,53,127,53,SSD1306_WHITE);center("READY TO BOARD",56);display.display();}
void releaseScreen(){display.clearDisplay();head(code(selected),"RELEASE");display.setTextSize(1);display.setCursor(3,15);display.print("WAITING");display.setCursor(70,15);display.print("EXITED");display.setTextSize(2);display.setCursor(3,25);display.print(waiting[selected]);display.setCursor(70,25);display.print(exitedCount);display.drawLine(0,45,127,45,SSD1306_WHITE);center("PRESS = EXIT +1",49);center("HOLD = CONFIRM",57);display.display();}
void confirmExitScreen(){display.clearDisplay();head(code(selected),"CONFIRM");center("STAFF EXITED",16);center(String(exitedCount),27,2);center("HOLD TO CONTINUE",49);center("10 SEC",57);display.display();}
void confirmReduceScreen(){display.clearDisplay();head(code(selected),"FINAL");uint32_t r=waiting[selected]>exitedCount?waiting[selected]-exitedCount:0;center("RELEASE "+String(exitedCount),15);center("REMAIN "+String(r),28,2);center("HOLD 10 SEC",51);display.display();}
void resetScreen(){display.clearDisplay();head("COUNTER RESET","ARMED");center("SELECT COUNTER",16);center("HOLD 5 SEC",29,2);center("KAL / VYT / VAZ",53);display.display();}
void messageScreen(){display.clearDisplay();center(msgTitle,9);center(msgBody,27,2);display.display();}
void progress(const String&a,uint32_t held,uint32_t total){float f=min(1.0f,(float)held/(float)total);uint8_t w=(uint8_t)(f*118);display.clearDisplay();head("JSPL TRANSPORT","HOLD");center(a,14);display.drawRect(4,30,120,13,SSD1306_WHITE);if(w>1)display.fillRect(5,31,w,11,SSD1306_WHITE);center(String(held/1000)+"."+String((held%1000)/100)+" / "+String(total/1000)+"s",47);center("KEEP HOLDING",57);display.display();}
void screen(){switch(mode){case Mode::NORMAL:normalScreen();break;case Mode::RELEASE:releaseScreen();break;case Mode::CONFIRM_EXIT:confirmExitScreen();break;case Mode::CONFIRM_REDUCE:confirmReduceScreen();break;case Mode::RESET_SELECT:resetScreen();break;default:messageScreen();}}
void message(const String&a,const String&b){msgTitle=a;msgBody=b;msgUntil=millis()+settings().messageMs;mode=Mode::MESSAGE;beep(2600,70);screen();}
void addReady(uint8_t i){if(waiting[i]<MAX_QUEUE_COUNT){waiting[i]++;saveCounters();beep(2300,35);}else beep(700,120);normalScreen();}
void startRelease(uint8_t i){if(!waiting[i]){message(code(i),"QUEUE EMPTY");return;}selected=i;exitedCount=0;mode=Mode::RELEASE;beep(1800,80);screen();}
void addExit(){if(exitedCount<waiting[selected]){exitedCount++;beep(2600,30);}else beep(700,120);releaseScreen();}
void applyReduction(){uint32_t n=min(exitedCount,waiting[selected]);waiting[selected]-=n;saveCounters();message("RELEASED",String(n)+" STAFF");}
void clearCounter(uint8_t i){waiting[i]=0;exitedCount=0;saveCounters();message(code(i),"RESET TO ZERO");}
bool all3(){return digitalRead(P[0])==LOW&&digitalRead(P[1])==LOW&&digitalRead(P[2])==LOW;}

// The reset chord is evaluated before individual button processing. While all three
// buttons are down, individual 10-second release actions are suppressed.
bool resetChordActive=false;
void resetChord(){
  static uint32_t started=0,last=0; static bool done=false;
  if(mode!=Mode::NORMAL&&mode!=Mode::RESET_SELECT)return;
  if(all3()){
    if(!started)started=millis();
    uint32_t held=millis()-started;
    if(!done&&held<15000&&millis()-last>80){last=millis();progress("HOLD ALL 3 = RESET",held,15000);}
    if(!done&&held>=15000){done=true;resetChordActive=true;mode=Mode::RESET_SELECT;beep(1500,150);delay(60);beep(2200,150);resetScreen();}
  }else{
    if(started&&done)resetChordActive=false;
    started=0;done=false;
  }
}
void shortPress(uint8_t i){if(mode==Mode::NORMAL)addReady(i);else if(mode==Mode::RELEASE&&i==selected)addExit();}
void longPress(uint8_t i){if(mode==Mode::NORMAL)startRelease(i);else if(mode==Mode::RELEASE&&i==selected){mode=Mode::CONFIRM_EXIT;beep();screen();}else if(mode==Mode::CONFIRM_EXIT&&i==selected){mode=Mode::CONFIRM_REDUCE;beep();screen();}else if(mode==Mode::CONFIRM_REDUCE&&i==selected)applyReduction();else if(mode==Mode::RESET_SELECT)clearCounter(i);}
void button(uint8_t i){
  if(resetChordActive||all3())return;
  Btn&b=btn[i];uint32_t now=millis();bool r=digitalRead(P[i]);
  if(r!=b.raw){b.raw=r;b.changed=now;}
  if(now-b.changed>=settings().debounceMs&&r!=b.stable){b.stable=r;if(!r){b.pressed=now;b.longFired=false;}else if(!b.longFired&&now-b.pressed<settings().longPressMs)shortPress(i);}
  bool active=mode==Mode::NORMAL||(mode==Mode::RELEASE&&i==selected)||(mode==Mode::CONFIRM_EXIT&&i==selected)||(mode==Mode::CONFIRM_REDUCE&&i==selected)||(mode==Mode::RESET_SELECT);
  if(b.stable==LOW&&active&&!b.longFired){uint32_t held=now-b.pressed,total=mode==Mode::RESET_SELECT?5000:settings().longPressMs;if(held>=total){b.longFired=true;longPress(i);}else if(now-lastUi>=80){lastUi=now;String a=mode==Mode::RESET_SELECT?"RESET "+code(i):mode==Mode::NORMAL?"HOLD TO RELEASE":mode==Mode::RELEASE?"HOLD TO CONFIRM":mode==Mode::CONFIRM_EXIT?"HOLD TO REDUCE":"HOLD TO COMPLETE";selected=i;progress(a,held,total);}}
}
String esc(String v){v.replace("&","&amp;");v.replace("<","&lt;");v.replace(">","&gt;");v.replace("\"","&quot;");return v;}
String jesc(String v){v.replace("\\","\\\\");v.replace("\"","\\\"");return v;}
String state(){String j="{";j+="\"deviceId\":\""+jesc(settings().deviceId)+"\",\"deviceName\":\""+jesc(settings().deviceName)+"\",\"showroom\":\""+jesc(settings().showroom)+"\",";j+="\"mode\":"+String((uint8_t)mode)+",\"selected\":\""+jesc(code(selected))+"\",\"waiting\":["+String(waiting[0])+","+String(waiting[1])+","+String(waiting[2])+"] ,\"exited\":"+String(exitedCount)+",\"wifi\":\""+(WiFi.status()==WL_CONNECTED?"ONLINE":"LOCAL")+"\"}";return j;}
String dashboard(){String h=R"HTML(<!doctype html><html><head><meta name="viewport" content="width=device-width,initial-scale=1"><title>JSPL Transport</title><style>*{box-sizing:border-box}body{margin:0;background:#080b12;color:#fff;font-family:system-ui}.wrap{max-width:720px;margin:auto;padding:16px}.top{display:flex;justify-content:space-between}.brand{font-size:22px;font-weight:800}.sub,.small{font-size:12px;opacity:.65}.grid{display:grid;grid-template-columns:repeat(3,1fr);gap:10px;margin:18px 0}.card,.panel{background:#151a24;border:1px solid #293142;border-radius:16px;padding:16px}.card{text-align:center}.code{opacity:.65}.name{font-size:12px;white-space:nowrap;overflow:hidden;text-overflow:ellipsis}.num{font-size:46px;font-weight:850}.panel{margin-top:10px}.mode{font-size:20px;font-weight:800}.big{font-size:36px;font-weight:850}.reset{display:grid;grid-template-columns:repeat(3,1fr);gap:8px;margin-top:10px}.btn,.link{display:block;width:100%;padding:12px;border:0;border-radius:10px;background:#fff;color:#080b12;font-weight:800;text-align:center;text-decoration:none}.danger{background:#d33;color:#fff}.link{margin-top:12px}@media(max-width:480px){.num{font-size:38px}.card{padding:10px}}</style></head><body><div class="wrap"><div class="top"><div><div class="brand">)HTML";h+=esc(settings().deviceName);h+=R"HTML(</div><div class="sub">)HTML"+esc(settings().showroom)+" • "+esc(settings().deviceId)+R"HTML(</div></div><div class="small">● )HTML";h+=WiFi.status()==WL_CONNECTED?"ONLINE":"LOCAL AP";h+=R"HTML(</div></div><div class="grid">)HTML";for(uint8_t i=0;i<3;i++)h+="<div class='card'><div class='code'>"+esc(code(i))+"</div><div class='name'>"+esc(name(i))+"</div><div class='num' id='c"+String(i)+"'>0</div></div>";h+=R"HTML(</div><div class="panel"><div class="small">CURRENT MODE</div><div class="mode" id="mode">NORMAL</div><div class="small">EXITED IN CURRENT RELEASE</div><div class="big" id="exit">0</div><div class="small" id="sel">—</div></div><div class="panel"><div class="small">COUNTER RESET</div><p>Reset one counter directly from the dashboard. Administrator password is required.</p><div class="reset">)HTML";for(uint8_t i=0;i<3;i++)h+="<button class='btn danger' onclick='webReset("+String(i)+")'>RESET "+esc(code(i))+"</button>";h+=R"HTML(</div></div><a class="link" href="/settings">DEVICE SETTINGS</a><script>async function poll(){try{let d=await(await fetch('/api/state',{cache:'no-store'})).json();d.waiting.forEach((v,i)=>document.getElementById('c'+i).textContent=v);document.getElementById('mode').textContent=['NORMAL','RELEASE','CONFIRM EXIT','CONFIRM REDUCE','MESSAGE','RESET SELECT'][d.mode]||'NORMAL';document.getElementById('exit').textContent=d.exited;document.getElementById('sel').textContent=d.selected||'—'}catch(e){}}async function webReset(i){let pin=prompt('Administrator password');if(!pin)return;let r=await fetch('/api/counter/reset',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'pin='+encodeURIComponent(pin)+'&index='+i});alert(await r.text());poll()}poll();setInterval(poll,500)</script></div></body></html>)HTML";return h;}

String settingsPage(const String&note=""){const DeviceConfig&c=settings();String h=R"HTML(<!doctype html><html><head><meta name="viewport" content="width=device-width,initial-scale=1"><title>JSPL Settings</title><style>body{margin:0;background:#080b12;color:#fff;font-family:system-ui}.wrap{max-width:650px;margin:auto;padding:16px}.p{background:#151a24;border:1px solid #293142;border-radius:16px;padding:16px;margin:10px 0}label{display:block;font-size:12px;opacity:.65;margin-top:10px}input{width:100%;padding:11px;box-sizing:border-box;border-radius:9px;background:#0d1119;color:#fff;border:1px solid #394255}button,a{display:block;width:100%;padding:12px;margin-top:10px;border:0;border-radius:10px;text-align:center;text-decoration:none;font-weight:800;background:#fff;color:#080b12}.note{padding:10px;background:#10241f;border-radius:10px}</style></head><body><div class="wrap"><h1>Device Settings</h1>)HTML";if(note.length())h+="<div class='note'>"+esc(note)+"</div>";h+="<form method='POST' action='/api/settings/save'><div class='p'><b>IDENTITY & SECURITY</b><label>Current Administrator Password</label><input name='pin' type='password' required><label>Device ID</label><input name='deviceId' value='"+esc(c.deviceId)+"'><label>Device Name</label><input name='deviceName' value='"+esc(c.deviceName)+"'><label>Showroom</label><input name='showroom' value='"+esc(c.showroom)+"'><label>Installation</label><input name='installation' value='"+esc(c.installation)+"'><label>New Administrator Password</label><input name='newPin' type='password' placeholder='Leave blank to keep current password'><small>Used for dashboard counter reset and protected settings.</small></div><div class='p'><b>BUS</b><label>Registration</label><input name='busReg' value='"+esc(c.busRegistration)+"'><label>Capacity</label><input name='busCap' type='number' value='"+String(c.busCapacity)+"'></div><div class='p'><b>DESTINATIONS</b>";for(uint8_t i=0;i<3;i++){h+="<label>Button "+String(i+1)+" code</label><input name='c"+String(i)+"' value='"+esc(code(i))+"'><label>Name</label><input name='n"+String(i)+"' value='"+esc(name(i))+"'>";}h+="</div><div class='p'><b>NETWORK</b><label>Wi-Fi SSID</label><input name='ssid' value='"+esc(c.wifiSsid)+"'><label>Wi-Fi Password</label><input name='pass' type='password' value='"+esc(c.wifiPassword)+"'></div><button>SAVE SETTINGS</button></form><a href='/'>BACK TO DASHBOARD</a></div></body></html>";return h;}

bool pinOK(){return server.hasArg("pin")&&server.arg("pin")==settings().adminPin;}
String clean(String v,size_t n){v.trim();if(v.length()>n)v.remove(n);v.replace("<","");v.replace(">","");return v;}
void saveSettings(){if(!pinOK()){server.send(403,"text/plain","Invalid administrator password");return;}DeviceConfig c=settings();c.deviceId=clean(server.arg("deviceId"),31);c.deviceName=clean(server.arg("deviceName"),40);c.showroom=clean(server.arg("showroom"),40);c.installation=clean(server.arg("installation"),40);c.busRegistration=clean(server.arg("busReg"),24);c.busCapacity=constrain(server.arg("busCap").toInt(),1,100);for(uint8_t i=0;i<3;i++){c.destinationCode[i]=clean(server.arg("c"+String(i)),8);c.destinationName[i]=clean(server.arg("n"+String(i)),24);}c.wifiSsid=clean(server.arg("ssid"),64);c.wifiPassword=clean(server.arg("pass"),64);String newPin=clean(server.arg("newPin"),64);if(newPin.length())c.adminPin=newPin;settingsSave(c);WiFi.disconnect();if(c.wifiSsid.length())WiFi.begin(c.wifiSsid.c_str(),c.wifiPassword.c_str());server.send(200,"text/html",settingsPage("Saved. Password changes take effect immediately."));screen();}
void webReset(){if(!pinOK()){server.send(403,"text/plain","Invalid administrator password");return;}int i=server.arg("index").toInt();if(i<0||i>=3){server.send(400,"text/plain","Invalid counter");return;}clearCounter(i);server.send(200,"text/plain",name(i)+" counter reset to zero.");}
void web(){server.on("/",HTTP_GET,[]{server.send(200,"text/html",dashboard());});server.on("/dashboard",HTTP_GET,[]{server.send(200,"text/html",dashboard());});server.on("/api/state",HTTP_GET,[]{server.send(200,"application/json",state());});server.on("/api/counter/reset",HTTP_POST,webReset);server.on("/settings",HTTP_GET,[]{server.send(200,"text/html",settingsPage());});server.on("/api/settings/save",HTTP_POST,saveSettings);server.begin();}
void network(){WiFi.mode(WIFI_AP_STA);WiFi.softAP(DEFAULT_AP_NAME,DEFAULT_AP_PASSWORD);if(settings().wifiSsid.length())WiFi.begin(settings().wifiSsid.c_str(),settings().wifiPassword.c_str());Serial.println("=== JSPL TRANSPORT COUNTER V3 ===");Serial.print("DASHBOARD: http://");Serial.println(WiFi.softAPIP());Serial.print("AP SSID: ");Serial.println(DEFAULT_AP_NAME);}
void setup(){Serial.begin(115200);delay(100);prefs.begin("jsplcounter",false);settingsBegin(prefs);loadCounters();for(uint8_t i=0;i<3;i++){pinMode(P[i],INPUT_PULLUP);btn[i].raw=digitalRead(P[i]);btn[i].stable=btn[i].raw;}pinMode(PIN_BUZZER,OUTPUT);Wire.begin(OLED_SDA,OLED_SCL);display.begin(SSD1306_SWITCHCAPVCC,OLED_ADDR);display.clearDisplay();display.setTextColor(SSD1306_WHITE);center("JSPL TRANSPORT",8);center("HW-724",25,2);center(settings().deviceId,47);display.display();network();web();delay(250);screen();otaStart();}
void loop(){server.handleClient();resetChord();for(uint8_t i=0;i<3;i++)button(i);if(mode==Mode::MESSAGE&&millis()>=msgUntil){mode=Mode::NORMAL;exitedCount=0;screen();}if(mode==Mode::NORMAL&&millis()-lastUi>3000){lastUi=millis();normalScreen();}}
