# Captured codes

Decoded from `ir_capture_raw` output. These are the source of truth for Phase 2.

---

## Hyundai fan + light — custom 12-bit PWM

No header. 12 symbols per frame, ~1675µs each, frames separated by a ~7ms gap.

| Symbol | Mark | Space | Means |
|---|---|---|---|
| `1` | ~1225µs | ~450µs | long mark |
| `0` | ~370µs | ~1310µs | short mark |

Frame layout: **`1 1 0 0 0`** (fixed address) + **7 payload bits**.

Every press opens with two preamble frames, identical for all buttons:

| Preamble | Payload |
|---|---|
| 1st | `0000000` |
| 2nd | `1111111` |

Then the command frame repeats while the button is held.

### Button codes

Payload bit 1 is frame symbol 6; payload bit 7 is symbol 12.

| Button | Payload | Symbols set | Confidence |
|---|---|---|---|
| 1H | `0100000` | 7 | 2 clean captures |
| OFF (fan) | `0010000` | 8 | 3 clean captures |
| ON/OFF (light) | `0001000` | 9 | 3 clean captures |
| MED | `0000100` | 10 | 4 clean captures |
| 6H | `0000010` | 11 | 3 clean captures |
| HI | `0000001` | 12 | 2 clean captures |
| 3H | `1000110` | 6, 10, 11 | 2 clean captures, identical |
| LOW | `1000011` | 6, 11, 12 | **1 clean capture** — verify by transmitting |

Six buttons are plain one-hot. `3H` and `LOW` both set symbol 6 plus two others, which looks like a second bank of codes rather than corruption — they reproduce exactly.

`LOW` has only a single clean read. It isn't worth re-capturing: Phase 2 tests it directly, and if the fan doesn't drop to low speed we recapture then.

**Transmit by synthesis, not replay.** The later captures show marks drifting from 1250µs down to 282µs as the receiver's gain control reacted to a too-close remote. Replaying that drift would make a weak transmitter. Build frames from the table above using nominal timings instead.

**Repeat count matters.** Confirmed on the real fan: preamble ×2 plus only 3 command frames is ignored; sending it repeatedly works. The real remote transmits the command frame continuously for as long as the button is held, so the receiver expects a sustained burst rather than a single shot. `send_test` uses **12 repeats**.

If 12 still proves unreliable, suspect the **carrier frequency** rather than the repeat count — the fan's may be 36 or 40kHz rather than the 38kHz default, and a mismatch costs detection margin. `send_test` has a `k` command to cycle it.

---

## Electra A/C — COOLIX

Confirmed by hand-decoding a capture:

- Header ~4480 / ~4480
- Bit mark ~560µs; `1` = ~1660µs space, `0` = ~560µs space
- Each byte followed by its exact inverse → 48 bits on the wire for a 24-bit code
- Leading byte `0xB2`
- Whole message sent twice, separated by a ~5150µs gap

Decoded samples:

| Code | Wire bytes | Meaning |
|---|---|---|
| `0xB2BF60` | `B2 4D BF 40 60 9F` | a state message |
| **`0xB27BE0`** | `B2 4D 7B 84 E0 1F` | **power off** — matches `IRremoteESP8266`'s `kCoolixOff` constant exactly |

The second one is the confirmation that matters: an independently hand-decoded capture landing on a named constant in the library, with all three byte/inverse pairs checking out. Captured twice, identical both times.

`IRremoteESP8266` handles this natively via `IRCoolixAC` — `setTemp()`, `setMode()`, `setFan()`, `setPower()`, `setSwing()`. No need for a per-button code table: the library builds any state on demand.

### Nothing further to record

No code table is needed. `IRCoolixAC` builds any state on demand, so Phase 2 sets temperature, mode and fan directly rather than replaying captures.

### Note on receiver noise

`ir_dump` drowns in 6–10µs glitches. Those are not infrared — no receiver's output stage can switch that fast. They're the ESP32 input retriggering as the divider's slow edges cross the threshold, helped along by breadboard capacitance. `ir_capture_raw` never showed them only because it discards anything whose first pulse is under 200µs.

Receive-side only, and irrelevant to transmitting. If the receiver is ever kept in the final build, the fix is a small capacitor across the divider's lower leg (~10nF; the kit's 100nF is too large and would round off the 560µs pulses).
