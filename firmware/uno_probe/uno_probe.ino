// Same diagnostic as pin_probe, but for an Arduino UNO — so the module runs
// at the 5V these boards are designed for, with no risk to the ESP32.
//
// Wiring (module -> UNO):
//   G -> GND
//   R -> 5V        <-- the whole point of this test
//   Y -> pin 2
//
// Serial Monitor at 115200.
//
// Reading guide:
//   HIGH, 0 idle, hundreds of changes when a remote is pressed
//        -> it IS an IR receiver, it just needs 5V. See the level-shift note.
//   HIGH, 0 no matter what
//        -> not an IR receiver (or it's dead). Use the other kit's module.

#include <Arduino.h>

const int kProbePin = 2;

void setup() {
  Serial.begin(115200);
  delay(300);
  pinMode(kProbePin, INPUT);
  Serial.print(F("\n\nUNO pin probe on D"));
  Serial.println(kProbePin);
  Serial.println(F("Idle should read HIGH with 0 changes.\n"));
}

void loop() {
  const unsigned long start = millis();
  int last = digitalRead(kProbePin);
  long changes = 0, highSamples = 0, samples = 0;

  while (millis() - start < 1000) {
    const int v = digitalRead(kProbePin);
    if (v != last) {
      changes++;
      last = v;
    }
    highSamples += v;
    samples++;
  }

  Serial.print(F("level: "));
  Serial.print((highSamples * 2 > samples) ? F("HIGH") : F("LOW "));
  Serial.print(F("   changes/sec: "));
  Serial.println(changes);
}
