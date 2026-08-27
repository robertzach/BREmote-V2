# DESIGN_FOLLOW_ME — FM Autonomous Following (Priority 9)

**Project:** BREmote V2.5-Evo
**Date:** July 18, 2026
**Status:** Implemented, including `FM_RETURN`; F4 In Front remains experimental and requires controlled validation.
The former standalone Return-to-Me mode is retired. Direct return is now an FM state.

## 1. Purpose and Safety Philosophy

After the whip, the rider releases the rope and rides the wave; the buggy is unmanned on its last heading. FM makes the buggy trail the rider at a configured offset and distance, steering itself, while the rider keeps eyes on the wave.

Non-negotiable (overrides everything below):
1. The buggy ONLY moves while the rider physically holds the throttle trigger.
2. Autonomous systems ONLY steer and SUBTRACT throttle — never add.
3. Release throttle = buggy stops immediately, always.
4. Every sensor/link fault drives motor → 0. Geometry/front-position limits are warning-only.
   The independent `min_dist_m` stop latches cap 0 until the trigger is released.

FM = autonomous steering + distance-based throttle limiting, under a human throttle hand.

## 2. Canonical mode mapping (single source of truth)

| Mode | Meaning | Geometry |
|---|---|---|
| 0 | Off | — |
| 1 | **Near-Right** | behind-right diagonal at `near_diag_offset_deg` |
| 2 | **Behind** (default) | directly behind rider's course |
| 3 | **Near-Left** | behind-left diagonal |
| 4 | **In Front** | forward pacer; radial engagement is identical to F1–F3 |

This TX-side convention is canonical for ALL surfaces (TX display F0–F4, RX struct, both web UIs, README). Default = 2 (Behind).

## 3. What already exists (reuse verbatim — do not reimplement)

- **Mode selection plumbing:** TX gesture → 0xF2 meta-packet ×3 → RX `fm_mode_runtime` (RAM), with a 30 s keepalive.
- **Validation:** Phase A (RX GPS: HDOP/teleport/accel → `gps_rejected`), Phase B (TX↔RX cross-validation on 0xF3, 30 s pass / 2 s fail cadence), Phase C pattern (periodic behavioral checks).
- **Steering pipeline** (`RTMState.ino`, historical filename): EMA target-position filter (τ per preset), P+D controller `kSteerPresets[5]`, heading-source ladder (GPS COG → compass snapshot → live), confidence-scaled authority, wrap-to-±180° error handling.
- **Throttle patterns:** subtract-only cap chain (`PWM.ino:47, 52–55`), approach-zone linear decel ramp, Align-phase ~5 % cap, Run-phase speed governor, engage ramp machinery.
- **Rider position:** 0xF3 GPS meta at 2 Hz (always on with TX fix) — measured sufficient: 2.0–2.4 m rider displacement per update at typical ride speed, 4.5 m at 20 mph, vs 6 m follow gap.
- **Telemetry slots:** `fm_heading_err` (idx 14) and `fm_status` (idx 15) already reserved in `TelemetryPacket`.

## 4. State machine (RX)

```
FM_IDLE ──live declaration──→ FM_ARMED ──fresh separation proof──→ FM_ACTIVE
                                  │                                  │
                                  └──────────→ FM_RETURN ←───────────┘
                                                   │
                                      arrival or rider moving
                                                   └────────→ FM_ARMED

FM_ACTIVE / FM_RETURN ──fault──→ FM_STOPPING ──→ FM_IDLE
Any live state ──disarm / declaration expiry / config disable──→ FM_IDLE
```

- **FM_IDLE:** `fm_mode_runtime` = 0 or unset. `0xFF` means no live TX declaration and never falls back to stored RX config.
- **FM_ARMED:** mode 1–4 selected; all monitoring runs; throttle chain inactive until activation conditions met. Display/telemetry reflect armed state (`fm_status`).
- **FM_ACTIVE:** once engaged, this remains the lifecycle state through ordinary trigger release and
  geometry/front-position warnings. `fm_rx_active` says whether automatic steering is live on this
  tick. An ordinary release leaves the cap unchanged because the physical trigger already commands
  zero. Geometry never changes cap, state, steering authority or the separation proof. Reaching
  `min_dist_m` while the trigger is held is different: it latches cap 0 until release. That release
  restores manual cap 255 and clears the separation proof, so automatic control must again prove
  radial distance above `D_engage` for 2 seconds. Warnings repeat every 3 seconds even with no trigger.
