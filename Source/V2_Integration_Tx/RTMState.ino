// V2.5-Evo - 2026-08-28 - FM Warning Distance is now live: while TX and RX both report FM armed
// and the link is fresh, reaching telemetry distance >= fm_warn_distance_m queues one medium pulse
// immediately and every 2 s until the distance falls below the threshold. It shares Pattern 8 with
// the existing geometry warning so simultaneous informational conditions do not create overlapping
// buzz streams. The config slot and packet byte already existed; no layout or SW_VERSION change.
// V2.5-Evo - 2026-08-28 - While FM is armed and the trigger is released, LEFT/RIGHT hold now steps backwards/forwards through F1-F6 after 2 s and repeats every 2 s. F0 is excluded from this selector and remains an explicit combo-disarm action. Each step is transmitted immediately; no config, packet or SW_VERSION change.
// V2.5-Evo - 2026-08-27 - Follow-Me front family expanded to F4 Front-Left, F5 Front and F6 Front-Right. Both gesture cycles, starting-mode validation and display/meta declarations accept 1-6. The existing 0xF2 byte carries the values unchanged, so there is no packet or confStruct change and SW_VERSION stays 27.
// V2.5-Evo - 2026-08-26 - FM trigger release no longer disarms the TX. Once the rider has applied throttle, the selected 0xF2 mode declaration and its 30 s keepalive persist until explicit FM/F0 disarm, RTM preemption, an RX-reported fault or declaration loss. The existing fm_arm_window_s timeout still applies before the first throttle input. No config/packet/struct change; SW_VERSION stays 27.
// V2.5-Evo - 2026-04-25 - P7: TX RTM and FM state machines.
// RTM: left-hold gesture → arm → squeeze(s) → active → cooldown → idle
// FM: combo arms/disarms; while armed, released-trigger LEFT/RIGHT holds select F1-F6
// V2.5-Evo - 2026-04-27 - P8: setRtmArmed shows "rn" ×2 (static, 3s total); showFmMode shows F0-F3;
//   added setRtmDisarmed(); steer-exit gate in ACTIVE; rtm_max_runtime_s=0 disables runtime gate
// V2.5-Evo - 2026-04-27 - fix: extern declaration for current_vib_pattern (defined in System.ino)
// V2.5-Evo - 2026-04-27 - P8.1 Bug 2 fix: FM mode display uses scroll3Digits("FM[n]") — digit "1" as second
//   character of displayDigits() renders as a barely-visible horizontal bar, so all modes looked like "F"
// V2.5-Evo - 2026-04-28 - P9: Bug1B pre-arm check; Bug1D all-exit Pattern4/StP; S2 FM full-screen confirms
// V2.5-Evo - 2026-04-28 - P9 S4: rtm_arm_dist_m captured at engage; reset on disengage (R5 proximity bar)
// V2.5-Evo - 2026-04-28 - Chg5: runDoubleSqueezeArm() blocking double-squeeze ceremony; "A rM"→"A r"; "St P"→"St"
// V2.5-Evo - 2026-04-28 - ChgB/C/D/E: SPIFFS seed on first arm; cycle 1→2→3→1 (skip F0); "FM" confirm; 30s keepalive
// V2.5-Evo - 2026-04-28 - ChgDZ: persistent "r n" blinks use displayDigitZone() to preserve R5 proximity bar
// V2.5-Evo - 2026-04-28 - Bug2: setRtmArmed() clears fm_armed — RTM and FM are mutually exclusive
// V2.5-Evo - 2026-04-28 - Bug3: rtmDisengage() clears displayBuffer[6] (R5) to prevent FM phantom pixel
// V2.5-Evo - 2026-04-28 - Bug4: runDoubleSqueezeArm() rewritten — handles single+double squeeze; removes "A r"; RTM_ARMED dead code
// V2.5-Evo - 2026-04-28 - Task2: fmSilentDisarm() for arm-window expiry; cycleFmMode() cycles on armed+no-throttle; "F n" display
// V2.5-Evo - 2026-04-28 - Task3: bobbing advanceArrow() in RTM arm wait loops; delay(250) after unlockAnimation(); Pattern4 after animation; clear on timeout
// V2.5-Evo - 2026-04-28 - TaskA: rtm_arm_gps_timeout_override — 4× GPS staleness threshold during blocking arm ceremony;
//   cleared by rtmDisengage() and ceremony timeout paths; Gate 2 reads it via ternary.
// V2.5-Evo - 2026-04-29 - Fix 1-4: setRtmArmed() calls fmSilentDisarm() so RX
//   receives 0xF2/0 when RTM preempts FM — prevents stale fm_mode_runtime on RX
//   TODO: remove when runDoubleSqueezeArm() is refactored to non-blocking.
// V2.5-Evo - 2026-04-29 - Fix 4-3: fm_armed declared volatile (read by a task / written by loop())
// V2.5-Evo - 2026-04-29 - Fix 2-1: pre-arm rejection path now clears rtm_arm_gps_timeout_override
// V2.5-Evo - 2026-04-29 - F0: FM cycle extended to 1→2→3→0; landing on 0 disarms FM (RAM-only hand-off mode)
// V2.5-Evo - 2026-04-29 - Display: F0-F3 confirms and FM arm confirm now use large-font
//   displayDigits(LET_F, mode) instead of showFullScreenMessage() compact font
// V2.5-Evo - 2026-04-29 - Display: "St" confirm now uses large-font displayDigits(LET_S, LET_T)
//   in all three call sites (rtmDisengage, runDoubleSqueezeArm rejection, fmInternalDisarm)
// V2.5-Evo - 2026-04-29 - Bug fix: RTM arm always failed — two root causes:
//   BugA: decodeRtmDistanceM() returned 0.0m for zero-init telemetry (d==0x00 now → -1.0f)
//   BugB: GPS age exceeded 4× override on double-squeeze; drain Serial1 before RTM_ACTIVE
// V2.5-Evo - 2026-04-29 - GPS dot fix: gpsKeepAliveDelay() replaces bare delay() in
//   ceremony and confirms; drains Serial1 to keep gps_tx.location.age() fresh and
//   prevent GPS dot blinking during blocking display holds.
// V2.5-Evo - 2026-05-02 - Gate 3 throttle-release timeout reduced 10000→4000ms (10s was too long for tow buggy field use)
// V2.5-Evo - 2026-05-10 - SAFETY FIX: zero rtm_thr_cap_tx during arm ceremony to prevent motor runaway (see setRtmArmed)
// V2.5-Evo - 2026-07-20 - T1: Gate 1 throttle-release disarm was raised 3000 -> 30000 ms.
//   [SUPERSEDED 2026-08-26: trigger release no longer disarms FM on the TX.]
// V2.5-Evo - 2026-07-20 - StopFeel: every STOP/DISARM confirm now fires Pattern 7 (one 400ms long pulse)
//   instead of Pattern 4 (two 80ms taps), so arm and disarm feel different by touch. Changed sites:
//   rtmDisengage() (RTM disengage), fmDisarm() (FM disarm), the runDoubleSqueezeArm() pre-arm "St"
//   rejection, and both F0-disarm paths (cycleFmMode / cycleFmModeArmed). [SUPERSEDED 2026-08-28:
//   cycleFmModeArmed now skips F0; only the combo-owned cycleFmMode path can select it.] ARM confirms are UNCHANGED and
//   stay Pattern 4: setRtmArmed(), the two runDoubleSqueezeArm() squeeze confirms, and the cycleFmMode()
//   arm path. fmSilentDisarm() stays silent (arm-window expiry is not a commanded stop).
// V2.5-Evo - 2026-08-17 - StopBuzz FIX: the 2026-08-16 haptic cut was applied inside rtmDisengage()
//   and fmDisarm() — the SHARED sinks — so it also silenced every stop the rider did NOT ask for
//   (RTM Gate 1 max-runtime, Gate 2 GPS-stale, Gate 3 throttle-release, FM Gate 1 release backstop,
//   FM RX fault-stop). That is the exact inverse of the intent. Both functions now take a
//   `commanded` flag: true = the rider asked for it (stay silent), false = a safety gate stopped
//   the system (fire the long STOP buzz). Every call site is classified explicitly below.
//   All Pattern 7 requests now go through vib_stop_pending (System.ino) so a stop buzz can never be
//   swallowed by a pattern that happens to be mid-play.
// V2.5-Evo - 2026-08-17 - StopBuzz REVISION (supersedes the classification in the entry above):
//   the rule is now A PURE TIMEOUT IS SILENT, A FAULT BUZZES. On a wave the rider has no attention
//   to spare for decoding a buzz, and the more buzzes there are the less each one is read, so
//   Pattern 7 is spent on faults only. The then-existing FM release timeout and RTM Gate 3 were
//   silent because both could only fire after the trigger was already released. [2026-08-26: the
//   FM timeout has been removed; RTM Gate 3 is unchanged.]
//   Still commanded = false (BUZZ): RTM Gate 2 (TX GPS lost) and the FM RX fault-stop - faults, not
//   timers - plus RTM Gate 1 (max runtime), the one timer that can fire MID-SQUEEZE, where
//   restoring rtm_thr_cap_tx to 255 un-clamps Throttle.ino and hands the rider raw manual throttle
//   with no gesture behind it. The two arm REFUSALS (RTM pre-arm distance, FM fundamental reject)
//   are unchanged and still buzz.

