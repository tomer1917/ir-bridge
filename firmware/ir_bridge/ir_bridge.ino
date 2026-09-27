// IR Bridge — Phase 3. WiFi, a URL API, and a phone page.
//
// Wraps the transmit functions proven in send_test with a web server, so the
// A/C and the ceiling fan answer to HTTP instead of the Serial Monitor.
//
// Board:     AI Thinker ESP32-CAM
// Partition: Huge APP (3MB No OTA/1MB SPIFFS)
// Libraries: IRremoteESP8266, WiFiManager (tzapu)
// Wiring:    IR LED on GPIO13 via transistor — see WIRING.md
//
// First boot creates a WiFi network called "IR-Bridge-Setup". Join it, enter
// your home WiFi details, and the bridge reboots onto your network. The IP is
// printed on Serial at 115200. To re-provision later: visit /reset-wifi.
//
// The A/C speaks Coolix, so any temperature between 17 and 30 works — the
// frame is generated, not replayed. The fan speaks an unknown 12-bit protocol,
// so its frames are synthesised from the decoded bits in CODES.md.

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <ir_Coolix.h>

// ── Configuration ─────────────────────────────────────────────────────────

const uint16_t kIrLedPin = 13;
const char kPortalName[] = "IR-Bridge-Setup";
const char kMdnsName[] = "irbridge";        // http://irbridge.local

// Change this. It is not real security — anyone on your LAN who can load the
// page can read it out of the HTML. Its job is to stop stray requests and
// casual port-scanners from turning your A/C on at 3am, nothing more.
const char kToken[] = "CHANGE_ME_BEFORE_FLASHING";

// ── Hardware ──────────────────────────────────────────────────────────────

IRsend irsend(kIrLedPin);
IRCoolixAC ac(kIrLedPin);
WebServer server(80);

// ── Fan: synthesised 12-bit frames (see CODES.md) ─────────────────────────

const uint16_t kFanMarkOne = 1225, kFanSpaceOne = 450;
const uint16_t kFanMarkZero = 370, kFanSpaceZero = 1310;
const uint16_t kFanGap = 7000;
const uint8_t kFanRepeats = 12;      // a held press on the real remote sends many
const char kFanAddress[] = "11000";

const char kFanLight[] = "0001000";
const char kFanHi[]    = "0000001";
const char kFanMed[]   = "0000100";
const char kFanLow[]   = "1000011";
const char kFanOff[]   = "0010000";
const char kFan1H[]    = "0100000";
const char kFan3H[]    = "1000110";
const char kFan6H[]    = "0000010";

uint16_t fanBuf[400];
uint16_t fanCarrierKHz = 38;

// ── State ─────────────────────────────────────────────────────────────────
//
// Infrared is one-way. Nothing reports back, so this is what we *believe* we
// last commanded. The A/C stays honest because Coolix carries the whole state
// in every frame, so any command re-syncs it. The fan's light is a toggle with
// no state in the message at all, so it is reported as unknown rather than
// guessed — see PLAN.md.

struct {
  bool acPower = false;
  uint8_t acTemp = 24;
  String acMode = "cool";
  String acFan = "auto";
  String fanSpeed = "unknown";
} state;

// ── IR sending ────────────────────────────────────────────────────────────

void appendFanFrame(uint16_t &n, const char *payload) {
  char bits[13];
  strcpy(bits, kFanAddress);
  strcat(bits, payload);

  for (uint8_t i = 0; i < 12; i++) {
    const bool one = (bits[i] == '1');
    fanBuf[n++] = one ? kFanMarkOne : kFanMarkZero;
    fanBuf[n++] = (i == 11) ? kFanGap : (one ? kFanSpaceOne : kFanSpaceZero);
  }
}

void sendFan(const char *payload) {
  uint16_t n = 0;
  appendFanFrame(n, "0000000");     // preamble 1
  appendFanFrame(n, "1111111");     // preamble 2
  for (uint8_t r = 0; r < kFanRepeats; r++) appendFanFrame(n, payload);
  irsend.sendRaw(fanBuf, n, fanCarrierKHz);
}

uint8_t coolixMode(const String &m) {
  if (m == "heat") return kCoolixHeat;
  if (m == "dry") return kCoolixDry;
  if (m == "fan") return kCoolixFan;
  if (m == "auto") return kCoolixAuto;
  return kCoolixCool;
}

uint8_t coolixFan(const String &f) {
  if (f == "min") return kCoolixFanMin;
  if (f == "med") return kCoolixFanMed;
  if (f == "max") return kCoolixFanMax;
  return kCoolixFanAuto;
}

void sendAc() {
  if (!state.acPower) {
    ac.off();
    ac.send();
    return;
  }
  ac.on();
  ac.setMode(coolixMode(state.acMode));
  ac.setTemp(state.acTemp);
  ac.setFan(coolixFan(state.acFan));
  ac.send();
}

// ── Web plumbing ──────────────────────────────────────────────────────────

