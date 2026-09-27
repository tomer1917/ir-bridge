# Wi-Fi / Voice Remote Bridge — Build Plan

One ESP32 on the wall replaces two remotes:
- **Electra A/C** — IR ✅ confirmed (camera test)
- **Hyundai fan + light** — IR ✅ confirmed (hand-block test: a hand stops transmission, which rules out RF)

Control path: phone assistant (Siri Shortcut) → HTTP GET → ESP32 on LAN → IR blast.

---

## 0. Feasibility verdict

**Yes. Total spend: $0 — all parts sourced.**

Both targets being IR is the ideal case — one emitter drives both devices, no radio hardware, no antenna, no frequency guessing, and pin pressure on the ESP32-CAM drops to comfortable.

| Need | Status |
|---|---|
| Decode both remotes | ✅ kit's IR Receiver Module (VS1838B) — you have two |
| Transmit IR | ✅ ~15 clear 5mm IR emitters, bag marked "IR TX" (from a friend) |
| Drive the LED hard | ✅ kit's NPN S8050 / PN2222 + resistors |
| Wi-Fi | ✅ ESP32-CAM (owned) — the UNO has none |
| Breadboard / wiring / 5V | ✅ kit |
| 433MHz radio | ~~needed~~ **not needed** |
| Flash the ESP32-CAM | ✅ USB-TTL/FTDI adapter already owned (per the GestureMedia README) |

The UNO is **bench-only**: its IRremote buffer can't hold an A/C frame (100+ bits of state). All final firmware runs on the ESP32-CAM.

---

## 1. Phase 0 — Carrier identification ✅ DONE

| Remote | Verdict | Evidence |
|---|---|---|
| Electra A/C | **IR** | visible flash on the front camera |
| Hyundai fan/light | **IR** | blocking the emitter with a hand kills transmission; uncovering it in the same spot restores it. RF passes through a hand — this is conclusive |

The red blink on the Hyundai is just a transmit-confirmation indicator, unrelated to the emitter.

---

## 2. Bill of materials

**Already owned**
- ESP32-CAM (AI-Thinker)
- Elegoo kit + RFID kit: 2× IR receiver module, 2× UNO, breadboard, jumpers, S8050/PN2222 NPN, 120 resistors, I2C LCD1602, DHT11, USB cable, 9V adapter

**Donated (bags from a friend)**
- **~15 water-clear 5mm, 2-leg — bag marked "IR TX"** → the emitters. This is the part the build was blocked on.
- **~20 dark/smoked 5mm, 2-leg** → IR **photodiodes/phototransistors**. See the warning below.

**To buy**

Nothing is required. One optional item, and only if §4b pushes you that way:

| Part | Why | ~Cost |
|---|---|---|
| *(optional)* second ESP32 board | Only if you want to keep GestureMedia running on the current CAM — see §4b | $5 |

### ⚠ The dark 2-leg parts are NOT capture receivers

A bare photodiode outputs the raw 38kHz carrier — no amplifier, no demodulation, no AGC. `IRremoteESP8266` cannot read that; it expects a demodulated envelope. **Capture stays on the kit's 3-pin VS1838B module** (§3).

Their one real use: a raw photodiode is the only way to *measure* the fan remote's carrier frequency, which the VS1838B filters away by design. Only relevant if replay misbehaves (§5), and only with a scope.

### Verify the bags before building

Clear-vs-dark is not a universal convention — some IR emitters ship in dark-tinted epoxy to hide them. Confirm:

Coin cell across one **clear** LED, long leg to +, viewed on your phone's **front** camera (rear cameras have an IR-cut filter) in a dim room. Violet-white on screen and invisible to the eye = emitter. Repeat with a **dark** one: it should show nothing. That confirms the pair isn't swapped.

A faint visible red glow when driven hard means 850nm rather than 940nm — fine, slightly less efficient at the receiver, and the visible glow helps debugging.

---

## 3. ESP32-CAM pinout

Only these header pins are free (camera and SD unused), and three of them are boot-strapping traps:

| Pin | Use | Notes |
|---|---|---|
| **GPIO13** | **IR LED** (via transistor) | safe |
| **GPIO15** | **IR receiver signal** | chosen over GPIO14: its neighbours `IO13`/`IO14` are harmless, so an off-by-one costs nothing. `IO14`'s neighbour `IO2` and `IO13`'s neighbour `IO12` both block booting when held high |
| **GPIO15** | I2C SDA — optional LCD | safe; LOW at boot only silences the boot log |
| **GPIO2** | I2C SCL — optional LCD | strapping pin: I2C pull-ups hold it HIGH, which can block *download* mode. Unplug the LCD if uploads start failing |
| GPIO12 | ⛔ avoid | MTDI strapping — must be LOW at boot. The IR receiver idles HIGH, which would set flash voltage to 1.8V and stop the board booting |
| GPIO0 | ⛔ reserved | pull to GND to enter flash mode |
| GPIO4 | ⛔ avoid | onboard blinding white flash LED |
| GPIO16 | ⛔ avoid | PSRAM chip select |

**No usable ADC.** ADC2 is dead whenever Wi-Fi is on, and no ADC1 pin (GPIO32–39) is broken out on the AI-Thinker header. So no photoresistor, no analog sensors. DHT11 is digital and fine.

### IR receiver wiring — **the module needs 5V**

Established the hard way: at 3.3V this module's receiver chip does not run, while its onboard `R1` pull-up still holds the output high. A dead receiver is indistinguishable from a healthy idle one. At 5V it works and its `L` LED blinks on every press.

So it runs from 5V, with a **1kΩ/2kΩ divider** bringing its output down to 3.3V for `IO15`. Full build sheet in [WIRING.md](WIRING.md).

Second lesson: the breadboard's power rails are split at the midpoint. Use plain numbered rows as junctions instead.

### IR transmitter circuit — two LEDs in series

```
GPIO13 --[1k]-- B
                S8050 (NPN)  E -- GND
                             C -- IR LED #2 cathode
                    LED #2 anode -- LED #1 cathode
    5V --[22R]-- IR LED #1 anode
```

About 100mA pulsed. **Series, not parallel** — series guarantees both LEDs carry identical current; parallel lets the lower-Vf LED hog it.

Why two: one aims at the A/C, the other rides a flying lead aimed at the ceiling fan's receiver (see §8). Never drive an IR LED straight from a GPIO — you'd get ~10mA and a metre of range.

---

## 4. Flashing — solved

You already own a USB-TTL/FTDI adapter (listed in the GestureMedia hardware table), so just reuse that rig. Known-good settings from that project:

- Board: `AI Thinker ESP32-CAM`
- Partition Scheme: `Huge APP (3MB No OTA/1MB SPIFFS)` — needed; IRremoteESP8266 is large
- PSRAM: `Enabled`
- GPIO0 → GND to enter download mode, then power-cycle

Note that board is an **AliExpress clone with a GC2145 sensor**, not an OV2640. Irrelevant for IR — the AI-Thinker *pinout* is standard, which is all that matters here.

<details>
<summary>Fallback: flashing via an UNO, if the FTDI ever goes missing</summary>

Borrow an UNO's USB-serial chip:

1. UNO **RESET → GND** (holds the ATmega in reset, exposing the 16U2 bridge on D0/D1)
2. UNO **D0 → ESP32 U0R (RX)** through a divider: `D0 --[1k]--*-- U0R`, `* --[2k]-- GND` (UNO is 5V logic; the ESP32 is **not** 5V-tolerant)
3. UNO **D1 → ESP32 U0T (TX)** — direct; 3.3V reads fine as HIGH
4. UNO **GND → ESP32 GND**; power the ESP32 from 5V
5. **GPIO0 → GND**, power-cycle to enter download mode
6. Upload at 115200, remove the GPIO0 jumper, power-cycle to run

You have two UNOs — one could stay permanently wired as the programmer.

</details>

---

## 4b. Board plan — DECIDED: repurpose the CAM for IR only

GestureMedia goes on the shelf; the ESP32-CAM is reflashed as a dedicated IR bridge. That project's git history keeps it recoverable, so nothing is lost.

What this buys:
- **Reliable IR capture.** IR receive is interrupt-driven; the GC2145 clone has no hardware JPEG encoder, so streaming would have the CPU software-encoding every frame and dropping edges. With the camera gone, capture is clean.
- **Comfortable power budget.** No camera draw competing with ~100mA IR pulses.
- **Simpler firmware.** No `esp_camera` init, no 8MB of framebuffer concerns, no boot-halt-if-camera-fails path.

