# Follow-Me (FM) — Rider's Guide

**BREmote V2.5-Evo** · tow buggy / eFoil remote

> **Status:** the Follow-Me control law is implemented and pending on-water field testing.
> This guide documents the intended behavior. It moves into the main README once FM is
> confirmed on the water.

---

## 1. What Follow-Me does

After the whip — once you've released the rope and you're riding the wave — Follow-Me
makes the **buggy trail you** at a set distance and angle, steering itself, so you can
keep your eyes on the wave instead of the remote.

It is **not** an autopilot that drives on its own. The rule never changes:

> **The buggy moves only while you hold the throttle trigger. Follow-Me only *steers* and
> only *reduces* throttle — it can never add throttle, and letting go of the trigger stops
> the buggy instantly.**

Follow-Me is autonomous *steering under your throttle hand*. You stay in charge of go/stop
at all times.

When you stop after the ride, **FM_RETURN** brings the buggy directly back toward you. It is
part of the same Follow-Me session; there is no separate Return-to-Me mode to arm.

---

## 2. Before you ride

Follow-Me needs, for the buggy to actually engage and follow:

- A **paired** TX and RX (your remote and the buggy talk to each other).
- **GPS fix on both** the remote and the buggy.
- A healthy **radio link** and **telemetry flowing**.
- A valid buggy heading: normally GPS course-over-ground while moving, with a calibrated compass
  (`?compasscal`) as the low-speed fallback. If the two disagree, the compass is excluded and FM may
  still engage on valid GPS COG; it cannot engage without either a live or briefly held GPS course.

You can *arm* Follow-Me before all of these are perfect (see §7 — Readiness), but it will
not *engage* until they are.

---

## 3. Arming — telling the buggy "I want you to follow"

Arming is a **declaration of intent.** It does not move anything. There are two ways to arm.

### 3a. The toggle (works while floating)

While floating, before takeoff:

1. **LEFT tap, then RIGHT hold** (~3 s — set by `fm_hold_duration_s`).
2. The remote shows **F1–F6** and buzzes **two quick taps** = armed.
3. Cycle the mode by repeating the gesture before you touch the throttle
   (**F1 → F2 → F3 → F4 → F5 → F6 → F0-off**).

> The toggle **cannot arm once you're on the throttle** — while you hold the trigger, the
> toggle *is* your steering. That's what the magnet is for.

Once FM is armed, you can still change its mode during the tow without disarming it:

1. Release the trigger and let the toggle return to centre briefly.
2. Hold **LEFT** to step backward or **RIGHT** to step forward through F1–F6.
3. The first change occurs after **2 seconds**; keep holding to change again every **2 seconds**.
4. Pull the trigger again to continue with the newly displayed mode.

This selector wraps F1↔F6 and never selects F0 accidentally. Explicit disarm stays on the existing
LEFT-tap → RIGHT-hold combo. Each selected mode is sent to RX immediately; RX resets the controller
edge and uses its normal engage ramp before restoring power.

### 3b. The magnet (works during the tow)

If your remote has the Hall sensor fitted and `mag_mode` is set:

- **Hold the magnet to the sensor for ~2 s, then pull it away.** You feel one short pulse
  at 2 s (get-ready), then **two quick taps** on release = armed.
- It's a **toggle**: armed already → the same gesture **disarms** (one long buzz).
- **It works with the throttle held** — so if you forgot to arm and you're already towing,
  you can arm mid-tow. This is the magnet's whole purpose.

**The arm survives** for up to `fm_arm_window_s` (default **180 s / 3 min**) even if you
don't touch the throttle — so a slow takeoff, a wait, or a short swim won't lose your arm.

---

## 4. The whip — how Follow-Me engages

Arming is not engaging. Every F1–F6 mode only **engages** when the **radial GPS distance
confirms you've actually separated** — not from side/front angles and not from a button:

1. You're armed (from §3); the trigger may be held or released.
2. You whip and separate from the buggy.
3. Once you are **beyond the engage distance for 2 continuous seconds** — proven by both
   GPS units — RX enters `FM_ACTIVE` readiness at your set offset.

You may keep the trigger held through the proof or release it while separating. The trigger is not
an input to the proof or the `FM_ARMED → FM_ACTIVE` transition. It remains the physical deadman:
the buggy begins moving and steering automatically only while you hold it, through the engage ramp,
and release still stops the motor immediately.

### Two ways to separate at the whip

- **Whip yourself to the side** (classic) — you slingshot past the buggy and ride away.
- **Steer the buggy away** — keep the trigger held and steer the buggy to its offset side so
  *it* peels off while you carry straight into the wave. Your steering during this moment
  does **not** cancel Follow-Me (you're still armed, not yet following). Once you settle and
  the separation holds for 2 s, it engages.

---

## 5. Where the buggy follows — the offset angle

Six modes set where the buggy stations, **relative to you** (imagine you facing
your direction of travel):

| Mode | Name | Buggy sits |
|---|---|---|
| **F1** | **Near-Right** | behind and to your right |
| **F2** | **Behind** (default) | straight behind you |
| **F3** | **Near-Left** | behind and to your left |
| **F4** | **Front-Left** | ahead and to your left |
| **F5** | **Front** | directly ahead as a forward pacer |
| **F6** | **Front-Right** | ahead and to your right |

F4–F6 use the same **radial** `>D_engage` 2-second proof as F1–F3. They may therefore drive from behind
toward their forward targets; there is no no-autonomous-overtake guarantee. Signed front lead and
error from the selected front axis still do not gate steering, state or the separation proof. They do
determine the speed phase: only a valid measurement showing the buggy more than one control band
behind its requested station grants uncapped catch-up. If the front position is lost, one medium warning
repeats every 3 seconds—even with the trigger released—and the speed governor falls back to its normal
rider-relative target instead of granting uncapped catch-up.

The exact diagonal angle is set by **`near_diag_offset_deg`**. F1/F3 apply it from straight behind;
F4/F6 apply it from straight ahead. F5 stays at zero offset:

- **Near-Right = 180° − offset** · **Near-Left = 180° + offset**
- **Front-Left = −offset** · **Front = 0°** · **Front-Right = +offset**
- **Bigger offset → more beside you. Smaller offset → more behind you.**

| Offset | Near-Right | Near-Left | Feel |
|---|---|---|---|
| 0° | 180° | 180° | straight behind |
| **45° (current)** | **135°** | **225°** | behind-diagonal |
| 60° | 120° | 240° | tucked behind, slightly out |
| 90° | 90° | 270° | directly beside you |

With the 10 m + 10 m defaults, the front station radius is 40 m. At 45°, F4/F6 sit approximately
**28.3 m ahead** and **28.3 m left/right**. F5 sits **40 m directly ahead**. The additional steering
lookahead is at least 40 m, giving nominal along-course steering points 68.3 m ahead for F4/F6 and
80 m ahead for F5. Front-mode use is capped below 90° so a
front station cannot be placed behind the rider.

*(0° = ahead of you, 90° = your right, 180° = straight behind, 270° = your left.)*

Follow distance is a **separate** setting — see §8.

---

## 6. Pauses vs Stops — what interrupts Follow-Me and what ends it

Not every interruption is the same. Follow-Me tells them apart:

| Situation | What it is | Follow-Me does | Re-arm needed? |
|---|---|---|---|
| **You release the trigger normally** | deadman | motor stops instantly; state/proof stay; cap is not rewritten because input throttle is already zero | no |
| **You stop beyond the engagement distance** | automatic return | after 2 s below 2 km/h, FM enters `FM_RETURN`; hold the trigger to bring the buggy directly toward you | no separate RTM arm |
| **You stop at/inside the engagement distance** | lifecycle completion | after 2 s below 2 km/h, FM passes through `FM_RETURN` and immediately completes to `FM_ARMED`; no return motion | fresh `>D_engage` proof |
| **You steer manually while following** | temporary takeover | your steering wins; FM state and throttle cap stay active; centre the input to return steering to FM | no |
| **`min_dist_m` is reached** | recoverable stop | cap 0; moving recovery above the boundary retains separation and resumes through the engage ramp; 2 s stationary uses the common RETURN cleanup | only stationary completion needs a new `>D_engage` proof |
| **F1–F3 warning geometry is invalid** | information | control continues unchanged; one medium warning every 3 s | **no** |
| **F4–F6 front position is lost** | information | control/proof continue unchanged; one medium warning every 3 s | **no** |
| **GPS heading/position or radio drops out** | a **FAULT** (something broke) | **stops** → shows `St`, throttle returns, must **re-arm** | **yes** |
| **Compass disagrees with valid GPS COG** | GPS-only degradation | compass is excluded; FM may engage/continue on live or briefly held COG | **no** |

The 3-second geometry warning continues even when the trigger is released and never changes the
control path. A real sensor/link failure is different: FM steps fully out and waits for a deliberate
re-arm, so autonomy never silently restarts after a fault.

### FM_RETURN — return after you stop

If your filtered speed remains below **2 km/h** for 2 seconds, `FM_ACTIVE` always changes to
`FM_RETURN`. Outside the effective `fm_engage_dist_m`, this starts direct retrieval. At or inside
the radius it immediately completes to `FM_ARMED` without return motion. A stationary `FM_ARMED`
declaration can start RETURN only outside the radius, including when armed before the tow.

- The buggy first remains stopped during the 2-second proof, then steers directly toward your
  current GPS position using the guarded FM heading controller.
- The trigger remains the deadman: releasing it pauses the return without cancelling it.
- Deliberate steering still has priority and temporarily suspends the automatic steering.
- Entry clears the old separation latch.
- If you move faster than 3 km/h for 1 second, return is cancelled to `FM_ARMED`.
- Arrival at `distance <= effective fm_engage_dist_m` stops first, clears the separation latch and
  enters `FM_ARMED`. The selected F1–F6 declaration remains armed, but automatic control needs a
  fresh 2-second radial proof above the engagement distance. Neither normal exit jumps directly to
  `FM_ACTIVE` or disarms to `FM_IDLE`.
- If the trigger is held when either normal exit occurs, cap 0 remains until you release it once;
  otherwise manual cap 255 is restored immediately.
- Return drive uses the same stateful PI speed governor as F1-F6. `rtm_target_speed_kmh` is its
  literal 0-50 km/h target (`0` means zero speed); a non-zero `boogie_vmax_in_followme_kmh`
  remains an absolute ceiling. It slows across `rtm_approach_zone_m`.

The stop alarm (`St` + one long buzz) fires **only when it would surprise you** — i.e. a
fault while you're holding the trigger. A stop after you've already let go just goes quiet.

---

## 7. Reading the remote

| Display | Meaning |
|---|---|
| **No FM bar** | disarmed |
| **Bar sweeping** (scanner) | **armed and ready** — waiting for your whip |
| **Bar blinking in place** | **armed but not ready yet** — still getting GPS / link. It flips to sweeping the moment it's ready. |
| **Steady distance bar** | **following you** — grows/shrinks with distance |
| **`rE` + blinking full bar** | **FM_RETURN** — returning directly toward you |
| **`Id`** | legacy RX only: old return completion entered idle and disarmed |
| **`St`** | stopped |

**Haptics (by feel, no need to look):**

| Buzz | Means |
|---|---|
| two quick taps | armed |
| **one long buzz** | stopped / disarmed |
| **one medium pulse every 3 s** | radial/front geometry warning; FM control itself is unchanged |
| one short pulse | magnet get-ready (release now for FM) |

---

## 8. Settings (RX, unless noted)

| Setting | What it does | Note |
|---|---|---|
| `followme_mode` | geometry: 1 = Near-Right, **2 = Behind**, 3 = Near-Left, 4 = Front-Left, 5 = Front, 6 = Front-Right | TX seed for the arm gesture |
| `near_diag_offset_deg` | diagonal angle for F1/F3 and F4/F6 (see §5) | **45°**; F5 ignores it |
| `min_dist_m` | ACTIVE hard-stop distance | cap 0 inside the boundary; moving radial recovery retains separation; stationary ACTIVE completion uses FM_RETURN and clears the lifecycle latches |
| `followme_smoothing_band_m` | decel band above the hard stop | rear follow distance = `min_dist_m` + this; F4–F6 station radius and added lookahead are each twice that base |
| `boogie_vmax_in_followme_kmh` | in-band F1–F6 PI speed ceiling | catch-up always opens speed cap 3; 0 removes the absolute in-band ceiling |
| `fm_arm_window_s` *(TX)* | how long an arm survives with no throttle | **180 s** |
| `mag_mode` *(TX)* | magnet gesture role: 0 off, 1 = FM | stored legacy values 2/3 are treated as FM-enabled |
| `fm_display_mode` *(TX)* | what the digit zone shows while armed | 2 = distance to buggy |
| `fm_engage_dist_m` | radial F1–F6 activation and FM_RETURN arrival radius; 0 selects automatic | stationary ACTIVE completion requires a fresh 2 s proof above it |
| `fm_diverge_dist_m` | absolute upper FM_ACTIVE divergence-test distance | default/max 100 m; values below `2 × D_engage` are raised to that minimum; legacy 0 derives the old `6 × D_engage` value under the 100 m cap; fault still needs 3 s without more than 2 m closure |
| `rtm_target_speed_kmh` | FM_RETURN PI speed target (historical key) | literal 0-50 km/h; 0 = zero speed; non-zero Boogie V-Max may clamp it |
| `rtm_approach_zone_m` | FM_RETURN slowdown-band width outside the arrival radius (historical key) | minimum effective width 2 m |

> **Tuning note:** the follow distance is currently set generous (`min_dist_m` + band
> larger than the final target). Tighten it only after the separation interlock is confirmed
> on the water — the generous setting is part of what keeps Follow-Me from engaging while
> you're still on the tow rope.

---

## 9. Disarming

- **Toggle:** repeat the arm gesture.
- **Magnet:** hold ~2 s again (one long buzz = off).
- **Automatic:** the arm expires after `fm_arm_window_s` with no throttle; a fault ends it.

Trigger release is not a disarm after FM has seen throttle. Stationary `FM_ACTIVE` completion through
`FM_RETURN` clears the separation proof; a moving min-stop recovery does not. A normal return exit
leaves FM armed and waits for a fresh proof. Before rigging a new tow, explicit
toggle/magnet/F0 disarm remains the deterministic session reset.

---

## 10. Manual steering while following

A deliberate steering deflection temporarily overrides FM steering without cancelling the mode.
FM continues applying its throttle cap and calculating its target; centring the steering input
hands steering back to FM. This is a direct takeover, not continuous target-angle repositioning.

## 11. Not yet — planned for later

- **Continuous angle repositioning** — steer to walk the buggy around your radius to any
  angle, live. Reserved as a v2 feature (`fm_steer_reposition_en`), off by default.

---

## Safety summary (never changes)

1. The buggy moves only while you hold the throttle trigger.
2. Follow-Me only steers and only reduces throttle — never adds.
3. Releasing the trigger stops the buggy at the hardware level, in every mode.
4. Manual steering has priority while deliberately deflected; genuine GPS-heading/position or radio faults still end FM. Compass disagreement alone degrades to GPS COG.
5. FM_RETURN is a guarded Follow-Me state; there is no separate RTM gesture or mode.

*See `BUGGY_FOIL_DOMAIN.md` for the domain model and `DESIGN_FOLLOW_ME.md` for the full
engineering design.*
