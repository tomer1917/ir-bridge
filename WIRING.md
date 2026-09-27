# Wiring — the authoritative build sheet (Build A, the listening rig)

Strip the breadboard bare and build this from scratch. 11 wires, 2 resistors.

---

## ESP32-CAM pin map

Labels are printed on the **top face** (camera side), down both edges. Read them — never count from memory.

```
LEFT edge (from camera end)      RIGHT edge (from camera end)
  1  5V        ← power in         1  3V3     ← UNUSED
  2  GND       ← ground           2  IO16
  3  IO12      ⚠ blocks boot      3  IO0     ← to MB
  4  IO13                         4  GND
  5  IO15      ← signal in        5  VCC     ← UNUSED (it's an output)
  6  IO14                         6  U0R     ← to MB
  7  IO2       ⚠ blocks upload    7  U0T     ← to MB
  8  IO4                          8  GND/R
```

**`IO15` is the signal pin.** It sits between `IO13` and `IO14`, and both of those are harmless if you land on them by mistake. The two pins that stop the board booting, `IO12` and `IO2`, are two away in either direction.

Nothing ever connects to `3V3` or `VCC`.

---

## Breadboard

Everything in **columns a–e**, one half of the board. **The power rails are not used at all** — leave both long strips empty. (They're split in the middle on this board, which cost us an afternoon.)

Rows used: 30, 31, 32, 36, 40, 50, 55.

```
row 30   a30 module G                        e30 ──────┐
row 31   a31 module R                        e31 ────┐ │
row 32   a32 module Y   c32 ─┐                       │ │
                         1kΩ │                       │ │
row 36                  b36 ─┘  c36 ─┐   d36 ── CAM IO15
                                 2kΩ │               │ │
row 40                  b40 ────────┘   c40 ──┐      │ │
                                               │      │ │
row 50   e50 ── MB 5V    d50 ── CAM 5V    c50 ─┼──────┘ │
row 55   e55 ── MB GND   d55 ── CAM GND   c55 ─┼────────┘
                                          b55 ─┘
```

---

## The 11 wires

### A. MB ↔ CAM — female-to-male, female end on the CAM's pins

| # | MB socket | → CAM pin |
|---|---|---|
| 1 | `U0T` | `U0T` |
| 2 | `U0R` | `U0R` |
| 3 | `IO0` | `IO0` |

Like to like; nothing crosses over.

### B. Power junctions

| # | From | To | Wire type |
|---|---|---|---|
| 4 | MB `5V` socket | **e50** | male-to-male |
| 5 | **d50** | CAM `5V` | female-to-male |
| 6 | MB `GND` socket | **e55** | male-to-male |
| 7 | **d55** | CAM `GND` | female-to-male |

Use the `GND` on the **left edge, immediately next to `5V`** — adjacent pins, easy to get right.

### C. Module power — male-to-male

| # | From | To |
|---|---|---|
| 8 | **c50** | **e31** (module `R`) |
| 9 | **c55** | **e30** (module `G`) |

### C2. Supply filter — not optional

The VS1838B is very sensitive to noise on its supply, and USB power through a breadboard is noisy. Without this the receiver chatters constantly and fills the capture buffer with junk, even in a dark room.

| Component | One leg | Other leg |
|---|---|---|
| **100nF ceramic** (disc marked `104`) | **b31** (module `R`) | **b30** (module `G`) |

No polarity. Keep it physically close to the module.

### D. The divider

| Component | One leg | Other leg |
|---|---|---|
| **1kΩ** | **c32** (module `Y`) | **b36** |
| **2kΩ** | **c36** | **b40** |

| # | From | To | Wire type |
|---|---|---|---|
| 10 | **c40** | **b55** | male-to-male |

**Order matters.** 1kΩ on top from the module, 2kΩ below to ground. Reversed gives 1.7V at the tap instead of 3.3V.

### E. The signal

| # | From | To | Wire type |
|---|---|---|---|
| 11 | **d36** | CAM `IO15` | female-to-male |

---

---

# Transmitter (Build B)

Additive — nothing above comes apart. Rows 10–25 are free and well clear of the receiver.

## Parts

| Part | Where from |
|---|---|
| 2 × IR LED | the clear ones, bag marked "IR TX" |
| 1 × S8050 or PN2222 transistor | black half-cylinder, three legs |
| **2 × 10Ω resistor** | the `10` taped strip — wired end to end for 20Ω |
| 1kΩ resistor | the `1K` taped strip |
| 100µF electrolytic | the taller barrel — **has polarity** |

**Why two 10Ω.** The kit's strips are 10, 100, 220, 330, 1K, 2K, 5K1, 10K, 100K, 1M — no 22Ω. Two 10Ω in series give 20Ω, which puts about 110mA through the LEDs. A single 10Ω would give ~220mA: brighter, survivable for pulsed infrared, but it runs the LEDs past their continuous rating and leans on the supply harder. A single 100Ω is the timid option — only ~22mA, fine for the camera test, too weak to cross a room.

## The transistor

The GPIO can supply about 10mA. The LEDs want 100mA. The transistor is the relay in between: a tiny current into the **base** lets a large one flow from **collector** to **emitter**.

Hold it with the **flat printed face toward you and the legs pointing down**. Left to right: **E** (emitter), **B** (base), **C** (collector).

Push the legs into rows **20, 21, 22** — E in 20, B in 21, C in 22.

> If nothing transmits later, swapping E and C is the first thing to try. Some TO-92 parts are packaged the other way round, and swapping them does no damage at this current.

## LED polarity

Each IR LED has a **long leg (anode, +)** and a **short leg (cathode, −)**. They only work one way round. Current flows in the long leg and out the short one, so the chain runs 5V → long, short → long, short → collector.

## Reference designators

Standard electronics shorthand, used throughout:

| Letter | Meaning | Here |
|---|---|---|
| **R** | Resistor | R1, R2 = 10Ω · R3 = 1kΩ |
| **C** | Capacitor | C1 = 100µF |
| **D** | Diode | D1, D2 = the clear IR LEDs (an LED is a light-emitting diode) |
| **Q** | Transistor | Q1 = S8050 / PN2222 |
| **W** | Wire | plain jumpers |

## Connections — every hole

All in columns **a–e**, rows 1–17, which are otherwise empty.

**Every component spans exactly 2 rows** — the LEDs' legs are short and won't stretch further. Only the plain wires jump long distances.

Row **3** is the transmitter's local 5V; row **1** is its local GND.

| Ref | Component | One end | Other end |
|---|---|---|---|
| **W1** | wire, male–male | **b50** | **a3** |
| **W2** | wire, male–male | **a55** | **a1** |
| **C1** | 100µF, **long leg** | **c3** | striped leg → **c1** |
| **R1** | 10Ω | **b3** | **b5** |
| **R2** | 10Ω | **c5** | **c7** |
| **D1** | IR LED, **long leg** | **b7** | short leg → **b9** |
| **D2** | IR LED, **long leg** | **c9** | short leg → **c11** |
| **W5** | wire, male–male | **d11** | **d15** |
| **Q1** | transistor | E **a13**, B **a14**, C **a15** | |
| **W3** | wire, male–male | **e13** | **e1** |
| **R3** | 1kΩ | **b14** | **b16** |
| **W4** | wire, female–male | male **c16** | female → CAM **`IO13`** |

### Row by row

| Row | Contents |
|---|---|
| **1** | `a1` W2 · `c1` C1 striped leg · `e1` W3 — **GND** |
| **3** | `a3` W1 · `b3` R1 · `c3` C1 long leg — **5V** |
| **5** | `b5` R1 · `c5` R2 |
| **7** | `c7` R2 · `b7` D1 long leg |
| **9** | `b9` D1 short leg · `c9` D2 long leg |
| **11** | `c11` D2 short leg · `d11` W5 |
| **13** | `a13` Q1 emitter · `e13` W3 |
| **14** | `a14` Q1 base · `b14` R3 |
| **15** | `a15` Q1 collector · `d15` W5 |
| **16** | `b16` R3 · `c16` W4 |

```
row 3   5V ─[R1]─ row5 ─[R2]─ row7 ─▶|─ row9 ─▶|─ row11
         │                             D1        D2      │
        C1                                              W5
         │                                               │
row 1   GND ─────── row13 emitter ─ Q1 ─ collector row15 ┘
                    row14 base ─[R3 1k]─ row16 ── IO13
```

Q1's three legs go in rows **13, 14, 15** — consecutive. Flat printed face toward you, legs down: E, B, C left to right.

W5 exists so Q1 sits clear of the LED chain rather than tangled in it. Without it the emitter would land on a row the LEDs already use.

> **If C1's legs won't span rows 3 and 1**, its pitch is 2.5mm rather than 5mm. Move W2 and W3 to row **2** and put C1 in `c3`/`c2`.

**The 100µF matters.** Each pulse draws 100mA in bursts. Without a reservoir right at the supply, those bursts sag the 5V rail and can brown out the ESP32 mid-transmission. It has polarity: the **striped side is negative**, and fitting it backwards can make it fail loudly.

## First test — no A/C required

Flash `send_test`, open Serial Monitor at 115200, press `l`.

Point your phone's **front camera** at the LEDs while you do it. You should see a brief violet-white flash. That proves the whole chain — GPIO, transistor, LEDs — without involving either appliance.

Only then aim it at the fan and the A/C.

---

## Checks before power

1. Rows 50 and 55 contain **only** what's listed — three wires in 50, four in 55.
2. Both rail strips are completely empty.
3. Nothing on `3V3` or `VCC`.
4. Wire 11 is on `IO15`, not `IO14` or `IO2`.
5. Every female connector pushed fully down; every male pin fully into its hole.

## Bring-up, in order

**Stage 1 — power.** Plug in USB-C. Point a remote at the module: its `L` LED should blink. If not, the fault is in wires 4, 6, 8 or 9.

**Stage 2 — upload.** Hold the MB's `IO0` button, replug USB-C while holding, release, then Upload. Then replug once more *without* `IO0` so the sketch actually runs — `RST` isn't wired, so nothing auto-restarts the chip.

**Stage 3 — probe.** `pin_probe` at 115200: idle `HIGH` / `0`, and hundreds of changes while a remote is pressed. `LOW` / `0` means the two resistors are swapped.

**Stage 4 — capture.** Flash `ir_dump`, validate with the kit's little remote, then record both real remotes.