What it does *not* change: the free-pin list is identical. The camera's pins (GPIO 0, 5, 18, 19, 21–23, 25–27, 32, 34–36, 39) were never broken out to the header anyway, so §3 stands as written — GPIO13, 14, 15, 2, and no usable ADC.

**Don't fork `esp32cam_main.ino`.** Start a fresh sketch and lift only the two patterns in §4c; copying the camera scaffolding just to delete it invites leftovers.

---

## 4c. Reuse from GestureMedia

The firmware patterns are already written and debugged — lift them:

- **`WiFiManager` provisioning.** `wm.autoConnect("ESP32-CAM-Setup")` opens a captive portal on first boot instead of hardcoding credentials, with a 180s timeout and reboot on failure. Use the same for the IR bridge (portal name `IR-Bridge-Setup`) and keep a `/reset-wifi` route.
- **`WebServer` + `server.on()` route table.** Exactly the shape §6's IR API needs.
- **Network reality check:** the LAN uses a fixed private subnet, and `config.json` sets `use_mdns: true` while `fallback_ip` duplicates the host address — which reads like mDNS proved unreliable in practice. Reinforces §6: **set a DHCP reservation and address the bridge by IP.**

---

## 5. Phase 1 — Capture the codes

Library: **IRremoteESP8266** (Library Manager; works on ESP32 despite the name). It has a native `ELECTRA_AC` encoder — if the A/C decodes as Electra we get a real thermostat object (`setTemp`, `setMode`, `setFan`), not eight canned recordings.

1. Flash [`ir_dump/ir_dump.ino`](ir_dump/ir_dump.ino), Serial Monitor at 115200.
2. **Sanity-check the rig with kit 2's own IR remote first.** If that decodes, the wiring is proven — so later silence means "odd protocol", not "bad solder joint".
3. Aim each real remote from ~10cm, press every button once, label the output.
4. Targets:
   - **Electra:** `decode_type: ELECTRA_AC` plus a decoded `AC state:` line. If it reports `COOLIX`, `AUX` or `UNKNOWN`, fall back to raw replay of ~8 presets.
   - **Fan:** 8 buttons (ON/OFF, 1H, 3H, 6H, HI, MED, LOW, OFF). Measured: no header, **12 bits** per frame, ~1675µs per symbol — long mark (~1225) + short space (~455) for one state, short mark (~367) + long space (~1312) for the other. Frame repeats every ~7ms while held. Decodes as `UNKNOWN`; raw replay handles it fine.
5. Save every `uint16_t rawData[]` block. That's the guaranteed fallback for anything unnamed.

**Carrier frequency caveat.** The VS1838B is tuned to 38kHz but still hears 36–40kHz at reduced range. If the fan remote only decodes when held very close while the Electra works from across the room, its carrier probably isn't 38kHz — pass 36 or 40 to `sendRaw(raw, len, 40)` in Phase 2 and test each.

---

## 6. Phase 2 — Replay, then serve

1. **Prove transmit over serial before writing any Wi-Fi code.** Send one captured code, watch the A/C beep. This separates IR-circuit faults from network faults — debug them together and you'll lose an evening.
2. Wrap in a web server with a flat URL API:

   ```
   GET /ac/on            /ac/off
   GET /ac/set?t=24&mode=cool&fan=auto
   GET /fan/hi           /fan/med   /fan/low   /fan/off
   GET /fan/timer?h=3
   GET /light/toggle
   GET /state            -> JSON
   ```

   Append a shared secret: `/ac/on?k=<random-token>`. LAN-only, but it stops a curious device on the guest network from freezing your bedroom at 3am.
3. Serve a single-page HTML UI at `/` so the phone has buttons even without voice.
4. Give the ESP32 a **DHCP reservation**. mDNS (`esp32.local`) works on iOS but is unreliable on Android; a fixed IP avoids a whole class of "it randomly stopped working".

---

## 6b. Optional — local LCD status

The I2C LCD1602 from kit 2 fits now that no radio is needed: GPIO13 (IR TX), GPIO14 (IR RX), GPIO15 + GPIO2 (I2C). Shows setpoint, measured room temp, Wi-Fi state.

