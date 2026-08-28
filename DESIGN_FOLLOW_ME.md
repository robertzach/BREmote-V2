# DESIGN_FOLLOW_ME — FM Autonomous Following (Priority 9)

**Project:** BREmote V2.5-Evo
**Date:** July 18, 2026
**Status:** Implemented, including `FM_RETURN`; F4–F6 front modes remain experimental and require controlled validation.
The former standalone Return-to-Me mode is retired. Direct return is now an FM state.

## 1. Purpose and Safety Philosophy

After the whip, the rider releases the rope and rides the wave; the buggy is unmanned on its last heading. FM makes the buggy trail the rider at a configured offset and distance, steering itself, while the rider keeps eyes on the wave.

Non-negotiable (overrides everything below):
1. The buggy ONLY moves while the rider physically holds the throttle trigger.
2. Autonomous systems ONLY steer and SUBTRACT throttle — never add.
3. Release throttle = buggy stops immediately, always.
4. Every sensor/link fault drives motor → 0. Geometry/front-position warning thresholds do not gate
   autonomy. The valid signed front gap is nevertheless a speed-governor input and is required to
   grant F4–F6 uncapped catch-up. The independent `min_dist_m` stop forces cap 0 while inside the
   boundary and can recover without discarding a valid separation proof.

FM = autonomous steering + distance-based throttle limiting, under a human throttle hand.

## 2. Canonical mode mapping (single source of truth)

| Mode | Meaning | Geometry |
|---|---|---|
| 0 | Off | — |
| 1 | **Near-Right** | behind-right diagonal at `near_diag_offset_deg` |
| 2 | **Behind** (default) | directly behind rider's course |
| 3 | **Near-Left** | behind-left diagonal |
| 4 | **Front-Left** | forward-left pacer using `near_diag_offset_deg` |
| 5 | **Front** | forward pacer directly on rider course |
| 6 | **Front-Right** | forward-right pacer using `near_diag_offset_deg` |

This TX-side convention is canonical for ALL surfaces (TX display F0–F6, RX struct, both web UIs, README). Default = 2 (Behind).

While FM is armed, the rider may release the trigger and hold LEFT/RIGHT to step backward/forward
through F1–F6. The first step occurs after 1 second and repeats every 1 second while held. This
armed selector excludes F0; explicit disarm remains on the combo gesture. Each step is transmitted
immediately. A live RX mode edge neutralizes the old command, resets controller continuity and
restarts the engage ramp; the radial separation proof remains valid because it is mode-independent.

## 3. What already exists (reuse verbatim — do not reimplement)

- **Mode selection plumbing:** TX gesture → 0xF2 meta-packet ×3 → RX `fm_mode_runtime` (RAM), with a 30 s keepalive.
- **Validation:** Phase A (RX GPS: HDOP/teleport/accel → `gps_rejected`), Phase B (TX↔RX cross-validation on 0xF3, 30 s pass / 2 s fail cadence), Phase C pattern (periodic behavioral checks).
- **Steering pipeline** (`RTMState.ino`, historical filename): EMA target-position filter (τ per preset), P+D controller `kSteerPresets[5]`, heading-source ladder (GPS COG → compass snapshot → live), confidence-scaled authority, wrap-to-±180° error handling.
- **Throttle patterns:** subtract-only cap chain (`PWM.ino:47, 52–55`), approach-zone linear decel ramp, Align-phase 60/255 (~24 %) cap, Run-phase speed governor, engage ramp machinery.
- **Rider position:** 0xF3 GPS meta at 2 Hz (always on with TX fix) — measured sufficient: 2.0–2.4 m rider displacement per update at typical ride speed, 4.5 m at 20 mph, vs 6 m follow gap.
- **Telemetry slots:** `fm_heading_err` (idx 14) and `fm_status` (idx 15) already reserved in `TelemetryPacket`.

## 4. State machine (RX)

```
FM_IDLE ──live declaration──→ FM_ARMED ──fresh separation proof──→ FM_ACTIVE
                                  │                                  │
              stationary >D_engage                                  │ stationary 2 s
                                  └──────────→ FM_RETURN ←───────────┘
                                                   │
                                  arrival <=D_engage or rider moving
                                                   └────────→ FM_ARMED

FM_ACTIVE / FM_RETURN ──temporary GPS/link/heading availability fault──→ FM_STOPPING ──→ FM_ARMED
FM_ACTIVE / FM_RETURN ──divergence/heading contradiction/convergence fault──→ FM_STOPPING ──→ FM_IDLE
Any live state ──disarm / declaration expiry / config disable──→ FM_IDLE
```

