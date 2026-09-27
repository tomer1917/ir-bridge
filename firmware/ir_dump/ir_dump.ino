// Phase 1 - IR capture for the Electra A/C remote (and the Hyundai fan remote, if it is IR).
//
// Board:   AI Thinker ESP32-CAM
// Library: IRremoteESP8266 (Library Manager) - works on ESP32 despite the name
//
// Wiring - kit IR Receiver Module (VS1838B):
//   "+" -> 3V3   (3V3, not 5V: keeps the signal line 3.3V-safe)
//   "-" -> GND
//   "S" -> GPIO15
//
// Do NOT use GPIO12 for this: it is a boot-strapping pin that must be LOW at
// reset, and the receiver idles HIGH, which would stop the board from booting.
//
// Usage: open Serial Monitor at 115200, aim a remote from ~10cm, press one
// button at a time and label the output as you go.

#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <IRac.h>

const uint16_t kRecvPin = 15;
const uint32_t kBaudRate = 115200;

// A/C remotes send one long frame carrying the whole state (temp, mode, fan),
// not a short button code - hence the oversized buffer and the long timeout.
const uint16_t kCaptureBufferSize = 1024;
const uint8_t kTimeout = 90;           // ms of silence that ends a frame

// Noise rejection. A real remote frame opens with a long header mark and has
// plenty of transitions; ambient-IR chatter has neither.
const uint16_t kMinUsefulRawLen = 12;   // shorter than any real remote frame

IRrecv irrecv(kRecvPin, kCaptureBufferSize, kTimeout, true);
decode_results results;

uint32_t noiseBursts = 0;
uint32_t lastNoiseReport = 0;

void setup() {
  Serial.begin(kBaudRate);
  delay(500);
  irrecv.setUnknownThreshold(kMinUsefulRawLen);
  irrecv.setTolerance(kTolerance);
  irrecv.enableIRIn();
  Serial.printf("\n\nIR dump ready. Listening on GPIO %d.\n", kRecvPin);
  Serial.println("Press one remote button at a time.\n");
}

// Ambient infrared makes the receiver chatter, and the library records that
// chatter faithfully. With the supply capacitor fitted the noise floor is only
// a couple of stray pulses per second, so this filter is deliberately loose —
// far better to print an occasional junk block than to silently swallow a real
// capture. Only an overflowed buffer or a burst too short to be any remote
// frame gets dropped.
//
// Deliberately NOT checking the opening mark length: the Hyundai fan remote has
// no header at all, and a frame caught mid-stream still carries useful bits.
bool looksLikeNoise() {
  if (results.overflow) return true;
  return results.rawlen < kMinUsefulRawLen;
}

void loop() {
  if (!irrecv.decode(&results)) return;

  if (looksLikeNoise()) {
    noiseBursts++;
    if (millis() - lastNoiseReport > 5000) {
      Serial.printf("[ignored %lu noise bursts - point the module away from "
                    "screens and lights]\n", noiseBursts);
      noiseBursts = 0;
      lastNoiseReport = millis();
    }
    return;
  }

  Serial.println("======================================================");
  Serial.printf("protocol : %s\n",
                typeToString(results.decode_type, results.repeat).c_str());
  Serial.printf("bits     : %d\n", results.bits);
  if (results.overflow)
    Serial.println("WARNING  : buffer overflow - raise kCaptureBufferSize");

  Serial.print(resultToHumanReadableBasic(&results));

  // If the library recognises this as an air conditioner, print the decoded
  // state. Seeing "Electra" here is the good outcome: it means we can build a
  // real thermostat rather than replaying fixed recordings.
  String acDesc = IRAcUtils::resultAcToString(&results);
  if (acDesc.length()) {
    Serial.println("--- decoded A/C state ---");
    Serial.println(acDesc);
  }

  // Paste-able fallback: works even for protocols the library cannot name.
  Serial.println("--- copy this ---");
  Serial.println(resultToSourceCode(&results));
  Serial.println();

  yield();
}