Two caveats:
- The PCF8574 backpack is a 5V part. At 5V its I2C pull-ups put 5V on non-5V-tolerant ESP32 pins. Run the backpack from 3V3 (works on most, slightly dim) or put a divider on each line.
- Keep it disconnected while flashing (GPIO2 note in §3).

A plain **ESP32 devkit (~$5)** would give onboard USB, 20+ free GPIOs, and working ADC pins — deleting §4, the GPIO12 trap, and the no-analog-sensors limitation in one move. Entirely optional now that the pin budget fits; worth it if you expect this project to grow.

---

## 7. Phase 3 — Voice

**iPhone:** Shortcuts → *Get Contents of URL* → `http://192.168.1.x/ac/on?k=...` → name it **"Turn on the air conditioner"** → "Hey Siri, turn on the air conditioner." One shortcut per command. No cloud, no account, works offline.

**Android:** Google Home routines can no longer fire arbitrary HTTP. Use the *HTTP Request Shortcuts* app, or move to Home Assistant.

Upgrade path: reflash the same hardware with **ESPHome** for a real thermostat card, schedules and native Alexa/Google exposure. The circuit doesn't change — only the firmware.

### Power

Feed the ESP32-CAM from a **5V phone charger into the 5V pin**, not the kit's power supply module. The CAM's onboard regulator is brownout-prone and the kit module's AMS1117 sags under Wi-Fi current spikes. Symptom: endless `Brownout detector was triggered` reboots.

---

## 8. Known risks

| Risk | Mitigation |
|---|---|
| **Two targets, two directions** | The A/C sits high on a wall; the fan's receiver is in the ceiling canopy. One LED probably can't see both. Hence two in series (§3) — one aimed at the A/C, one on a flying lead aimed at the fan or bounced off the ceiling. Decide final placement *before* building the enclosure. |
| ~~Electra not recognised by the library~~ | **Resolved — the A/C speaks COOLIX.** Confirmed by hand-decoding a capture: 4480/4480 header, every byte followed by its exact inverse, leading `0xB2`, ~5040µs gap between the two repeats of each message. 24-bit code, first sample `0xB2BF60`. `IRCoolixAC` gives `setTemp` / `setMode` / `setFan` / `setPower` — a real thermostat, not preset replay. |
| Fan remote decodes as UNKNOWN | Normal for cheap remotes. Raw replay, and try 36/38/40kHz carriers. |
| **Open-loop state** | IR is fire-and-forget. The light's ON/OFF is a *toggle*, so if anyone uses the physical remote the ESP32's idea of state inverts and never recovers. The A/C is safer — Electra sends full state every frame, so any command re-syncs it. No clean fix for the light toggle: report it as "unknown" in `/state` rather than lying, and add the **DHT11** so at least room temperature is ground truth. (The usual fix — a photoresistor watching the lamp — is blocked by §3's no-ADC limitation, but would work on a plain ESP32 devkit.) |
| ESP32-CAM pin mistake | §3 — GPIO12 is the trap |

---

## 9. Order of work

- [x] **Phase 0** — both remotes confirmed IR
- [x] IR emitters sourced (friend's "IR TX" bag) — nothing to buy
- [x] Flashing rig — FTDI already owned and proven on GestureMedia
- [ ] Coin-cell + front-camera check on one clear and one dark LED (§2)
- [x] §4b decided — repurpose the CAM for IR only, fresh sketch
- [x] Capture rig built and debugged (5V receiver + 1k/2k divider + 100nF, GPIO15)
- [x] **Fan remote fully decoded** — all 8 buttons, see [CODES.md](CODES.md)
- [x] **A/C identified as COOLIX** — library handles it natively
- [x] A/C confirmed — hand-decoded `0xB27BE0` matches the library's `kCoolixOff`. **Phase 1 complete**
- [x] Transmit circuit built (2 IR LEDs + S8050 + 2×10R on GPIO13)
- [x] **Both appliances confirmed controlled over serial** — A/C via generated Coolix, fan via synthesised frames
- [ ] WiFi + web server + phone UI
- [ ] Siri Shortcuts
- [ ] Build the transistor + 2×IR LED circuit
- [ ] **Phase 2** — replay one code over serial, then web server + HTML page
- [ ] **Phase 3** — Siri Shortcuts
- [ ] Optional: LCD status, DHT11, enclosure, ESPHome / Home Assistant