- **FM_RETURN:** entered after the filtered rider speed stays below 2 km/h and radial distance stays beyond effective `D_engage` for 2 s. It is valid after normal following and directly from a stationary `FM_ARMED` declaration. Entry clears the separation latch. While the trigger is held, cap 0 keeps the buggy still during the proof; with the trigger released, zero input already does that. It then aims directly at the rider using the FM heading controller, align cap, return speed governor, approach band and convergence check. Trigger release pauses without leaving `FM_RETURN`. Arrival at `dist < D_engage`, or rider speed above 3 km/h for 1 s, exits only to `FM_ARMED`: the F1–F4 declaration remains live, but automatic Follow-Me needs a fresh 2-second radial proof above `D_engage`. There is no normal shortcut to `FM_ACTIVE` and no arrival-driven transition to `FM_IDLE`. If the trigger is held at the exit edge, cap 0 remains until one release; otherwise manual cap 255 is restored immediately.
- **FM_STOPPING:** a sensor/link/heading or divergence fault ends the run, ramps the cap back to manual and requires a fresh TX declaration.

While FM is ACTIVE, a manual steering deflection of at least 40 raw counts from centre takes steering
priority immediately at the 100 Hz PWM layer. FM remains ACTIVE, its separation proof and throttle
cap remain in force, and centring the input returns steering to FM. Divergence timing is parked while
manual steering has priority because the resulting path is rider-commanded, not FM convergence.
F4's physical front-corridor calculation remains as diagnostic feedback only. Losing it raises the
periodic warning but does not alter steering, throttle cap, lifecycle state or separation proof.

## 5. Automatic-control conditions (checked every loop)

1. Throttle held: `thr_received ≥ 25` (absolute rule; motor is already 0 without it).
2. Phase A pass (RX GPS not rejected).
3. Phase B pass (TX↔RX cross-validation current).
4. TX GPS age < `tx_gps_stale_timeout_ms`.
5. RX GPS age < 6000 ms.
6. Valid heading source (per heading ladder; `rtm_compass_required` honored).
7. LoRa healthy: `millis() − last_packet < failsafe_time`.
8. Radial separation proven: `dist > effective D_engage` continuously for 2 s. This is identical for F1–F4; angles and signed front lead are not activation gates.
9. `dist > min_dist_m`, unless the min-distance stop has already latched. Crossing `min_dist_m` latches cap 0 until trigger release.

There is no configurable rider-low-speed gate. Below 2 km/h, the fixed 2-second stationary proof
enters `FM_RETURN` only while outside `D_engage`. Low speed inside `D_engage` does not pause or
complete normal FM; the buggy keeps following until `min_dist_m` is reached or the trigger is released.

Failure of 1 keeps `FM_ACTIVE`; no cap write is needed because input throttle is already zero.
Failures 2–7 are genuine faults and end the FM run through STOPPING. Failure of 8 means ARMED/manual
or an ACTIVE session waiting for a new proof. Geometry/front checks are not conditions: they only
publish the 3-second warning. Condition 9 is the explicit cap-0 stop latch described above.

## 6. Target-point computation (the new control code)

Per control tick (10 Hz), all on RX:

1. **Filter:** EMA the raw 0xF3 rider position with the active preset's τ.
2. **Rider course + speed:** derived from successive filtered positions. Valid only while rider speed ≥ ~5 km/h (measured: course noise doubles below ~3 mph). Invalid course → degraded mode: offset direction = bearing(rider → buggy) ("hold station at distance"), no diagonal.
3. **Lag anchor:** a first-order EMA trails a moving target by v·τ (13–18 m at 15–20 mph with τ = 2 s — larger than the follow gap and speed-dependent). Correct it: `anchor = filtered_pos + û_course · min(v_rider · τ_active, 2 · d_follow)`. This makes geometry speed-independent without touching the proven smoothing (deliberately NOT solved by shrinking τ: the filter must keep ignoring carves — measured p95 turn rate 49°/s at ~5 m radius; the buggy follows the low-passed path, never mirrors bottom turns).
4. **Trailing point:** `target = anchor − d_follow · R(offset) · û_course`, where `d_follow = min_dist_m + followme_smoothing_band_m` and offset = 0° (mode 2), −`near_diag_offset_deg` (mode 1, rider's right), +`near_diag_offset_deg` (mode 3). Sign convention MUST be fixed against the code's bearing convention and verified in `Tools/FollowMe Settings Visualizer.html` before water.
5. **Steer:** feed `target` to the existing steering pipeline unchanged. Publish `fm_heading_err` and `fm_status` each rotation.
6. **Side-zone hysteresis:** when the angle between rider course and rider→buggy bearing crosses `zone_angle_enter_deg`/`zone_angle_exit_deg`, blend diagonal ↔ pure-behind (Schmitt pair) so an unstable rider course cannot whip the target point across the wake.

Mode 4 replaces the trailing point with a forward station at `d_follow`. Its steering point is a
derived lookahead that is kept ahead of the buggy, so excess lead is corrected by slowing down rather
than by a U-turn toward the rider. Engagement uses the same **radial** `dist > D_engage` 2-second proof
as F1–F3. Consequently F4 may drive from behind toward its forward target; there is no longer a
no-autonomous-overtake gate. Loss of rider course, signed lead or front cone raises a warning only.
With no valid rider course, F4 temporarily aims straight ahead on the buggy's trusted heading while
the warning remains asserted.

## 7. Throttle cap chain (subtract-only; lowest cap wins)

| # | Cap | Source pattern |
|---|---|---|
| 1 | Hard stop: `dist <= min_dist_m` → latch cap 0 until trigger release; release restores manual cap 255 and clears the separation proof | FM stop latch |
| 2 | Approach ramp: F1–F3 linear 255→0 across the smoothing band; F4 omits it because slowing when caught collapses the front gap | FM approach ramp |
| 3 | Speed governor: F1–F3 target rider speed + margin; F4 varies around rider speed from along-track error; non-zero `boogie_vmax_in_followme_kmh` is the final absolute ceiling, while 0 means no absolute ceiling | Run-phase governor |
| 4 | Align phase: heading error > threshold → ~5 % cap | Align-phase pattern |
| 5 | Engage ramp: 0→cap over 3–4 s on every FM_ACTIVE entry | FM engage ramp |

FM writes caps only. The human trigger remains the sole throttle source; trigger release stops the
buggy through the unchanged base architecture without disarming FM. After FM has seen its first
throttle input, the TX keeps its mode declaration alive until explicit F0/gesture disarm, completed
FM Return, an RX-reported fault or declaration loss. Ordinary trigger release preserves a valid
separation proof. A release after the `min_dist_m` stop specifically clears it, so the next automatic
run must again prove `dist > D_engage`. Explicit disarm remains the deterministic session boundary.

## 8. Parameters

F4 reuses the existing FM parameters (no SW_VERSION bump):

`fm_diverge_dist_m` also requires no version bump: it renames the banked final float slot in place,
so `confStruct` remains 192 bytes. Explicit values are absolute metres. The effective value is raised
to `2 × D_engage` when necessary and capped at 100 m. Existing SW35 configurations contain `0`; this
compatibility value reconstructs the old `6 × D_engage` limit and then applies the 100 m cap.

| Param | v1 role | Owner default |
|---|---|---|
| `followme_mode` | TX starting mode / RX stored preference; live arming still requires 0xF2 | 2 (Behind) |
| `min_dist_m` | hard-stop distance | 10 m |
| `followme_smoothing_band_m` | hysteresis + ramp band (station = sum) | 10 m |
| `near_diag_offset_deg` | diagonal offset (modes 1/3) | 45° |
| `boogie_vmax_in_followme_kmh` | FM absolute speed ceiling; 0 = no absolute ceiling (F4 gap governor remains active) | 25 km/h (~15.5 mph) |
| `zone_angle_enter_deg` / `zone_angle_exit_deg` | F1/F3 side-target Schmitt; F4 warning Schmitt only | 35° / 45° |
| `fm_engage_dist_m` | one radial F1–F4 activation and FM_RETURN arrival radius; 0 = auto, otherwise 8–50 m | 0 (auto) |
| `fm_diverge_dist_m` | absolute FM_ACTIVE sustained non-closing ceiling; effective minimum `2 × D_engage`, maximum 100 m; 0 = legacy auto | 100 m |
| `rtm_target_speed_kmh` | historical key: FM Return GPS speed target; 0 uses 5 km/h, hard-limited to 8 km/h | 4 km/h |
| `rtm_align_threshold_deg` | historical key: FM Return align threshold | 45° |
| `rtm_approach_zone_m` | historical key: FM Return slowdown-band width outside `D_engage`; minimum effective width 2 m | 12 m |

Field-retunable to 4/10/20 m equivalents without reflash. At the next SW_VERSION bump (whenever one happens for other reasons), bake the proven values into `defaultConf` on both sides — a version bump resets stored config to `defaultConf` (verified against `ConfigService`/`SPIFFSEngine`; web-UI-only tuning does not survive bumps).

FM_RETURN has a fixed 60-second continuous-return safety timeout. A configurable RX-side
autonomous-runtime cap remains deferred to a future config-version bump; retired TX RTM fields are
not used at runtime.

## 9. Failure modes and recovery

| Failure | Detection | Response |
|---|---|---|
| TX (rider) GPS loss | age > `tx_gps_stale_timeout_ms` | cap 0, STOPPING → IDLE, re-arm required |
| RX GPS loss | age > 6 s | same |
| Heading source invalid | ladder empty | same |
| LoRa loss | > `failsafe_time` | PWM pulses stop immediately; FM fault/disarm when control resumes |
| Sustained divergence | distance > effective `fm_diverge_dist_m` for 3 s without closing by more than 2 m (after engage grace) | cap 0, STOPPING → IDLE, re-arm required |
| Rider reaches stop radius | dist ≤ `min_dist_m` while ACTIVE and trigger held | latch cap 0; distance recovery alone does nothing; trigger release clears stop + separation proof and exposes manual cap 255 |
| Rider stationary | filtered speed < 2 km/h for 2 s | outside `D_engage`: `FM_RETURN`; inside it: no special transition |
| Rider course invalid | speed < ~5 km/h | F1–F3 use radial hold-station target; F4 goes straight on buggy heading and raises warning |
| F1–F3 warning geometry invalid | radial warning Schmitt | stay/control normally; 300 ms warning every 3 s; no cap/state/steering/latch change |
| F4 no longer ahead/in front cone | signed lead / angle warning Schmitt | stay/control normally; 300 ms warning every 3 s; no cap/state/steering/latch change |
| Trigger released | physical | motor stops; ordinary release leaves current cap/state/proof untouched; warning continues; a latched min-distance stop is released and its separation proof cleared |
| Rider stationary beyond effective `D_engage` | radial distance > `D_engage` and filtered speed < 2 km/h for 2 s | enter `FM_RETURN`; drive remains trigger-gated |
| FM Return arrival | radial distance < `D_engage` | stop first, clear latch, preserve declaration and enter `FM_ARMED`; held trigger stays cap 0 until release, then manual cap 255; fresh `>D_engage` proof required |
| Rider moves during FM Return | filtered speed > 3 km/h for 1 s | same normal exit to `FM_ARMED`; never directly to `FM_ACTIVE` |
| Manual steering deflected ≥40 counts | raw RX steering input | rider steering wins; FM state/latch/cap preserved; centre to return |

**Field-validation prerequisite:** all FM states use the same guarded heading ladder. Verify GPS COG, compass orientation and steering direction on the actual hardware, wheels up and then at controlled low speed, before any open-water use.

## 10. Test plan

1. **Bench, motor off:** mode plumbing end-to-end; simulated coordinates through target-point math; sign/offset verification; all F1–F4 modes engage only after radial `>D_engage` for 2 s; geometry/front warning edges have no control effect; min-distance cap 0 persists across distance recovery and clears only on trigger release; every fault gate force-failed → cap 0.
2. **Bench, wheels up:** cap chain order; engage ramp; align-phase cap; compass-under-load table (from Phase 0.5).
3. **Controlled water, tethered:** mode 2 (Behind) only, walking-pace rider, hard-stop verification, GPS-denial stop.
4. **Field, incremental:** mode 2 first at 6 m; then mode 1 (Near-Right); throttle hand ready to cut throughout; logging enabled (aux button) every run.

## 11. Implementation order

1. Phase 0.5 heading-feedback triage (blocking).
2. RX: FM state machine + activation conditions (§4–5).
3. RX: target-point computation (§6) feeding the existing steering pipeline.
4. RX: throttle cap chain (§7) from existing patterns.
5. Both web UIs + RX comments: canonical mapping + param descriptions (§2, §8); README table.
6. Telemetry: populate idx 14/15; TX display FM-active state.
7. Safety audit (motor-safety verdict first), then §10 gates in order.

F4 only extends TX mode selection and the existing 0xF2 value range. No new packet, config field or rate change (2 Hz 0xF3 remains unchanged).