extern volatile uint8_t current_vib_pattern;
extern volatile bool    vib_stop_pending;   // set true to REQUEST the Pattern 7 STOP buzz (defined in
                                            // System.ino). Never write current_vib_pattern = 7 directly:
                                            // the flag is what makes the stop buzz preempt and survive.
// ---- GPS-aware blocking delay ----
// Replaces bare delay() calls in the arm ceremony and mode confirms.
// Drains Serial1 in 10ms chunks so gps_tx.location.age() stays fresh.
// Prevents the GPS dot from blinking during blocking display animations.
// Safe to call from loop() only — gps_tx.encode() must not be
// called concurrently from core 0.
static void gpsKeepAliveDelay(uint32_t ms)
{
  unsigned long start = millis();
  while (millis() - start < ms)
  {
    while (Serial1.available()) gps_tx.encode(Serial1.read());
    delay(10);
  }
}


// ============================================================
// FM STATE MACHINE
// V2.5-Evo - 2026-04-27 - P8.1: FM redesigned as arm/disarm toggle with persistent mode memory.
//
// ARM (LEFT tap + RIGHT hold 5s, first time or after disarm):
//   - Arms at last_fm_mode (RAM; defaults to F1 on power cycle — never resets to F0/disabled)
//   - Blinks "F[mode]" x2 on display; fires Pattern 4 (2 fast buzzes) as arm confirm
//   - FM active: user engages throttle to ride
//
// CHANGE MODE while armed and trigger released (intercepted by Hall.ino):
//   - LEFT hold steps backwards, RIGHT hold forwards through F1-F6 after 2 s and every 2 s after.
//   - F0 is excluded; explicit disarm remains on the existing combo gesture.
//
// DISARM (any of):
//   - Same combo again (LEFT tap + RIGHT hold 5s) — toggle
//   - Arm window expires (fm_arm_window_s) before any throttle input — auto-disarm
//   - RX-reported FM fault, legacy-RX FM_RETURN completion, or F0 selection
// ============================================================

