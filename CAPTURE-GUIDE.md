# Phase 1, step by step — capturing the remote codes

Assumes no electronics background. Goal: read the invisible codes both remotes send, so the ESP32 can imitate them later.

Nothing in this guide can damage anything as long as you build the voltage divider in B3 rather than wiring the module's output straight to the ESP32.

---

## Part A — Find the parts

### A1. The IR receiver module

**In the kit inventory sticker (photo 1):** bottom-left grid, **second row, second picture from the right** — between "DHT11 Temperature and Humidity Module" and "Joystick Module". It's labelled **"IR Receiver Module"**.

**What it looks like in real life:** a small circuit board, roughly fingernail-sized, carrying one black component that looks like **a tiny black eye** — a squarish blob with a rounded, slightly shiny front face. Three metal pins stick out of the board.

That "eye" is the part that sees infrared. You have one in each kit, so if you can't find the first, check the other box.

> **Careful not to confuse it with:** the RFID-RC522 (the big blue board with the spiral antenna printed on it), or the DHT11 (small blue box with a grid of holes). The IR receiver is the only one with a shiny black dome.

**If yours has no circuit board** — just a black 3-legged component on its own — that's fine, it's the same part. Skip to A1b below.

### A1b. Which pin is which

**CONFIRMED — this module is labelled `G` `R` `Y`.** Those letters are the *wire colours* the factory expects, not function names:

| Pin | Stands for | Function |
|---|---|---|
| **G** | Ground | ground |
| **R** | Red | **power** |
| **Y** | Yellow | signal |

Cross-check: `R` is the middle pin, and on essentially every 3-pin sensor module the middle pin is power. The two conventions agree.

Those three words — **ground, power, signal** — are all you need. Ground and power feed it electricity; signal carries what it sees back to the ESP32.

### A2. The IR LEDs (photo 4)

The **right-hand bag, marked "IR TX"** — the water-clear ones. Those are the transmitters. You don't need them yet; they're for Phase 2.

The **left bag** (dark, smoky-looking) — leave those alone entirely. They're a different part that won't work for this.

### A3. Jumper wires

**In the kit inventory sticker (photo 1):** fourth row, **second picture from the right** — "Female-to-Male Dupont Wire 20PCS". These are the ones with a **plastic socket (a hole) on one end and a metal pin on the other.**

You need about **nine** in total. Colour doesn't matter electrically, but keep a convention so you don't lose track: red = power, black = ground, yellow = signal.

You'll also need **two resistors** (1kΩ and 2kΩ) and a couple of the male-to-male "Breadboard Jumper Wire" for the rail connections.

### A4. The breadboard

**Photo 1, fourth row, fourth picture** — "830 Tie-Points Breadboard". The white plastic slab covered in little holes.

**How a breadboard works, in one sentence:** the holes are secretly wired together in **rows of five, running across the short direction**, so anything plugged into the same row of five is electrically joined.

That's the whole trick — it lets two metal pins touch each other without soldering.

### A5. The ESP32-CAM and the USB adapter

The small board with the camera lens on it, and the USB-to-serial adapter you used for the gesture project. Same two items, same cable.

---

## Part B — Wire it up

The receiver runs from 5V and reports back to the ESP32 through a two-resistor divider. No soldering.

### B1. Find the pin names on the ESP32-CAM

The pin names are printed **on the top face**, in tiny white text down both edges:

- **Left edge**, from the camera end: `5V` `GND` `IO12` `IO13` `IO15` `IO14` `IO2` `IO4`
- **Right edge**, from the camera end: `3V3` `IO16` `IO0` `GND` `VCC` `U0R` `U0T` `GND`

Don't count positions — read the labels. You need three: **`5V`**, **`GND`**, **`IO14`**.

> `GND` appears more than once. They're all the same connection internally, so any free one works. `3V3` is *not* used in this build — the module needs 5V (see B3).

### B2. Plug the receiver into the breadboard

Your breadboard has coordinates printed on it: columns `a`–`e` and `f`–`j`, rows numbered 1–60. The rule to remember: **the five holes of one numbered row (a,b,c,d,e) are joined together.** So `a30` and `b30` are the same electrical point; `a30` and `a31` are not.

