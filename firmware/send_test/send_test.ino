// Phase 2, step 1 — prove the transmitter works. No WiFi yet.
//
// Type a single character into the Serial Monitor and the matching command
// goes out of the IR LED. Debug the optics and the circuit here, while the
// only moving parts are a transistor and two LEDs.
//
// Board:   AI Thinker ESP32-CAM
// Library: IRremoteESP8266
// Wiring:  GPIO13 -> 1k -> S8050 base; 5V -> 22R -> LED -> LED -> collector.
//          See WIRING.md.
//
// The A/C speaks Coolix, so its commands are *generated* rather than replayed —
// any temperature works, including ones never captured. The fan remote is an
// unknown 12-bit protocol, so its frames are synthesised from the decoded bits
// in CODES.md using nominal timings, which is cleaner than replaying captures
// whose pulse lengths drifted as the receiver's gain control hunted.

#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <ir_Coolix.h>

const uint16_t kIrLedPin = 13;
const uint32_t kBaudRate = 115200;

IRsend irsend(kIrLedPin);
IRCoolixAC ac(kIrLedPin);

// ── Hyundai fan/light: synthesised 12-bit frames ──────────────────────────
//
// Symbol timings measured from the remote (CODES.md):
//   '1' = long mark + short space
//   '0' = short mark + long space
// Every frame is a fixed 5-bit address plus 7 payload bits, and each press is
// preceded by two invariant preamble frames.

const uint16_t kFanMarkOne = 1225, kFanSpaceOne = 450;
const uint16_t kFanMarkZero = 370, kFanSpaceZero = 1310;
const uint16_t kFanGap = 7000;      // silence between frames
const uint8_t kFanRepeats = 12;     // a held press on the real remote sends many
const char kFanAddress[] = "11000";

const char kFanLight[] = "0001000";  // ON/OFF (light)
const char kFanHi[]    = "0000001";
const char kFanMed[]   = "0000100";
const char kFanLow[]   = "1000011";
const char kFanOff[]   = "0010000";
const char kFan1H[]    = "0100000";
const char kFan3H[]    = "1000110";
const char kFan6H[]    = "0000010";

uint16_t fanBuf[400];               // 14 frames x 24 entries, with headroom
uint16_t fanCarrierKHz = 38;        // try 36 or 40 if the fan is unreliable

void appendFanFrame(uint16_t &n, const char *payload) {
  char bits[13];
  strcpy(bits, kFanAddress);
  strcat(bits, payload);            // 5 + 7 = 12 symbols

  for (uint8_t i = 0; i < 12; i++) {
    const bool one = (bits[i] == '1');
    fanBuf[n++] = one ? kFanMarkOne : kFanMarkZero;
    // The final space of a frame becomes the inter-frame gap.
    fanBuf[n++] = (i == 11) ? kFanGap : (one ? kFanSpaceOne : kFanSpaceZero);
  }
}

void sendFan(const char *payload, const char *label) {
  uint16_t n = 0;
  appendFanFrame(n, "0000000");     // preamble 1
  appendFanFrame(n, "1111111");     // preamble 2
  for (uint8_t r = 0; r < kFanRepeats; r++) appendFanFrame(n, payload);

  irsend.sendRaw(fanBuf, n, fanCarrierKHz);
  Serial.printf("fan  -> %-6s  [%s]  %u pulses\n", label, payload, n);
}

// ── Electra A/C: generated Coolix state ───────────────────────────────────

void sendAc(bool power, uint8_t temp, const char *label) {
  if (!power) {
    ac.off();
  } else {
    ac.on();
    ac.setMode(kCoolixCool);
    ac.setTemp(temp);
    ac.setFan(kCoolixFanAuto);
  }
  ac.send();
  Serial.printf("a/c  -> %s\n", label);
}

void printMenu() {
  Serial.println(F("\n--- send test -------------------------------"));
  Serial.println(F("  1  A/C on, cool 24C, fan auto"));
  Serial.println(F("  2  A/C on, cool 22C, fan auto"));
  Serial.println(F("  3  A/C on, cool 26C, fan auto"));
  Serial.println(F("  0  A/C off"));
  Serial.println(F("  l  fan: light on/off"));
  Serial.println(F("  h  fan: high      m  fan: medium"));
  Serial.println(F("  w  fan: low       f  fan: off"));
  Serial.println(F("  a  timer 1H       b  timer 3H       c  timer 6H"));
  Serial.println(F("  k  cycle fan carrier 38 / 40 / 36 kHz"));
  Serial.println(F("  ?  this menu"));
  Serial.println(F("---------------------------------------------"));
}

void setup() {
  Serial.begin(kBaudRate);
  delay(500);
  irsend.begin();
  ac.begin();
  Serial.printf("\n\nIR transmitter ready on GPIO %d.\n", kIrLedPin);
  printMenu();
}

void loop() {
  if (!Serial.available()) return;
  const char cmd = Serial.read();

  switch (cmd) {
    case '1': sendAc(true, 24, "on, cool 24C"); break;
    case '2': sendAc(true, 22, "on, cool 22C"); break;
    case '3': sendAc(true, 26, "on, cool 26C"); break;
    case '0': sendAc(false, 0, "off"); break;

    case 'l': sendFan(kFanLight, "light"); break;
    case 'h': sendFan(kFanHi, "hi"); break;
    case 'm': sendFan(kFanMed, "med"); break;
    case 'w': sendFan(kFanLow, "low"); break;
    case 'f': sendFan(kFanOff, "off"); break;
    case 'a': sendFan(kFan1H, "1H"); break;
    case 'b': sendFan(kFan3H, "3H"); break;
    case 'c': sendFan(kFan6H, "6H"); break;

    // Cycle the fan's carrier frequency. If the fan needs several attempts at
    // 38kHz but responds first time at 36 or 40, that's its real carrier and
    // the difference is detection margin, not repeat count.
    case 'k':
      fanCarrierKHz = (fanCarrierKHz == 38) ? 40 : (fanCarrierKHz == 40 ? 36 : 38);
      Serial.printf("fan carrier -> %u kHz\n", fanCarrierKHz);
      break;

    case '?': printMenu(); break;
    default: break;                 // ignore newlines and stray characters
  }
}