static bool          fm_session_init_done = false;  // last_fm_mode seeded from config this session
static unsigned long fm_last_sync_ms      = 0;      // last 0xF2 keepalive; 0 while disarmed
volatile bool        fm_armed         = false;  // FM arm state; RAM only, cleared on power cycle. Not static — extern'd by Display.ino (R5 bar)
                                                 // volatile: read by updateBargraphs() (task), written by loop()
static uint8_t       last_fm_mode     = 1;      // last active FM mode (1-6); defaults F1; RAM only
static unsigned long fm_arm_ms        = 0;      // time of arm; used only until first throttle input
static bool          fm_throttle_seen = false;  // becomes true once thr_scaled>10 after arming

// Returns true if FM is currently armed; Hall.ino uses it to intercept both simple hold directions.
bool isFmArmed() { return fm_armed; }

// V2.5-Evo - 2026-07-20 - Batch T: previous telemetry.fm_flags snapshot for bit3 (fault-stop)
// rising-edge detection in runFmLoop(). Updated every runFmLoop() tick so re-arming always
// starts from a fresh baseline (no stale edge). RAM only.
static uint8_t fm_flags_prev = 0;

// RX geometry warnings use the two free high bits of fm_flags. The TX-side distance threshold uses
// a synthetic ninth bit in this local warning mask; it changes no packet. All are conditions, not
// lifecycle states, and repeat even with the trigger released. Pattern 8 is queued immediately on
// an edge, then every 2 s while the distance condition is present or every 3 s for geometry alone.
// One shared scheduler prevents simultaneous conditions from creating overlapping buzz streams;
// higher-priority haptics defer rather than consume a due warning.
static const uint16_t      kFmDistanceWarningMask     = 0x0100u;
static const unsigned long kFmDistanceWarningPeriodMs = 2000UL;
static const unsigned long kFmGeometryWarningPeriodMs = 3000UL;
static unsigned long fm_warning_last_ms = 0;
static uint16_t      fm_warning_prev    = 0;
static bool          fm_warning_sent    = false;