bool authorised() {
  if (server.arg("k") == kToken) return true;
  server.send(403, "text/plain", "bad or missing token");
  return false;
}

void sendState() {
  String j = "{\"ac\":{";
  j += "\"power\":" + String(state.acPower ? "true" : "false");
  j += ",\"temp\":" + String(state.acTemp);
  j += ",\"mode\":\"" + state.acMode + "\"";
  j += ",\"fan\":\"" + state.acFan + "\"},";
  j += "\"fan\":{\"speed\":\"" + state.fanSpeed + "\",\"light\":\"unknown\"},";
  j += "\"rssi\":" + String(WiFi.RSSI());
  j += ",\"ip\":\"" + WiFi.localIP().toString() + "\"";
  j += ",\"uptime_s\":" + String(millis() / 1000);
  j += "}";
  server.send(200, "application/json", j);
}

void handleAcOn() {
  if (!authorised()) return;
  state.acPower = true;
  sendAc();
  sendState();
}

void handleAcOff() {
  if (!authorised()) return;
  state.acPower = false;
  sendAc();
  sendState();
}

void handleAcSet() {
  if (!authorised()) return;

  if (server.hasArg("t")) {
    int t = server.arg("t").toInt();
    state.acTemp = constrain(t, kCoolixTempMin, kCoolixTempMax);
  }
  if (server.hasArg("mode")) state.acMode = server.arg("mode");
  if (server.hasArg("fan")) state.acFan = server.arg("fan");

  state.acPower = true;             // setting anything implies "on"
  sendAc();
  sendState();
}

void handleFan(const char *payload, const char *label) {
  if (!authorised()) return;
  sendFan(payload);
  if (label) state.fanSpeed = label;
  sendState();
}

void handleFanTimer() {
  if (!authorised()) return;
  const int h = server.arg("h").toInt();
  if (h == 1) sendFan(kFan1H);
  else if (h == 3) sendFan(kFan3H);
  else if (h == 6) sendFan(kFan6H);
  else {
    server.send(400, "text/plain", "h must be 1, 3 or 6");
    return;
  }
  sendState();
}

// Relative temperature, so Siri can nudge it without naming a number.
// Works from the bridge's remembered setpoint, which is also what every Coolix
// frame carries — so the unit ends up agreeing with us either way.
void handleAcTemp() {
  if (!authorised()) return;

  const int d = server.arg("d").toInt();
  if (d == 0) {
    server.send(400, "text/plain", "d must be non-zero, e.g. d=1 or d=-1");
    return;
  }

  state.acTemp = constrain((int)state.acTemp + d, kCoolixTempMin, kCoolixTempMax);
  state.acPower = true;             // nudging the temperature implies "on"
  sendAc();
  sendState();
}

void handleResetWifi() {
  if (!authorised()) return;
  server.send(200, "text/plain", "forgetting WiFi, rebooting into setup portal");
  delay(300);
  WiFiManager wm;
  wm.resetSettings();
  ESP.restart();
}

// ── Phone page ────────────────────────────────────────────────────────────

