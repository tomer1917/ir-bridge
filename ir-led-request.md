# Message: IR LED request

> Copy-paste ready. See PLAN.md §2 for why this part is the only purchase left.

---

Hey — do you have a spare IR LED? Details below so you can check what you've got.

**WHAT I NEED**
An infrared emitter LED: 5mm, 940nm, clear lens. Two of them if possible (I need one aimed at the A/C and one at the ceiling fan), but one is enough to get started. It's the part that goes in a TV remote.

**HOW TO RECOGNIZE IT**
- Looks like an ordinary 5mm LED, but the lens is WATER-CLEAR or very faintly blue/purple tinted. No colour at all when it's off.
- Two legs, longer leg = anode (+).
- It must be an EMITTER, not a receiver. The receiver is a black three-legged blob with a little dome on the front — I already have two of those, they're no use to me.

**PART NUMBERS** (if yours are labelled or in a bag with a code)
Ideal: `TSAL6200`, `TSAL6400`, `IR333`, `IR333C`, `LD274`, `SFH4545`
All of those are 940nm high-output emitters. Any of them is perfect.

**THE EASIEST SOURCE: A DEAD REMOTE**
If you have any broken or unused remote lying around — TV, A/C, stereo, set-top box, old DVD player — the LED at the very tip of it is exactly the part I need. Desoldering it takes two minutes and it's guaranteed to be the right thing. Honestly this is the best option; I'd rather gut a dead remote than buy a pack of ten.

**OTHER PLACES IT HIDES**
- IR obstacle-avoidance / line-follower sensor modules from Arduino kits: they carry a matched pair, and the CLEAR one is the emitter I want.
- An IR blaster cable from a cable box or AV receiver.
- Any "IR transmitter + receiver pair" bag from a component kit.

**ACCEPTABLE ALTERNATIVES**
- **850nm** instead of 940nm: works fine, slightly less efficient with my receiver. Bonus: 850nm glows a faint visible red, which actually makes debugging easier. Happy to take it.
- **3mm** instead of 5mm: fine, a bit less output.
- **A pre-built "IR transmitter module"** (small PCB with the LED already on it): usable, but it has a built-in resistor that limits it to about 1 metre of range. Only as a last resort — I want to drive a bare LED hard through a transistor for proper room coverage.

**WHAT WON'T WORK**
- IR receiver modules (the black 3-pin blobs) — already have them.
- IR photodiodes or phototransistors. These also look like clear 2-leg LEDs, but they RECEIVE light instead of emitting it. The test below tells them apart.
- Ordinary visible LEDs, however clear they look.

**30-SECOND TEST TO CONFIRM** (worth doing before you dig it out)
1. Connect the LED across a CR2032 coin cell — long leg to +. The cell's own internal resistance keeps the current safe, so no resistor needed.
2. Point your phone's FRONT (selfie) camera at it in a dimmish room.
3. If it glows bright violet-white ON SCREEN but is invisible to your eyes, it's an IR emitter. That's the one.
4. Nothing on camera = it's a photodiode/receiver, or it's backwards — try flipping it before giving up.

Front camera matters: rear cameras have an IR-cut filter and will show nothing even for a perfectly good IR LED.

If you've got several and aren't sure which is which, just bring them all and I'll test them. Thanks!

---

## Notes for me (not part of the message)

**Why 940nm specifically.** The VS1838B receivers in both kits peak at 940nm, and so does every consumer remote — matching wavelengths means maximum range for a given drive current. 850nm is down maybe 20–30% in sensitivity at the receiver, which costs range but nothing else.

**The spec that identifies it with a multimeter.** IR LEDs have a forward voltage of ~1.2–1.4V, far below any visible LED (red ~1.8V, blue/white ~3.0V+). In diode-test mode a 940nm emitter reads roughly 1.0–1.3V; anything above ~1.6V is a visible LED. A photodiode reads open in both directions. This is faster than the camera test if a meter is handy.

**Why bare LED over a module.** Pre-made transmitter modules put a ~100R resistor in series off 3.3V, giving ~15mA. The plan drives ~100mA pulsed through a transistor (PLAN.md §3) — roughly 7x the output, which is the difference between "must point it at the A/C" and "bounce it off the ceiling and forget about aiming".

**If the friend delivers only one LED.** Still fine to start. Two in series only matters for covering the A/C and the ceiling fan simultaneously; Phase 1 and Phase 2 bench testing need just one.