static void fmResetWarningScheduler()
{
  fm_warning_last_ms = 0;
  fm_warning_prev    = 0;
  fm_warning_sent    = false;
}

// ============================================================
// V2.5-Evo - 2026-07-20 - Batch T (Fable FM v1.4): FM arm-time and display readiness gating.
// All inputs are TX-LOCAL (paired flag, own GPS fix/age, last-reply age) plus the RX's own
// armed-not-ready bit — instant, zero telemetry dependency, no new confStruct field. Called
// only from the loop task: fmFundamentalReject() from cycleFmMode(), fmArmedNotReady()
// from updateR5ProximityBar() via the render path. gps_tx access stays on the loop task (invariant).
// ============================================================

// FUNDAMENTAL arm-time reject — the three conditions under which arming would be a lie AND a
// rider genuinely mid-tow cannot be in, so false-refusal ≈ 0. Everything else (transient GPS
// staleness, a momentary packet gap, high HDOP) is NOT fundamental: it arms as NOT-READY, so a
// brief mid-tow glitch never blocks the magnet. Returns true = refuse the fresh arm.
bool fmFundamentalReject()
{
  if (!usrConf.paired)                return true;   // never paired
  if (last_packet == 0)               return true;   // no RX packet ever this session
  if (!gps_tx.location.isValid())     return true;   // no TX GPS fix ever this session (isValid latches on first fix)
  return false;
}

// ARMED-NOT-READY vs READY — the OR of the RX's armed-not-ready bit (bit2) with the TX-local
// transient readiness set. Any not-ready input → true (scanner blinks in place). None → false
// (scanner sweeps). Flips live each render, so the "now it's good" moment appears for free.
bool fmArmedNotReady()
{
  if (telemetry.fm_flags & FM_FLAG_NOTREADY)                          return true;  // RX half: latch not yet proven
  if (!usrConf.paired)                                               return true;  // pairing lost
  if (last_packet == 0 || (millis() - last_packet) >= FM_LINK_HEALTHY_MS) return true;  // RX telemetry not recent / link unhealthy
  if (!(gps_tx.location.isValid() &&
        gps_tx.location.age() < (unsigned long)usrConf.tx_gps_stale_timeout_ms))  return true;  // TX GPS fix missing or stale
  return false;  // all TX-local inputs fresh AND RX not-ready clear → READY → sweep
}

// Silent disarm: clears FM state and notifies RX, but shows no display and fires no haptic.
// Used when arm-window expires before any throttle input — nothing active to confirm.
static void fmSilentDisarm()
{
  fm_armed         = false;
  fm_throttle_seen = false;
  fm_last_sync_ms  = 0;
  fmResetWarningScheduler();
  queueMetaPacketBurst(0xF2, 0);   // mode 0 = FM disabled on RX
}