const char kPage[] PROGMEM = R"HTML(<!DOCTYPE html>
<html lang="en"><head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<title>IR Bridge</title>
<style>
:root{--bg:#14161a;--card:#1e2128;--line:#2c313a;--fg:#e8eaed;--dim:#9aa2af;--accent:#4c9aff;--warm:#ff8a5c}
*{box-sizing:border-box}
body{margin:0;padding:16px;background:var(--bg);color:var(--fg);
 font:16px/1.5 -apple-system,BlinkMacSystemFont,"Segoe UI",system-ui,sans-serif}
h1{font-size:18px;margin:4px 0 16px;font-weight:600}
section{background:var(--card);border:1px solid var(--line);border-radius:14px;
 padding:16px;margin-bottom:14px}
h2{font-size:13px;letter-spacing:.08em;text-transform:uppercase;color:var(--dim);
 margin:0 0 12px;font-weight:600}
.row{display:flex;gap:8px;flex-wrap:wrap}
button{flex:1 1 0;min-width:72px;min-height:48px;padding:12px 8px;font-size:15px;
 font-family:inherit;color:var(--fg);background:#262a33;border:1px solid var(--line);
 border-radius:10px;cursor:pointer;-webkit-tap-highlight-color:transparent}
button:active{background:#333947}
button.on{background:var(--accent);border-color:var(--accent);color:#08121f;font-weight:600}
button.off{background:#3a2a2a;border-color:#4a3434}
.temp{display:flex;align-items:center;gap:14px;margin-bottom:14px}
.temp button{flex:0 0 64px;font-size:26px;min-height:64px}
.deg{flex:1;text-align:center;font-size:44px;font-weight:300;font-variant-numeric:tabular-nums}
.deg span{font-size:20px;color:var(--dim)}
footer{color:var(--dim);font-size:12px;text-align:center;padding:4px 0 20px}
.warn{color:var(--warm)}
</style></head><body>

<h1>IR Bridge</h1>

<section>
  <h2>Air conditioner</h2>
  <div class="temp">
    <button onclick="bump(-1)">&minus;</button>
    <div class="deg"><span id="acstate">&mdash;</span><br><b id="t">24</b><span>&deg;C</span></div>
    <button onclick="bump(1)">+</button>
  </div>
  <div class="row" style="margin-bottom:8px">
    <button onclick="go('/ac/on')">On</button>
    <button class="off" onclick="go('/ac/off')">Off</button>
  </div>
  <div class="row">
    <button onclick="setp('mode','cool')">Cool</button>
    <button onclick="setp('mode','heat')">Heat</button>
    <button onclick="setp('mode','dry')">Dry</button>
    <button onclick="setp('mode','fan')">Fan</button>
  </div>
</section>

<section>
  <h2>Ceiling fan</h2>
  <div class="row" style="margin-bottom:8px">
    <button onclick="go('/fan/hi')">High</button>
    <button onclick="go('/fan/med')">Med</button>
    <button onclick="go('/fan/low')">Low</button>
    <button class="off" onclick="go('/fan/off')">Off</button>
  </div>
  <div class="row">
    <button onclick="go('/fan/timer?h=1')">1h</button>
    <button onclick="go('/fan/timer?h=3')">3h</button>
    <button onclick="go('/fan/timer?h=6')">6h</button>
  </div>
</section>

<section>
  <h2>Light</h2>
  <div class="row"><button onclick="go('/fan/light')">Toggle</button></div>
  <p style="margin:12px 0 0;font-size:13px;color:var(--dim)">
    The remote sends a toggle, so the bridge cannot know whether the light is
    currently on. It reports <span class="warn">unknown</span> rather than guess.</p>
</section>

<footer id="foot">connecting&hellip;</footer>

<script>
const K="__TOKEN__";
let t=24;

function url(p){return p+(p.includes("?")?"&":"?")+"k="+K}

async function go(p){
  document.getElementById("foot").textContent="sending…";
  try{ render(await (await fetch(url(p))).json()); }
  catch(e){ document.getElementById("foot").textContent="failed — "+e; }
}

function setp(k,v){ go("/ac/set?"+k+"="+v) }
function bump(d){ t=Math.min(30,Math.max(17,t+d)); document.getElementById("t").textContent=t;
                  clearTimeout(bump.j); bump.j=setTimeout(()=>go("/ac/set?t="+t),450) }

function render(s){
  t=s.ac.temp;
  document.getElementById("t").textContent=t;
  document.getElementById("acstate").textContent =
    s.ac.power ? s.ac.mode+" · fan "+s.ac.fan : "off";
  document.getElementById("foot").textContent =
    s.ip+" · "+s.rssi+" dBm · up "+Math.floor(s.uptime_s/60)+"m"+
    " · fan "+s.fan.speed;
}

fetch(url("/state")).then(r=>r.json()).then(render)
  .catch(e=>document.getElementById("foot").textContent="offline");
</script>
</body></html>)HTML";

void handleRoot() {
  String page = FPSTR(kPage);
  page.replace("__TOKEN__", kToken);
  server.send(200, "text/html", page);
}

// ── Setup ─────────────────────────────────────────────────────────────────

void setup() {
  Serial.begin(115200);
  delay(400);

  irsend.begin();
  ac.begin();

  WiFiManager wm;
  wm.setConfigPortalTimeout(180);           // give up after 3 min and reboot
  Serial.println(F("\n\n[wifi] connecting, or opening the setup portal..."));
  if (!wm.autoConnect(kPortalName)) {
    Serial.println(F("[wifi] failed / timed out - rebooting"));
    ESP.restart();
  }
  Serial.printf("[wifi] connected: %s  RSSI %d dBm\n",
                WiFi.localIP().toString().c_str(), WiFi.RSSI());

  if (MDNS.begin(kMdnsName)) {
    MDNS.addService("http", "tcp", 80);
    Serial.printf("[mdns] http://%s.local\n", kMdnsName);
  } else {
    Serial.println(F("[mdns] failed - use the IP directly"));
  }

  server.on("/", handleRoot);
  server.on("/state", sendState);

  server.on("/ac/on", handleAcOn);
  server.on("/ac/off", handleAcOff);
  server.on("/ac/set", handleAcSet);
  server.on("/ac/temp", handleAcTemp);

  server.on("/fan/light", []() { handleFan(kFanLight, nullptr); });
  server.on("/fan/hi",    []() { handleFan(kFanHi, "hi"); });
  server.on("/fan/med",   []() { handleFan(kFanMed, "med"); });
  server.on("/fan/low",   []() { handleFan(kFanLow, "low"); });
  server.on("/fan/off",   []() { handleFan(kFanOff, "off"); });
  server.on("/fan/timer", handleFanTimer);

  server.on("/reset-wifi", handleResetWifi);

  server.begin();
  Serial.printf("[http] ready at http://%s/\n", WiFi.localIP().toString().c_str());
}

void loop() {
  server.handleClient();
}