- **FM_IDLE:** `fm_mode_runtime` = 0 or unset. `0xFF` means no live TX declaration and never falls back to stored RX config.
- **FM_ARMED:** mode 1–6 selected; all monitoring runs; the trigger-independent radial separation proof has not completed. The throttle chain is inactive and display/telemetry reflect armed state (`fm_status`).
- **FM_ACTIVE:** the separation proof has completed, even if the trigger was open. This remains the
  lifecycle state through ordinary trigger release and geometry/front-position warnings.
  `fm_rx_active` says separately whether a held trigger has granted automatic motor/steering
  authority on this tick. An ordinary release leaves the cap unchanged because the physical trigger commands
  zero. Warning-threshold crossings never change state, steering authority or the separation proof.
  The radial/signed distance measurements still select catch-up versus in-band speed regulation;
  invalid F4–F6 signed geometry cannot grant uncapped catch-up. Reaching `min_dist_m` while the trigger
  is held is different: it pauses automatic authority at cap 0. Trustworthy distance recovery above
  `min_dist_m` clears only that stop, retains the separation proof and resumes through the normal
  engage ramp. If the rider remains below 2 km/h for 2 seconds at any trustworthy distance,
  `FM_ACTIVE` enters `FM_RETURN`. RX geometry warnings repeat every 3 seconds even with no trigger;
  the TX `fm_warn_distance_m` separation warning repeats every 2 seconds at/above its threshold.
- **FM_RETURN:** entered from `FM_ACTIVE` after the filtered rider speed stays below 2 km/h for 2 s, regardless of distance. A stationary `FM_ARMED` declaration can enter only while radially beyond effective `D_engage`. Entry clears the separation and min-distance latches. During the proof, a held trigger is capped at zero; with the trigger released, zero input already stops the buggy. Outside `D_engage`, RETURN then aims directly at the rider using the FM heading controller, align cap, return speed governor, approach band and convergence check. At or inside `D_engage`, arrival completes immediately without return motion. Trigger release pauses a running retrieval without leaving `FM_RETURN`. Arrival at `dist <= D_engage`, or rider speed above 3 km/h for 1 s, exits only to `FM_ARMED`: the F1–F6 declaration remains live, but automatic Follow-Me needs a fresh 2-second radial proof above `D_engage`. There is no normal shortcut to `FM_ACTIVE` and no arrival-driven transition to `FM_IDLE`. If the trigger is held at the exit edge, cap 0 remains until one release; otherwise manual cap 255 is restored immediately.
- **FM_STOPPING:** every active sensor/link/heading or divergence fault stops automatic authority at cap 0 and ramps the cap back to manual over 2 seconds. Temporary GPS rejection/Phase-B/staleness, ordinary heading unavailability and LoRa loss preserve the live F1–F6 declaration, clear all automatic proof and finish in `FM_ARMED`; if the trigger remained held, one release is required before a fresh trigger-independent `>D_engage` proof can begin. Sustained divergence, RETURN runtime/not-closing and a proven compass-vs-COG contradiction finish in `FM_IDLE` and require a deliberate new arm declaration.

While FM is ACTIVE, a manual steering deflection of at least 40 raw counts from centre takes steering
priority immediately at the 100 Hz PWM layer. FM remains ACTIVE, its separation proof and throttle
cap remain in force, and centring the input returns steering to FM. Divergence timing is parked while
manual steering has priority because the resulting path is rider-commanded, not FM convergence.
The F4–F6 selected-axis angle thresholds remain diagnostic feedback only. Losing valid signed front
geometry raises the periodic warning and falls the speed governor back from uncapped catch-up to its
normal rider-relative target; it does not alter steering, lifecycle state or separation proof.

## 5. Lifecycle and automatic-control conditions (checked every loop)

1. Throttle held: `thr_received ≥ 25` (final physical deadman for actual automatic authority only;
   it is not part of the separation proof or `FM_ARMED → FM_ACTIVE`).
2. Phase A pass (RX GPS not rejected).
3. Phase B pass (TX↔RX cross-validation current).
4. TX GPS age < `tx_gps_stale_timeout_ms`.
5. RX GPS age < 6000 ms.
6. Valid heading source from the guarded ladder. Ordinary temporary loss is recoverable. A proven
   compass-vs-COG disagreement is a separate terminal trust fault for FM even if live GPS COG or the
   short held-COG bridge remains available; RTM may still use its bounded degraded behaviour.
