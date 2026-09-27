# IR Bridge

An ESP32 that impersonates two infrared remotes, so an air conditioner and a ceiling fan answer to HTTP, a phone page, and Siri.

Built from a starter kit, a spare ESP32-CAM, and two IR LEDs. Total cost of parts not already on the shelf: about a dollar.

---

## What it does

| | |
|---|---|
| **Electra air conditioner** | power, mode, fan speed, and **any temperature from 17–30°C** |
| **Hyundai ceiling fan + light** | high / medium / low / off, light toggle, 1h / 3h / 6h timers |

The A/C temperature is *generated*, not replayed — the bridge can produce setpoints that were never recorded from the original remote.

```
   phone ──"Hey Siri, cool the room"
     │
     │ HTTP over the local network
     ▼
  ESP32 ──GPIO13──► transistor ──► 2× IR LED
                                    │      │
                                    ▼      ▼
                                   A/C    fan
```

---

## The protocols

Both remotes were reverse-engineered with the same ESP32 and a 5V IR receiver. Findings are in [CODES.md](CODES.md).

### Electra A/C → Coolix

Identified by hand-decoding a capture: a ~4480/4480 header, 560µs bit marks, every byte followed by its exact inverse, a leading `0xB2`, and the whole message sent twice with a ~5150µs gap.

The clincher was decoding a power-off capture to `0xB27BE0` — which is `IRremoteESP8266`'s own `kCoolixOff` constant, with all three byte/inverse pairs valid.

Because it's Coolix, `IRCoolixAC` builds any state on demand. No code table needed.

### Hyundai fan → custom 12-bit PWM

No header. 12 symbols per frame, ~1675µs each, frames separated by ~7ms.

| Symbol | Mark | Space |
|---|---|---|
| `1` | ~1225µs | ~450µs |
| `0` | ~370µs | ~1310µs |

Each frame is a fixed 5-bit address `11000` plus 7 payload bits. Every press opens with two invariant preamble frames (`0000000`, then `1111111`), after which the command frame repeats for as long as the button is held.

| Button | Payload |
|---|---|
| 1H | `0100000` |
| Fan off | `0010000` |
| Light toggle | `0001000` |
| Medium | `0000100` |
| 6H | `0000010` |
| High | `0000001` |
| 3H | `1000110` |
| Low | `1000011` |

Six buttons are plain one-hot. `3H` and `LOW` set an extra bit each, reproducibly — apparently a second bank rather than corruption.

**Repeat count matters.** Three command frames is ignored by the fan; twelve works. The receiver expects a sustained burst, because that's what a held button produces.

---

## Hardware

| Part | Notes |
|---|---|
| ESP32-CAM (AI-Thinker) | camera unused — it just happened to be the spare board |
| ESP32-CAM-MB | USB-C programmer and permanent power inlet |
| 2 × IR LED (940nm) | in series, one aimed at each appliance |
| 1 × S8050 NPN transistor | the GPIO supplies ~10mA; the LEDs want ~110mA |
| 2 × 10Ω, 1 × 1kΩ | LED current limit, transistor base |
| 1 × 100µF electrolytic | reservoir for the current pulses |

Full breadboard layout, hole by hole, in [WIRING.md](WIRING.md).

The receiver side (used only for capture, not needed in the final build) is documented in [CAPTURE-GUIDE.md](CAPTURE-GUIDE.md).

---

## Building it

1. Arduino IDE, board **AI Thinker ESP32-CAM**, partition **Huge APP**
2. Libraries: **IRremoteESP8266**, **WiFiManager** (tzapu)
3. **Change `kToken` in `firmware/ir_bridge/ir_bridge.ino` before flashing**
4. Flash, then join the WiFi network **`IR-Bridge-Setup`** and enter your network details
5. The IP prints on Serial at 115200 — give it a DHCP reservation

WiFi credentials live in the ESP32's own flash via WiFiManager and never touch the source. `/reset-wifi` forgets them.

---

## API

Every endpoint takes `k=<your token>`.

| Endpoint | |
|---|---|
| `GET /` | phone page |
| `GET /state` | JSON |
| `GET /ac/on` · `/ac/off` | power |
| `GET /ac/set?t=23&mode=cool&fan=auto` | absolute state; any argument may be omitted |
| `GET /ac/temp?d=-1` | relative nudge, clamped to 17–30 |
| `GET /fan/hi` · `/fan/med` · `/fan/low` · `/fan/off` | fan speed |
| `GET /fan/light` | light toggle |
| `GET /fan/timer?h=1\|3\|6` | timer |
| `GET /reset-wifi` | forget credentials and reopen the setup portal |

Modes: `cool` `heat` `dry` `fan` `auto`. A/C fan: `auto` `min` `med` `max`.

### Siri

One shortcut per command, each a single *Get Contents of URL* action. The shortcut's **name is the spoken phrase**.

Two things that catch people out: **Settings → Privacy & Security → Local Network → Shortcuts** must be on, or requests to a LAN address fail silently. And avoid names like "turn on the air conditioner", which Siri routes to HomeKit instead.

---

## Known limitations

**The light state is unknowable.** Its remote sends a toggle carrying no state, and infrared is one-way. The bridge reports `"light":"unknown"` rather than guessing.

**The A/C is self-correcting.** Coolix puts the complete state in every frame, so whatever the bridge sends becomes the truth. If someone uses the physical remote, one command re-syncs everything.

**The setpoint lives in RAM.** After a power cut the bridge believes 24°C again.

---

## Notes from the build

Things that cost time, recorded so they don't cost anyone else any:

- **The IR receiver module needed 5V, not 3.3V.** At 3.3V its chip doesn't run, but the onboard pull-up still holds the output high — so a dead receiver looks exactly like a healthy idle one. It reads `HIGH` and never changes, which is also what a *working* receiver does when no remote is pointed at it.
- **Breadboard power rails are split down the middle.** Two things a few centimetres apart can sit on opposite, unconnected halves.
- **`GPIO2` and `GPIO12` block booting when held high.** The receiver's divider output held one of them up, so the board uploaded fine with the module unpowered and refused the moment it had 5V. `GPIO15` was chosen instead because both of *its* neighbours are harmless.
- **`Invalid head of packet (0x65)` means the chip is talking, not listening** — it booted normally instead of entering download mode. `0x65` is the letter `e`.
- **Tiny microsecond "pulses" in a capture aren't infrared.** No receiver's output stage switches in 6µs. That's the input retriggering on slow edges.
- **Transmit by synthesis, not replay.** Captures taken too close to the remote drift badly as the receiver's gain control hunts. Frames rebuilt from decoded bits are cleaner than the recordings they came from.