Push the module's three pins into column `a`, three consecutive rows:

- `G` → **a30**
- `R` → **a31**
- `Y` → **a32**

(Rows 30–32 are arbitrary; any three consecutive numbers work. Use these so the table below matches.)

The long red and blue strips down both edges are the **power rails** — every hole in a strip is joined to every other one along it. B3 uses them to share 5V and ground between the CAM and the module.

### B3. Run the three wires

Take three female-to-male jumpers. The **socket end goes onto the ESP32-CAM's pin**; the **metal pin end goes into the breadboard**.

The module needs 5V, and the CAM's single `5V` pin is taken by the MB wire — so share power through the breadboard's **power rails** (the long strips marked red `+` and blue `−`; every hole in a strip is connected to every other). Use the pair nearest the module and stay within one half of the board, since some breadboards split the rails in the middle.

Feed the rails from the MB, then feed the CAM from the rails:

| From | To |
|---|---|
| MB `5V` socket | red `+` rail |
| MB `GND` socket | blue `−` rail |
| red `+` rail | CAM `5V` |
| blue `−` rail | CAM `GND` |

`U0T`, `U0R` and `IO0` stay wired MB-to-CAM directly.

Then the module and divider:

| What | Where |
|---|---|
| Module `G` (row 30) | wire to blue `−` rail |
| Module `R` (row 31) | wire to red `+` rail |
| Module `Y` (row 32) | one leg of the **1kΩ** |
| 1kΩ other leg | **row 36** |
| **2kΩ** | one leg in **row 36**, other in **row 40** |
| Row 40 | wire to blue `−` rail |
| **Row 36** | yellow wire to CAM `IO14` |

Row 36 is the tap — the 3.3V copy of the signal. Rows 36 and 40 are arbitrary; any free rows work provided the resistors bridge them as shown. There is no `3V3` wire any more.

### ⚠ This module needs 5V — and therefore a voltage divider

**Confirmed by testing:** at 3.3V this module's receiver chip does not run at all. Its onboard `R1` pull-up still holds the output high, so the pin *looks* alive while being completely deaf. At 5V it works, and the onboard `L` LED blinks on every button press.

But a 5V output cannot go straight into an ESP32 pin — they are not 5V-tolerant. Two resistors fix it:

| Resistor | 4-band colours | Position |
|---|---|---|
| **1kΩ** | brown, black, red, *gold* | between the module's `Y` and the tap point |
| **2kΩ** | red, black, red, *gold* | between the tap point and ground |

The tap sits at 5V × 2/(1+2) = **3.3V**. When the module's output falls to 0V the tap follows. Same signal, safe height.

No 2kΩ in the kit? Two 1kΩ end to end make 2kΩ. (5-band kits: 1kΩ = brown-black-black-brown-brown, 2kΩ = red-black-black-brown-brown.)

### B4. Connect the ESP32-CAM-MB — wired, not stacked

The **ESP32-CAM-MB** is the black board with the USB-C socket. It's the USB-serial chip, the 5V supply and the `IO0`/`RST` buttons in one.

**Don't stack the CAM onto it.** Stacking fills all 16 of the CAM's pins, leaving nothing for the IR receiver. Wire the two boards together instead — the MB works exactly the same way, and every other CAM pin stays free.

**Learn the MB's socket layout first:** plug the CAM in the normal way once, note which way the camera points relative to the USB-C connector, then unplug. Each socket belongs to whichever pin was sitting in it. Write `5V`, `GND`, `U0T`, `U0R`, `IO0` on the MB's plastic with a pen.

Then five female-to-male jumpers — female end on the CAM's pins, male end into the MB's sockets:

| MB socket | → CAM pin |
|---|---|
| `5V` | `5V` |
| `GND` | `GND` |
| `U0T` | `U0T` |
| `U0R` | `U0R` |
| `IO0` | `IO0` |

Like to like — nothing crosses over. (TX/RX crossing applies to a generic FTDI adapter; the MB handles that internally.)

The CAM has two `GND` pins. This uses one; the IR receiver in B3 uses the other.

**To put the board in flashing mode:** hold down the MB's **`IO0` button**, and while holding it, unplug and replug the USB-C cable. Release the button. That's what the `IO0` wire is for.