// Internal disarm: clears state, notifies RX, shows "St" full-screen, buzzes only on a FAULT.
// V2.5-Evo - 2026-04-28 - P9 S2: showFmMode() removed; disarm shows blocking stop message.
// V2.5-Evo - 2026-04-28 - ChgE: fm_last_sync_ms reset to 0 on disarm so keepalive timer clears.
// INPUT: commanded — true = stay SILENT, false = fire the Pattern 7 STOP buzz. Since the 2026-08-17
//        revision the test is "FAULT or TIMEOUT", not "did the rider press something": true for the
//        deliberate disarms (toggle combo, magnet toggle, F0); false only for the RX fault-stop.
// OUTPUT: none. SIDE EFFECTS: fm_armed cleared, keepalive stopped, 0xF2/0 sent to RX, STOP buzz
//        requested when uncommanded, and a BLOCKING 2s "St" display hold.
static void fmDisarm(bool commanded)
{
  fm_armed         = false;
  fm_throttle_seen = false;
  fm_last_sync_ms  = 0;            // Change E: clear keepalive timer
  fmResetWarningScheduler();
  queueMetaPacketBurst(0xF2, 0);   // mode 0 = FM disabled on RX (followme_mode=0)
  // V2.5-Evo - 2026-08-16 - HAPTIC CUT: silent on a DELIBERATE disarm. You just did it, and the
  // display already says so. A buzz confirming your own action is noise.
  // V2.5-Evo - 2026-08-17 - StopBuzz: the caller now decides, on the rule A PURE TIMEOUT IS SILENT,
  // A FAULT BUZZES. Exactly ONE FM path is a fault — the RX fault-stop in runFmLoop(), where the RX
  // gave up steering on its own and the rider has no way to know. Trigger release no longer calls
  // this function at all; it leaves FM armed. Pattern 7 is therefore spent on RX faults only.
  if (!commanded) vib_stop_pending = true;   // Pattern 7: one long buzz = a FAULT stopped the system

  // Large-font stop confirm on FM disarm.
  DISP_LOCK(); displayDigits(LET_S, LET_T); updateDisplay(); DISP_UNLOCK();
  gpsKeepAliveDelay(2000);
}