7. LoRa healthy: `millis() − last_packet < failsafe_time`.
8. Radial separation proven: `dist > effective D_engage` continuously for 2 s. This proof is
   trigger-independent and identical for F1–F6; angles and signed front lead are not activation gates.
9. `dist > min_dist_m`, unless the recoverable min-distance stop is already active. Crossing
   `min_dist_m` forces cap 0; radial recovery permits a ramped resume with the separation proof intact.

There is no configurable rider-low-speed gate. Below 2 km/h, the fixed 2-second stationary proof
always moves `FM_ACTIVE` through `FM_RETURN`. Outside `D_engage` it performs retrieval; at or inside
the radius it immediately completes to `FM_ARMED`. A stationary `FM_ARMED` declaration still needs
`dist > D_engage`, so near-range ARMED cannot loop through RETURN.

Failure of 1 neither blocks entry into nor exits `FM_ACTIVE`; it only withholds `fm_rx_active`, and no
cap write is needed because input throttle is already zero.
Failures 2–7 prevent proof/readiness while ARMED and end an ACTIVE run through STOPPING. Temporary
availability failures return to ARMED; a heading contradiction is terminal. Failure of
8 means ARMED/manual. Geometry/front warning checks are not activation or
fault conditions. Valid signed front geometry is still required to grant F4–F6 uncapped catch-up;
without it the normal rider-relative target is used. Condition 9 is the explicit cap-0 stop latch.

## 6. Target-point computation (the new control code)

Per control tick (10 Hz), all on RX:

1. **Filter:** EMA the raw 0xF3 rider position with the active preset's τ.
2. **Rider course + speed:** derived from successive filtered positions. Valid only while rider speed ≥ ~5 km/h (measured: course noise doubles below ~3 mph). Invalid course → degraded mode: offset direction = bearing(rider → buggy) ("hold station at distance"), no diagonal.
3. **Lag anchor:** a first-order EMA trails a moving target by v·τ (13–18 m at 15–20 mph with τ = 2 s — larger than the follow gap and speed-dependent). Correct it: `anchor = filtered_pos + û_course · min(v_rider · τ_active, 2 · d_follow)`. This makes geometry speed-independent without touching the proven smoothing (deliberately NOT solved by shrinking τ: the filter must keep ignoring carves — measured p95 turn rate 49°/s at ~5 m radius; the buggy follows the low-passed path, never mirrors bottom turns).
4. **Trailing point:** `target = anchor − d_follow · R(offset) · û_course`, where `d_follow = min_dist_m + followme_smoothing_band_m` and offset = 0° (mode 2), −`near_diag_offset_deg` (mode 1, rider's right), +`near_diag_offset_deg` (mode 3). Sign convention MUST be fixed against the code's bearing convention and verified in `Tools/FollowMe Settings Visualizer.html` before water.
5. **Steer:** feed `target` to the existing steering pipeline unchanged. Publish `fm_heading_err` and `fm_status` each rotation.
6. **Side-zone hysteresis:** when the angle between rider course and rider→buggy bearing crosses `zone_angle_enter_deg`/`zone_angle_exit_deg`, blend diagonal ↔ pure-behind (Schmitt pair) so an unstable rider course cannot whip the target point across the wake.

Modes 4–6 replace the trailing point with a front station on a `2 × d_follow` radius. F5 lies on
the rider course; F4 rotates the doubled radius by `-near_diag_offset_deg` toward rider-left and F6 by
the positive angle toward rider-right. Their steering points add at least another `2 × d_follow` of
rider-course lookahead while retaining the station's cross-track component. The point is kept ahead of the buggy, so excess lead is corrected
by slowing down rather than by a U-turn. Engagement uses the same **radial** `dist > D_engage`
2-second proof as F1–F3. Consequently F4–F6 may drive from behind toward their forward targets; there
is no no-autonomous-overtake gate. Loss of rider course, signed lead or selected axis raises a warning
only. With no valid rider course, F4–F6 temporarily aim straight ahead on the buggy's trusted heading.

## 7. Throttle cap chain (subtract-only; lowest cap wins)

| # | Cap | Source pattern |
|---|---|---|
| 1 | Hard stop: `dist <= min_dist_m` → cap 0; recovery above the boundary retains the separation proof; stationary `FM_ACTIVE` completion is owned by the common FM_RETURN path | FM recoverable stop |
| 2 | Approach ramp: F1–F3 linear 255→0 across the smoothing band; F4–F6 omit it because slowing when caught collapses the front gap | FM approach ramp |
| 3 | Stateful PI speed governor: outside the applicable distance-control band, F1–F6 request speed cap 3 open to 255; `boogie_vmax_in_followme_kmh` does not limit catch-up. F1–F3 leave catch-up at the radial `min_dist_m + band` edge; F4–F6 leave it when their valid signed positive along-track error enters one control band. Invalid front geometry cannot grant catch-up, and a 2 m Schmitt margin is required to re-enter it. In-band, F1–F3 target rider speed +10 km/h and F4–F6 vary from rider speed −10 to +10 km/h; F4/F6 target the longitudinal cosine component of their doubled diagonal station radius. A non-zero Boogie V-Max is the in-band ceiling. GPS speed and target are filtered, a 0.5 km/h deadband suppresses jitter, and a finite target reaches cap 0 at target +2 km/h. A separate published-cap state applies every reduction immediately and restores authority only on fresh buggy-GPS speed samples at at most 35 throttle counts/s, including the transition to catch-up's requested 255. | Run-phase governor |
| 4 | Align phase: heading error > threshold → 60/255 (~24 %) cap | Align-phase pattern |
| 5 | Engage ramp: 0→cap over 1.5 s whenever held trigger grants or resumes actual automatic authority | FM engage ramp |

FM writes caps only. The human trigger remains the sole throttle source; trigger release stops the
buggy through the unchanged base architecture without disarming FM. After FM has seen its first
throttle input, the TX keeps its mode declaration alive until explicit F0/gesture disarm, a
terminal RX-reported fault or declaration loss. Recoverable RX faults and normal FM Return
completion preserve the declaration.
Ordinary trigger release preserves a valid
separation proof. A trustworthy 2-second stationary proof instead completes every `FM_ACTIVE`
lifecycle through `FM_RETURN` and clears it, so the next automatic run must again prove
`dist > D_engage`. Explicit disarm remains the deterministic session boundary.

## 8. Parameters

F4–F6 reuse the existing FM parameters (no SW_VERSION bump):

`fm_diverge_dist_m` also requires no version bump: it renames the banked final float slot in place,
so `confStruct` remains 192 bytes. Explicit values are absolute metres. The effective value is raised
to `2 × D_engage` when necessary and capped at 100 m. Existing SW35 configurations contain `0`; this
compatibility value reconstructs the old `6 × D_engage` limit and then applies the 100 m cap.

| Param | v1 role | Owner default |
|---|---|---|
| `followme_mode` | TX starting mode / RX stored preference; live arming still requires 0xF2 | 2 (Behind) |
| `min_dist_m` | hard-stop distance | 10 m |
| `followme_smoothing_band_m` | hysteresis + ramp band (`d_follow = min_dist + band`; F4–F6 station radius = `2 × d_follow`) | 10 m |
| `near_diag_offset_deg` | diagonal offset for modes 1/3 and front modes 4/6; F5 uses 0° | 45° |
| `boogie_vmax_in_followme_kmh` | in-band F1–F6 PI ceiling only; catch-up always opens speed cap 3 to 255; 0 removes the absolute in-band ceiling | 25 km/h (~15.5 mph) |
| `zone_angle_enter_deg` / `zone_angle_exit_deg` | F1/F3 side-target Schmitt; F4–F6 selected-axis warning Schmitt only | 35° / 45° |
| `fm_engage_dist_m` | one radial F1–F6 activation and FM_RETURN arrival radius; 0 = auto, otherwise 8–50 m | 0 (auto) |
| `fm_diverge_dist_m` | absolute FM_ACTIVE sustained non-closing ceiling; effective minimum `2 × D_engage`, maximum 100 m; 0 = legacy auto | 100 m |
| `fm_warn_distance_m` (TX) | fresh TX-RX separation warning; medium pulse immediately and every 2 s at/above threshold; range 50–164 m | 150 m |
| `rtm_target_speed_kmh` | historical key: literal 0-50 km/h FM Return PI target; 0 means zero speed; non-zero Boogie V-Max may clamp it | 4 km/h |
| `rtm_align_threshold_deg` | historical key: FM Return align threshold | 45° |
| `rtm_approach_zone_m` | historical key: FM Return slowdown-band width outside `D_engage`; minimum effective width 2 m | 12 m |

Field-retunable to 4/10/20 m equivalents without reflash. At the next SW_VERSION bump (whenever one happens for other reasons), bake the proven values into `defaultConf` on both sides — a version bump resets stored config to `defaultConf` (verified against `ConfigService`/`SPIFFSEngine`; web-UI-only tuning does not survive bumps).

FM_RETURN has a fixed 60-second continuous-return safety timeout. A configurable RX-side
autonomous-runtime cap remains deferred to a future config-version bump; retired TX RTM fields are
not used at runtime.

## 9. Failure modes and recovery

| Failure | Detection | Response |
|---|---|---|
| TX (rider) GPS loss | age > `tx_gps_stale_timeout_ms` | cap 0, STOPPING → ARMED; declaration preserved, proof cleared, release acknowledgement if needed, then fresh `>D_engage` proof |
| RX GPS loss | age > 6 s | same recoverable path |
| Heading source temporarily unavailable | ladder empty without a disagreement latch | same recoverable path |
| LoRa loss | > `failsafe_time` | PWM pulses stop immediately; STOPPING → ARMED when control resumes, with fresh proof required |
| Compass disagrees with valid GPS COG | sustained guarded contradiction | cap 0, STOPPING → IDLE, deliberate re-arm required |
| Sustained divergence | distance > effective `fm_diverge_dist_m` for 3 s without closing by more than 2 m (after engage grace) | cap 0, STOPPING → IDLE, re-arm required |
| FM_RETURN runtime/not-closing | 60 s runtime or no 0.5 m closure over 5 s | cap 0, STOPPING → IDLE, re-arm required |
| Rider reaches stop radius | dist ≤ `min_dist_m` while ACTIVE and trigger held | cap 0; recovery above `min_dist_m` retains separation proof and resumes through the engage ramp |
| Rider stationary while ACTIVE | filtered speed < 2 km/h for 2 s with trustworthy position | always enter `FM_RETURN`; outside `D_engage` retrieve, at/inside it immediately complete to `FM_ARMED`; common cleanup clears all lifecycle latches |
| Rider course invalid | speed < ~5 km/h | F1–F3 use radial hold-station target; F4–F6 go straight on buggy heading and raise warning |
| F1–F3 warning geometry invalid | radial warning Schmitt | stay/control normally; 300 ms warning every 3 s; no cap/state/steering/latch change |
| F4–F6 lose valid signed front geometry | rider course/front projection unavailable | stay ACTIVE and steer on the existing degraded target; 300 ms warning every 3 s; withdraw uncapped catch-up and use normal rider-relative speed target; no state/steering/latch change |
| Trigger released | physical | motor stops; ordinary release and moving min-stop recovery leave state/proof untouched; stationary lifecycle completion is trigger-independent and uses FM_RETURN |
| Rider stationary while ARMED beyond effective `D_engage` | radial distance > `D_engage` and filtered speed < 2 km/h for 2 s | enter `FM_RETURN`; drive remains trigger-gated |
| FM Return arrival | radial distance ≤ `D_engage` | stop first, clear latches, preserve declaration and enter `FM_ARMED`; held trigger stays cap 0 until release, then manual cap 255; fresh `>D_engage` proof required |
| Rider moves during FM Return | filtered speed > 3 km/h for 1 s | same normal exit to `FM_ARMED`; never directly to `FM_ACTIVE` |
| Manual steering deflected ≥40 counts | raw RX steering input | rider steering wins; FM state/latch/cap preserved; centre to return |

**Field-validation prerequisite:** all FM states use the same guarded heading ladder. Verify GPS COG, compass orientation and steering direction on the actual hardware, wheels up and then at controlled low speed, before any open-water use.

## 10. Test plan

1. **Bench, motor off:** mode plumbing end-to-end; simulated coordinates through target-point math; sign/offset verification; all F1–F6 modes enter `FM_ACTIVE` only after radial `>D_engage` for 2 s, including with the trigger open, while `fm_rx_active` remains false until the trigger is held; zone-angle warning edges do not gate control; catch-up phase edges and 2 m re-entry hysteresis verified in every mode; invalid F4–F6 signed geometry cannot grant V-Max; overspeed cap reductions land immediately and recovery/catch-up exposure cannot exceed 35 throttle counts/s on fresh GPS samples; min-distance cap 0 recovers above the boundary without clearing separation; moving release preserves both latches; stationary ACTIVE at distances below, equal to and above `D_engage` enters RETURN after 2 s, with below/equal completing immediately to `FM_ARMED`; stationary ARMED enters RETURN only above `D_engage`; every fault gate force-failed → cap 0.
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

F4–F6 only extend TX mode selection and the existing 0xF2 value range. No new packet, config field or rate change (2 Hz 0xF3 remains unchanged).
