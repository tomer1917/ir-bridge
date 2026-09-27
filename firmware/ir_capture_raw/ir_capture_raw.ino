// Raw IR capture with no library involved.
//
// IRremoteESP8266 relies on a hardware timer whose API changed in ESP32 core
// 3.x; a mismatch there yields plausible-looking but wrong pulse lengths. This
// sketch sidesteps all of it: a pin-change interrupt timestamps every edge with
// micros(), and the main loop prints the gaps once the air goes quiet.
//
// Wiring: unchanged. Receiver output through the 1k/2k divider into GPIO15.
//
// What good output looks like: the first few numbers should be in the
// thousands (a header mark), followed by a long run of values repeating
// between roughly two sizes (the 0 and 1 bits). Electra sends ~100+ numbers;
// a simple remote sends ~68.

#include <Arduino.h>

const int kIrPin = 15;
const uint16_t kMaxEdges = 1500;       // generous: Electra frames are ~250
const uint32_t kFrameGapUs = 250000;   // 250ms; long enough to survive brief dropouts
const uint16_t kMinEdges = 20;          // fewer than this is ambient noise
const uint32_t kMinHeaderUs = 200;      // a real frame opens with a long mark

volatile uint32_t edgeAt[kMaxEdges];
volatile uint16_t edgeCount = 0;
volatile uint32_t lastEdgeUs = 0;

void IRAM_ATTR onEdge() {
  const uint32_t now = micros();
  if (edgeCount < kMaxEdges) edgeAt[edgeCount++] = now;
  lastEdgeUs = now;
}

void setup() {
  Serial.begin(115200);
  delay(300);
  pinMode(kIrPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(kIrPin), onEdge, CHANGE);
  Serial.printf("\n\nRaw IR capture on GPIO %d.\n", kIrPin);
  Serial.println("Press one remote button at a time.\n");
}

void loop() {
  if (edgeCount == 0) return;
  if (micros() - lastEdgeUs < kFrameGapUs) return;  // frame still arriving

  // Detach rather than disable interrupts globally — safer on the ESP32, which
  // has WiFi and system tasks that dislike having interrupts held off.
  detachInterrupt(digitalPinToInterrupt(kIrPin));
  static uint32_t frame[kMaxEdges];
  const uint16_t n = edgeCount;
  for (uint16_t i = 0; i < n; i++) frame[i] = edgeAt[i];
  edgeCount = 0;
  attachInterrupt(digitalPinToInterrupt(kIrPin), onEdge, CHANGE);

  if (n < kMinEdges) return;
  if (n >= 2 && (frame[1] - frame[0]) < kMinHeaderUs) return;

  Serial.println("======================================================");
  Serial.printf("pulses   : %u\n", n - 1);
  Serial.printf("duration : %lu us\n", (unsigned long)(frame[n - 1] - frame[0]));
  Serial.println("--- copy this ---");

  Serial.printf("uint16_t rawData[%u] = {", n - 1);
  for (uint16_t i = 1; i < n; i++) {
    Serial.print(frame[i] - frame[i - 1]);
    if (i < n - 1) Serial.print(", ");
  }
  Serial.println("};");
  Serial.println();
}