// Called by handleGearToggle() combo (LEFT tap + RIGHT hold 5s) — toggles arm/disarm.
// On arm: seeds last_fm_mode from SPIFFS on first arm this session (Change B); fires Pattern 4;
// shows "FM" confirm (Change D, 6 cols ≤ C0-C5); sends 0xF2 to RX; starts keepalive timer (Change E).
void cycleFmMode()
{
  if (!usrConf.fm_override_enabled || !usrConf.gps_en) return;

  if (fm_armed)
  {
    if (fm_throttle_seen)
    {
      // User already rode — treat gesture as disarm toggle
      fmDisarm(true);   // COMMANDED: the rider made the disarm gesture → silent
    }
    else
    {
      // No throttle yet — cycle to next mode (1→2→3→4→5→6→0 where 0 = disarm)
      last_fm_mode = (last_fm_mode < 6) ? last_fm_mode + 1 : 0;

      if (last_fm_mode == 0)
      {
        // F0: FM disabled — disarm with brief visual confirm and return to normal display.
        // Sends 0xF2/0 to RX (FM off) and resets mode to SPIFFS default. No buzz — selecting F0 is
        // a deliberate disarm the rider is watching happen on the display (comment corrected
        // 2026-08-17: it said "fires Pattern 7", which stopped being true with the 08-16 cut).
        // This is RAM-only; power cycle restores usrConf.followme_mode.
        // V2.5-Evo - 2026-08-16 - HAPTIC CUT: silent on a DELIBERATE disarm. You just did it, and

        // the display already says so - RTM/FM stops being shown and the stop confirm appears. A buzz

        // confirming your own action is noise, and it was the single most frequent buzz in the system.

        // V2.5-Evo - 2026-08-17 - and after the StopBuzz revision Pattern 7 means ONE thing:
        // a FAULT stopped the system. Timeouts — including the ones the rider did not ask for —
        // are silent, so this deliberate F0 disarm is in the majority, not the exception.

        // Large-font F0 disarm confirm: LET_F + 0. Shorter hold (1s) — this is a disarm, not a mode select.
        DISP_LOCK();
        displayDigits(LET_F, 0);
        updateDisplay();
        DISP_UNLOCK();
        gpsKeepAliveDelay(1000);
        queueMetaPacketBurst(0xF2, 0);       // tell RX: FM disabled
        fm_armed         = false;
        fm_throttle_seen = false;
        fm_last_sync_ms  = 0;
        // Reset mode to SPIFFS default so next arm starts at configured mode, not 0
        last_fm_mode = (usrConf.followme_mode >= 1 && usrConf.followme_mode <= 6)
                       ? usrConf.followme_mode : 1;
        return;
      }

      // Large-font mode confirm: LET_F + mode digit (1-6). snprintf no longer needed.
      DISP_LOCK();
      displayDigits(LET_F, last_fm_mode);
      updateDisplay();
      DISP_UNLOCK();
      gpsKeepAliveDelay(2000);
      queueMetaPacketBurst(0xF2, last_fm_mode);
      fm_last_sync_ms = millis();
      fm_arm_ms       = millis();   // reset arm window — user is actively choosing a mode
    }
    return;
  }

  // Backward compatibility with an older RX whose RETURN arrival was a two-sided terminal
  // transition. Current RX firmware exits normal RETURN to FM_ARMED and never emits FM_FLAG_DONE.
  // If an old RX reports it, re-acknowledge 0xF2/0 instead of sending a declaration it must reject.
  if (telemetry.fm_flags & FM_FLAG_DONE)
  {
    queueMetaPacketBurst(0xF2, 0);
    DISP_LOCK(); displayDigits(LET_I, LET_D); updateDisplay(); DISP_UNLOCK();
    gpsKeepAliveDelay(1000);
    return;
  }

  // V2.5-Evo - 2026-07-20 - Batch T (Fable FM v1.4): FUNDAMENTAL arm-time reject.
  // Reached only on a FRESH arm (fm_armed was false above). Covers BOTH entry points — the
  // toggle combo AND the magnet gesture both funnel through cycleFmMode() when disarmed. On a
  // fundamental not-ready state the arm DOES NOT TAKE: fire Pattern 7 (long stop buzz) + "St",
  // leave fm_armed false, and return. Marginal/transient conditions are deliberately NOT checked
  // here — they arm as NOT-READY (fmArmedNotReady()) so the magnet still arms through a glitch.
  if (fmFundamentalReject())
  {
    // Pattern 7: one long buzz = the arm was REFUSED. Kept (the rider would otherwise walk away
    // believing FM is armed), and routed through the pending flag like every other stop request.
    vib_stop_pending = true;
    DISP_LOCK(); displayDigits(LET_S, LET_T); updateDisplay(); DISP_UNLOCK();
    gpsKeepAliveDelay(2000);
    return;                          // arm refused — no fm_armed, no 0xF2, no keepalive
  }

  // V2.5-Evo - 2026-04-28 - Change B: On first arm this session, seed last_fm_mode from SPIFFS.
  // usrConf.followme_mode is the user's configured starting mode (range 1-6; 0 is invalid here).
  // After seeding, fm_session_init_done prevents overriding any mode the user cycled to mid-session.
  if (!fm_session_init_done)
  {
    if (usrConf.followme_mode >= 1 && usrConf.followme_mode <= 6)
      last_fm_mode = usrConf.followme_mode;
    fm_session_init_done = true;
  }

  // Arm at last used mode (never arms at F0 = disabled; last_fm_mode defaults to 1)
  fm_armed         = true;
  fm_arm_ms        = millis();
  fm_throttle_seen = false;
  if (current_vib_pattern == 0) current_vib_pattern = 4;         // Pattern 4: 2 fast buzzes = arm confirm
  fm_last_sync_ms  = millis();     // Change E: start keepalive timer from now (avoids immediate re-sync)

  // Display the actual mode being armed (F1-F6) in large font
  // instead of the generic "FM" text. Uses large num0[] font via LET_F(15) + mode digit.
  DISP_LOCK();
  displayDigits(LET_F, last_fm_mode);
  updateDisplay();
  DISP_UNLOCK();
  gpsKeepAliveDelay(2000);

  queueMetaPacketBurst(0xF2, last_fm_mode);
}

