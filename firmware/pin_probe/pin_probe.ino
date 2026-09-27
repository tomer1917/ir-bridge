// Diagnostic only — no IR library involved.
//
// Reports, once per second, what GPIO15 is actually doing:
//   level        = is the pin sitting high or low right now
//   changes/sec  = how many times it flipped during that second
//
// A healthy, powered IR receiver sits HIGH and quiet when idle, and bursts
// into hundreds of changes while a remote is pointed at it.
//
// Reading guide:
//   HIGH,  0 idle  →  module powered and wired correctly. Press a remote:
//                     changes should jump into the hundreds. If it stays 0,
//                     the module isn't seeing infrared.
//   LOW,   0 always → no power reaching the module, or the signal wire is on
//                     the wrong pin / in a dead breadboard row.
//   noisy, random   → nothing is driving the pin at all; the signal wire is
//                     not actually connected to the module's Y pin.

#include <Arduino.h>

const int kProbePin = 15;

void setup() {
  Serial.begin(115200);
  delay(300);
  pinMode(kProbePin, INPUT);
  Serial.printf("\n\nPin probe on GPIO %d\n", kProbePin);
  Serial.println("Idle should read HIGH with 0 changes.\n");
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

  Serial.printf("level: %-4s   changes/sec: %ld\n",
                (highSamples * 2 > samples) ? "HIGH" : "LOW", changes);
}