---

## Part C — Software

1. **Open the Arduino IDE.**
2. **Tools → Manage Libraries…**, search **`IRremoteESP8266`**, install the one by **David Conran**. (The name says ESP8266; it works on ESP32 too.)
3. **Open `ir_dump.ino`.**
4. **Tools menu**, set:
   - Board → `AI Thinker ESP32-CAM`
   - Partition Scheme → `Huge APP (3MB No OTA/1MB SPIFFS)`
   - Port → whichever COM port appears when you plug the adapter in
5. **Enter flashing mode:** hold the MB's `IO0` button, unplug and replug USB-C while holding, release.
6. **Press Upload** (the right-arrow button). Wait for "Done uploading".
7. **Unplug and replug USB-C again**, this time *without* touching `IO0`. The board now runs the code instead of waiting for more.

   > **Don't skip this.** Because `RST` isn't wired between the MB and the CAM, the IDE's automatic `Hard resetting via RTS pin...` has no wire to travel down. After a successful upload the chip is still sitting in download mode, and the Serial Monitor shows only a short burst of garbage — that's the ROM bootloader, not your sketch. A manual power-cycle (or the `RST` button on the CAM's underside) starts the program.
   >
   > If garbage then repeats endlessly, unplug the `IO0` wire and power-cycle again — the MB's auto-programming circuit can hold the chip in download mode. Refit that wire next time you upload.
8. **Tools → Serial Monitor.** Set the dropdown at its bottom-right to **115200**.

You should see:

```
IR dump ready. Listening on GPIO 14.
Press one remote button at a time.
```

Garbled symbols instead → the baud rate is wrong. Blank panel → the banner prints only once at startup and you may have missed it; replug USB-C with the monitor already open.

Upload itself failing with `Failed to connect to ESP32: Timed out waiting for packet header` → the chip isn't listening. Redo step 5, and check the `IO0` wire between the MB and the CAM.

---

## Part D — Capture

### D1. Prove the rig works first

Take the **little black remote that came with the kit** (photo 1, third row, first picture — "Remote Control"; it's also visible in photo 3's lower tray). Point it at the black eye from about 10cm and press any button.

Text should appear in the Serial Monitor. **Don't continue until it does** — this step exists so that silence later means "unusual remote", not "wire in the wrong hole".

Nothing appearing? In order of likelihood: the yellow wire isn't in the same breadboard row as the signal pin; the remote's battery is flat; power and ground are swapped.

### D2. Capture the real remotes

For **each** button below: point the remote at the eye from ~10cm, press once, wait two seconds, and note in a text file which button you pressed and what appeared.

**Electra A/C remote (the left one in photo 2)** — press each of these, and note the **temperature and mode showing on its screen** at the moment you press:
- power on
- power off
- `+` a few times (note each temperature)
- `−` a few times
- the fan-speed button

> A/C remotes are different from normal remotes: each press transmits the **entire state** of the unit — temperature, mode and fan speed all at once — not just "the button you pressed". That's why the screen reading matters more than the button name.

**Hyundai fan/light remote (the right one in photo 2)** — all eight buttons:
`ON/OFF`, `1H`, `3H`, `6H`, `HI`, `MED`, `LOW`, `OFF`

### D3. Send it back

Copy everything from the Serial Monitor and paste it back to me, with your button labels. The line that decides the whole next phase is `protocol :` on the Electra — if it says `ELECTRA_AC`, you get a proper thermostat instead of canned recordings.

---

## Optional side quest — confirm the IR LEDs

Not needed for capture. Do it while waiting if you like.

Take one **clear** LED from the "IR TX" bag. Press its two legs against the two flat faces of a **CR2032 coin cell** (the flat silver battery in car keys and bathroom scales) — **longer leg on the `+` side**, the one with writing on it. No resistor needed; the little battery can't push hard enough to hurt it.

Now look at the LED **through your phone's front/selfie camera** in a dimmish room.

- Glowing bright violet-white on screen, invisible to your eyes → **confirmed emitter.**
- Nothing → flip the LED around and retry. Still nothing → it's the wrong type, try a different one.

Your phone's **rear** camera has a filter that blocks infrared, so it will show nothing even for a perfect LED. Front camera only.