// Called by Hall.ino once per 2-second hold interval while FM is armed and the trigger is released.
// LEFT/negative steps backwards and RIGHT/positive forwards. Only F1-F6 participate: F0 is an
// explicit disarm command and cannot be reached accidentally by holding a direction.
void cycleFmModeArmed(int direction)
{
  if (!fm_armed || direction == 0) return;
  last_fm_mode = followMeStepActiveMode(last_fm_mode, direction);

  // Publish before returning to the hold loop, so the RX begins its safe mode-transfer ramp without
  // waiting for the rider to release the toggle or for another two-second repeat interval.
  DISP_LOCK();
  displayDigits(LET_F, last_fm_mode);
  updateDisplay();
  DISP_UNLOCK();
  queueMetaPacketBurst(0xF2, last_fm_mode);
  fm_last_sync_ms = millis();             // reset keepalive — just synced
  fm_arm_ms       = millis();             // reset arm window — user is actively choosing a mode
}

// Called from loop() every ~110ms.
// Handles the pre-throttle arm-window auto-disarm, RX fault handoff and 0xF2 keepalive.
void runFmLoop()
{
  unsigned long now = millis();

  // V2.5-Evo - 2026-07-20 - Batch T (Fable FM v1.4): DISARM OWNERSHIP — the display can't lie.
  // The RX owns engagement; on an RX fault it stops FM and raises fm_flags bit3 (fault-stop),
  // held sticky ~6s so this ~110ms loop is guaranteed to catch the rising edge across the
  // ~2.4s telemetry rotation. On that rising edge, while WE still believe we are armed, the TX
  // must clear its own arm and STOP re-declaring 0xF2/mode — otherwise the 30s keepalive below
  // would re-arm the RX within 30s of a fault. fmDisarm() does exactly that: fm_armed=false
  // (so this function early-returns next tick and the keepalive never fires), queues 0xF2/0
  // (belt-and-suspenders — RX already idle), and shows the stop as "St" + Pattern 7. bit3 is
  // already surprise-gated on the RX, so it is only set when the alarm is warranted — no TX
  // re-gating needed. fm_flags_prev is updated every tick (armed or not) so a re-arm starts clean.
  uint8_t fm_flags_now = telemetry.fm_flags;
  bool fault_rising = (fm_flags_now & FM_FLAG_FAULT) && !(fm_flags_prev & FM_FLAG_FAULT);
  bool done_rising  = (fm_flags_now & FM_FLAG_DONE)  && !(fm_flags_prev & FM_FLAG_DONE);
  fm_flags_prev = fm_flags_now;
  if (fm_armed && done_rising)
  {
    // Legacy-RX FM_RETURN arrival, not a fault: stop the declaration and its keepalive so that RX's
    // terminal FM_IDLE cannot be undone 30 s later. Current RX firmware does not take this path.
    fm_armed         = false;
    fm_throttle_seen = false;
    fm_last_sync_ms  = 0;
    fmResetWarningScheduler();
    queueMetaPacketBurst(0xF2, 0);
    DISP_LOCK(); displayDigits(LET_I, LET_D); updateDisplay(); DISP_UNLOCK();
    gpsKeepAliveDelay(1000);
    return;
  }
  if (fm_armed && fault_rising)
  {
    // FAULT — and since the 2026-08-17 revision this is the ONLY FM path that buzzes. The RX
    // faulted and stopped following by itself: the rider asked for nothing, no timer explains it,
    // and he has no other way to learn the buggy is no longer steering for him. → commanded =
  // false → Pattern 7. Trigger release itself never enters this path.
    fmDisarm(false);   // clears fm_armed + keepalive, sends 0xF2/0, "St" + Pattern 7 — TX & RX can't disagree
    return;
  }

  if (!fm_armed)
  {
    fmResetWarningScheduler();
    return;
  }

  // Periodic FM warnings intentionally have NO throttle gate: trigger release withdraws automatic
  // authority, but it must not hide excessive separation or bad geometry. Distance requires a
  // fresh link and matching TX/RX armed state. Geometry is already owned by the RX fm_flags.
  // Pattern 8 is informational and never overwrites a fault/stop or another pattern in progress.
  const bool link_fresh = last_packet != 0 && (now - last_packet) < FM_LINK_HEALTHY_MS;
  const bool distance_warning_now = followMeDistanceWarningActive(
      fm_armed,
      (fm_flags_now & FM_FLAG_ARMED) != 0,
      link_fresh,
      telemetry.rtm_distance,
      usrConf.fm_warn_distance_m);

  uint16_t warning_now = fm_flags_now & (FM_FLAG_GEOMETRY | FM_FLAG_FRONT_LOST);
  if (!(fm_flags_now & FM_FLAG_ARMED)) warning_now = 0;
  if (distance_warning_now) warning_now |= kFmDistanceWarningMask;

  if (warning_now == 0)
  {
    fmResetWarningScheduler();
  }
  else
  {
    const unsigned long warning_period_ms = distance_warning_now
        ? kFmDistanceWarningPeriodMs
        : kFmGeometryWarningPeriodMs;
    const bool warning_changed = warning_now != fm_warning_prev;
    const bool warning_due = warning_changed || followMeWarningPulseDue(
        true, fm_warning_sent, fm_warning_last_ms, now, warning_period_ms);
    if (warning_due && current_vib_pattern == 0 && !vib_stop_pending)
    {
      current_vib_pattern = 8;  // one medium pulse; implemented in vibrationTask()
      fm_warning_last_ms = now;
      fm_warning_sent = true;
      // Advance the observed mask only after a pulse was really queued. If another haptic is busy,
      // leaving the old mask in place preserves the edge and retries it on the next loop tick.
      fm_warning_prev = warning_now;
    }
  }

  // Arm-window auto-disarm: if user never applied throttle since arming, disarm after fm_arm_window_s
  if (!fm_throttle_seen)
  {
    if (now - fm_arm_ms > (unsigned long)usrConf.fm_arm_window_s * 1000UL)
    {
      // V2.5-Evo - 2026-07-20 - Batch T (A2 D1): the silent disarm is no longer fully silent.
      // The scanner going dark is the primary signal; add ONE short blip (Pattern 5, a single
      // 150ms pulse) as a NUDGE so a rider relying on the long arm window isn't left believing
      // he is still armed. Distinct by feel from the two-tap arm (Pattern 4) and the long stop
      // buzz (Pattern 7). Placed at the call site (not inside fmSilentDisarm) so it fires ONLY on
      // arm-window expiry, never on the RTM-preemption path that also calls fmSilentDisarm().
      fmSilentDisarm();   // arm window expired before first throttle — no blocking confirm
      if (current_vib_pattern == 0) current_vib_pattern = 5;   // nudge: one short blip
      return;
    }
  }

  // Once throttle has been used, the pre-throttle arm window no longer applies. Trigger release
  // deliberately does not clear fm_armed or stop the 0xF2 keepalive.
  if (thr_scaled > 10)
  {
    fm_throttle_seen = true;
  }

  // V2.5-Evo - 2026-04-28 - Change E: Send 0xF2 keepalive every 30s while FM is armed.
  // Ensures RX stays in the correct FM mode after any transient packet loss.
  if (fm_last_sync_ms > 0 && now - fm_last_sync_ms >= 30000UL)
  {
    queueMetaPacketBurst(0xF2, last_fm_mode);
    fm_last_sync_ms = now;
  }
}
