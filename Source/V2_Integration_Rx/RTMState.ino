// V2.5-Evo - 2026-08-27 - A proven compass-vs-COG disagreement no longer blocks or aborts Follow-Me. The disagreement still latches and withdraws the compass, but live GPS COG and the short held-COG bridge remain valid FM heading sources. Per-tick disagreement no longer vetoes that valid COG. FM still requires an actual heading: no/stale/frozen COG while the compass is withdrawn remains a condition-6 failure. Telemetry and serial diagnostics now report GPS-only degradation instead of "will not engage". No config/packet/SW_VERSION change.
// V2.5-Evo - 2026-08-27 - FM divergence ceiling is now configurable as the absolute-metre fm_diverge_dist_m. Its effective value is never below 2 x D_engage and never above 100 m. The setting claims the existing final reserved float without changing layout or SW_VERSION; 0 reconstructs the old 6 x D_engage behavior and applies the new 100 m cap for existing SW35 configs. Dwell, closure epsilon, engage grace and fault path are unchanged.
// V2.5-Evo - 2026-08-27 - RX FM D-term sign fix. d_error is calculated as (heading_error_now - heading_error_previous) / dt, so the correct PD law is Kp*error + Kd*d(error)/dt. The previous subtraction was anti-damping: a negative derivative while closing the error increased steering instead of reducing it. The existing +/-180 deg delta normalization and source-change resets remain unchanged. No gain, config, packet or struct change; SW_VERSION stays 35.
// V2.5-Evo - 2026-08-26 - Follow-Me now has one radial activation boundary: every F1-F4 mode proves dist > effective D_engage for 2 s; side/front geometry never gates steering or changes the throttle cap and is warning-only. Once ACTIVE, reaching min_dist_m latches cap 0 until the rider releases the trigger; that release clears the stop and separation proof, exposes manual cap 255, and a later automatic resume must re-prove >D_engage. Ordinary trigger release leaves the current cap untouched because the trigger itself already commands zero. FM_HOLD remains removed and the retired low-speed config float stays reserved in-place for SW35 ABI compatibility.
// V2.5-Evo - 2026-08-27 - FM_RETURN now clears the separation latch on entry and always exits normally to FM_ARMED: both arrival below effective D_engage and a moving-rider cancellation preserve the live F1-F4 declaration but require a fresh radial >D_engage proof before automatic Follow-Me may engage again. There is no normal RETURN -> ACTIVE shortcut and no arrival-driven RETURN -> IDLE/TX-disarm handshake. A held trigger remains capped at zero until released once at either normal RETURN exit, preventing an ARMED/manual-throttle surge. Fault, explicit disarm, config disable and declaration expiry retain their existing STOPPING/IDLE semantics. No packet-size or confStruct-size change.
// V2.5-Evo - 2026-08-26 - FM_RETURN replaces the separately armed RTM product mode. Any live F1-F4 declaration can enter FM_RETURN after fresh/plausible TX+RX positions show the foiler below 2 km/h and radially beyond effective D_engage for 2 s, including stationary arming before a tow. FM_RETURN holds still for that proof dwell, then uses the shared direct-to-rider RTM steering/align/speed-governor control under the unchanged trigger deadman. [SUPERSEDED 2026-08-27: normal RETURN exits now preserve the declaration and enter FM_ARMED as described above; the completion-bit/IDLE handshake was removed.]
// V2.5-Evo - 2026-08-26 - [SUPERSEDED later the same day] The stationary-near separation reset described here was replaced by the deterministic min-distance stop/release rule above. FM_RETURN still uses the fixed <2 km/h, >D_engage, 2 s proof. [SUPERSEDED 2026-08-27: normal RETURN exits now enter FM_ARMED.]
// V2.5-Evo - 2026-08-26 - FM rider-override semantics changed. Releasing the trigger remains the immediate physical deadman stop without directly ending the FM lifecycle. The TX keeps declaring the selected FM mode until explicit FM/F0 disarm, pre-throttle arm-window expiry, fault or declaration loss. Manual steering outside kFmManualSteerDeadband wins at PWM cadence without steer-cancelling FM or clearing its latch; centring hands steering back to FM, and divergence proof is parked during deliberate manual deflection. Genuine GPS/link/heading/divergence faults remain safety stops; F4 front loss is warning-only per the newer rule above. No config/packet/struct change; SW_VERSION stays 35.
// V2.5-Evo - 2026-08-26 - F4 now accepts boogie_vmax_in_followme_kmh=0 with the same documented meaning as the other Follow-Me modes: no absolute vehicle-speed ceiling. The signed front-gap governor remains active and still targets rider speed +/- the existing closing margin; only the final absolute clamp is skipped. [Later the same day: the front cone became warning-only per line 1.] No config/packet/struct change; SW_VERSION stays 35.
// V2.5-Evo - 2026-08-25 - F4 IN FRONT added as a forward-pacer Follow-Me geometry. [SUPERSEDED 2026-08-26: its activation proof is now the same radial >D_engage dwell as F1-F3, so it may autonomously move from behind to the front target; the front cone/loss is warning-only and never clears the latch or changes cap/steering authority. The original zero-vmax refusal is also superseded: zero means no absolute ceiling.] No new packet, config field or confStruct change; SW_VERSION stays 35.
// V2.5-Evo - 2026-08-25 - RX FM HOLD manual-recovery delay reduced 10 -> 2 s. [SUPERSEDED 2026-08-26/27: FM_HOLD was removed; ordinary release remains FM_ACTIVE and preserves proof, while release of a min-distance stop clears its proof immediately.] Compile-time timing change only; no confStruct change; SW_VERSION stays 35.
// V2.5-Evo - 2026-08-25 - RX RTM/FM D-term wrap fix. heading_error itself was normalized to +/-180 deg, but the derivative subtracted two normalized samples directly. Crossing the branch cut (for example +179 -> -179) therefore looked like a -358 deg step instead of the physical +2 deg change and Kd could saturate steering for one control tick. Normalize the same-source error delta to +/-180 before dividing by dt; source-switch/re-snap suppression, P term, gains, logging and config stay unchanged. No confStruct change; SW_VERSION stays 35.
// V2.5-Evo - 2026-08-17 - THREE FOLLOW-UPS TO THE PASS BELOW, ALL OF THEM NOTIFICATION, NONE OF THEM CONTROL. (1) THE DEGRADATION NOTICE COULD BE LOST ENTIRELY, NOT MERELY DEFERRED. headingDisagreeAnnounceDegraded() rightly returns without setting its one-shot flag while thr_received >= 25 — four Serial lines upstream of a hard stop would break the motor-to-zero-first rule — but its ONLY call site was inside the if (disagree_now) branch, so the retry needed another MEASURED disagreement. A measurement needs a live COG plus a compass snapshot younger than kHeadingCompareSnapMs, and that snapshot only refreshes while the trigger is released, so a dwell that completed inside the ~1 s window after a squeeze was silenced — and a rider who then finished the session under power and never coasted above rtm_cog_min_speed_kmh again rode the WHOLE SESSION with the compass withdrawn and Follow-Me refusing to engage, announced nowhere but a manual ?diag. getRtmHeading() now offers the notice on EVERY tick while the verdict stands, so the retry no longer depends on the evidence coming back; the deferral guard itself is untouched, and the print still cannot land between a proven fault and a motor-stopping write, because it can only fire below 25 counts where the deadman already holds the motor at 0. (2) A FAULT PROVEN WHILE COASTING NOW REACHES THE REMOTE. fm_fault_alarm_ms was set only if (thr_held), but the heading-disagree latch can only complete with the trigger RELEASED — so for this one fault the sticky fm_flags bit 3 never rose, the TX never learned the run had ended on a fault, and Follow-Me silently re-armed on the next keepalive into a blocked ARMED state whose only field signal was the not-ready flag. The alarm is now also set for a standing heading-disagree fault; every other fault keeps the surprise gating exactly as it was. (3) COMMENT-ONLY: the note in front of the restored FM fault term claimed a HOLD-parked Follow-Me would sit at cap 0 "for the rest of the session". The throttle-release clear rescues FM_HOLD back to FM_ARMED after 10 continuous seconds below 25 counts, so the accurate hazard is narrower — a rider FEATHERING the trigger restarts that timer on every squeeze, never accumulates the 10 s, and gets a dead motor on every squeeze with no explanation. Plus heading_disagree_fault is now volatile: it is read cross-task by Logger.ino through headingDisagreeLatched(), and as a file-scope static whose address never escapes the compiler may cache it. Read-only, log columns only, no control impact. No confStruct change, sizeof stays 192, SW_VERSION stays 35.
// V2.5-Evo - 2026-08-17 - THREE CORRECTIONS TO THE DEGRADATION PASS BELOW, WHICH ASSUMED THE COMPASS WAS THE LIAR. The cross-check proves only that the compass and the GPS course CANNOT BOTH BE RIGHT. It does not say which one is wrong — the guard says so itself in guard 2's own header: "It deliberately does NOT pick a winner." Degrading the session onto GPS course alone picks one anyway, and it picks the compass as the culprit; if the RX's course is the bad source instead (marina multipath, a wrong dynamic platform model, a lagged COG at low speed) the degradation withdraws the GOOD sensor and operates on the bad one. (1) FOLLOW-ME REFUSES AGAIN — RELEASE BLOCKER. !heading_disagree_fault is back in can_be_active, and back in the FM fault-stop classifier it was deleted with, so a disagreement proven mid-engagement ends the run through the existing FM_STOPPING ramp (back to manual, re-arm required) instead of parking FM in FM_HOLD at cap 0 for the rest of the session. RTM KEEPS DEGRADING, and that part was sound: RTM's degraded behaviour is BOUNDED — with no heading the steering override is pinned at 127 and the align cap holds the throttle at 13/255 on the 180 deg sentinel — and the alternative was a genuinely dangerous half-armed state, buggy dead with the throttle at 0 while the TX still displayed RTM as ACTIVE. FOLLOW-ME IS NOT BOUNDED LIKE THAT: it engages autonomously at rider speed plus a margin, its steering authority is continuous, its only backstops are the divergence fault (about a 6.5 s grace plus a 3 s dwell) and the deadman — and the disagreement is UNMEASURABLE during the engagement, because compare_possible needs a compass snapshot younger than kHeadingCompareSnapMs and the snapshot only refreshes while thr_received < 25, so about a second into the run the comparison goes dormant and COG is served at confidence 3 unchallenged. Refusing to engage is the right answer to "one of my two heading sources is lying and I cannot tell which", and it is the same answer guard 1 already gives a few lines up: HOLDING STRAIGHT IS SAFER THAN STEERING ON THE SURVIVOR. The rider is not left guessing: the one-shot degradation notice still prints, the rate-limited "ARMED, NOT ENGAGING" explanation is restored, fm_flags bit 2 (armed-not-ready) carries the fault again so the TX cannot render "ready" for a Follow-Me that will not engage, and ?diag now reports the latch on demand. (2) THE kCogHoldMs COG HOLD SURVIVES DEGRADATION. The fault term moved back BELOW the hold, to the site it occupied before. The hold serves a last-good GPS COURSE, and the latch is evidence about the COMPASS, so withdrawing a GPS-derived value on compass evidence was outside this guard's charter — and it cost real behaviour twice over: a COG dropout longer than 1.5 s failed FM's condition 6 and forced a stop-and-re-arm the hold would have bridged, and on the RTM side the documented cog_valid flicker in the 3-4 km/h approach band (rtm_target_speed_kmh 4.0 against rtm_cog_min_speed_kmh 3) alternated the steering override between centre and bearing at 10 Hz. Degraded now means precisely "mode 1 minus the compass", which is what the evidence supports. (3) THE CLEAR IS AS STRICT AS THE SET. Setting the fault needs at least four measured samples spanning 5 s of continuity; clearing it took ONE agreeing tick. For a MIRRORED module the reported heading is theta - h_true, so the gap is 2*h_true - theta, which passes below 45 deg in two 45-deg-wide windows per revolution — a rider coasting near one of them cleared the latch instantly with the module still mirrored, and that is exactly the fault ?magalign cannot detect. Agreement must now hold continuously for kHeadingDisagreeMs with measured samples no more than kHeadingDisagreeGapMaxMs apart, mirroring the set. No new config field, no threshold retuned, no confStruct change, sizeof stays 192, SW_VERSION stays 35.
// V2.5-Evo - 2026-08-17 - A HEADING DISAGREEMENT NOW DEGRADES THE SESSION INSTEAD OF REFUSING IT. WHAT SHIPPED BEFORE: a proven heading_disagree_fault withdrew the compass fallback (correct) and then refused to run on what was left (wrong). Gate 6 is enforced whenever usrConf.rtm_compass_required is set, which is the default, so on a stationary arm there was no heading at all, the gate raised rtm_rx_emergency_stop and the buggy sat dead with the throttle pinned at 0 — while gates 2-8 deliberately do NOT clear rtm_rx_active, so the TX went on displaying RTM as ACTIVE. Follow-Me had the twin of it: can_be_active carried !heading_disagree_fault, so FM sat in FM_ARMED for the rest of the session. The rider armed, nothing moved, and the remote said it had. WHAT IT DOES NOW: the COMPASS is the sensor that was caught lying; the GPS course is not, so refusing to run threw away a good source because a different one failed. A standing fault now degrades the session to COG-only — which is exactly usrConf.rtm_use_compass == 0, an existing, documented, supported mode — instead of refusing: getRtmHeading() takes the mode-0 path for heading SELECTION while the fault stands (live GPS course above rtm_cog_min_speed_kmh, nothing below it: no compass fallback and no kCogHoldMs hold, because mode 0 has neither), gate 6 is bypassed exactly as it is for a rider who has set rtm_compass_required = 0, FM's can_be_active and its fault-stop chain no longer carry the flag, and fm_flags bit 2 stops calling it armed-not-ready. The rider's experience becomes the ordinary COG-only one: get the buggy moving and the GPS course takes over. NOTHING IS WRITTEN TO usrConf — rtm_use_compass and rtm_compass_required are untouched, no SPIFFS write, no ?conf change; this is runtime degradation for the session only. THE COMPARISON ITSELF STILL RUNS IN THE CONFIGURED MODE (compare_possible keeps testing the stored mode, not the degraded one), because that measurement is the evidence route that clears the fault. AND BECAUSE THE FLAG NO LONGER BLOCKS ANYTHING, THE LATCH IS NOW STICKY: the disarm-edge clear in runRtmLoop() and the FM-idle clear in fmEnterIdle() are GONE — they forgave a proven fault with no evidence, so a mis-mounted compass had to be re-proven by coasting on every single run. Exactly three clear routes remain: a live measurement showing the two sources agreeing again, a SUCCESSFUL ?compasscal (CAL_FULL only) or ?magalign — the rider actually fixing the mounting, wired in Compass.ino — and a reboot. A PARTIAL or failed calibration does NOT clear it. The rider is told once on serial, the moment the degradation engages, what happened and how to fix it. Subtract-only throughout: the flag grants no heading, raises no cap, extends no engagement, and it now removes strictly MORE heading than before. No threshold retuned, no confStruct change, sizeof stays 192, SW_VERSION stays 35.
// V2.5-Evo - 2026-08-17 - The heading-disagree backstop threw away the only evidence it ever gets. THE EVIDENCE WINDOW: a comparison needs a live COG *and* a compass snapshot less than kHeadingCompareSnapMs (1000 ms) old, and updateCompassSnapshot() only refreshes the snapshot while thr_received < 25, because a snapshot taken under motor current is worthless. RTM and FM both require a HELD trigger. So about one second into any engagement the comparison goes dormant and the kHeadingDisagreeMs (5 s) dwell can never complete inside a run — the ONLY time both inputs exist together is when the rider is coasting with the trigger released above rtm_cog_min_speed_kmh, or during the arm ceremony, where the TX pins the throttle byte to zero. The previous pass then added an ARM-EDGE clear that wiped heading_disagree_fault at exactly that boundary, i.e. it discarded the freshest evidence available anywhere in the sequence and left the guard unable to act on anything it had proven. FIX: the arm edges (rtm_rx_active false -> true in runRtmLoop, and the FM_IDLE -> armed edge in runFmLoop) now clear only the UNFINISHED dwell (heading_disagree_since_ms, heading_disagree_last_seen_ms). A PROVEN fault survives into the engagement, where getRtmHeading() consumes it by withdrawing the compass fallback. The DISARM edges (rtm_rx_active true -> false, and a genuine transition into FM idle via fmEnterIdle) still clear all three, so this is still not a power-cycle latch, and a live measurement showing the two sources agreeing again still clears it outright. Because a surviving fault can now refuse an engagement, both refusals were made to SAY SO instead of being silent: RTM gate 6 prints the real cause (a compass caught disagreeing with GPS course) rather than the misleading generic "No valid heading source", and FM — which would otherwise sit in FM_ARMED with the TX reading "ready" — now raises fm_flags bit 2 (armed-not-ready) and prints a rate-limited explanation. Subtract-only throughout: the flag grants no heading, raises no cap and extends no engagement. No threshold retuned, no confStruct change, sizeof stays 192, SW_VERSION stays 35.
// V2.5-Evo - 2026-08-17 - SAFETY (release blocker): rtm_steer_override could survive an idle period and be applied on the first tick of the NEXT engagement. Only gate 1 (throttle released) and updateRtmSteering() ever wrote the neutral 127, so a GATE 9 HANDOFF WITH THE TRIGGER STILL HELD left the last bearing-derived value — say 210 — parked indefinitely: the inactive path in runRtmLoop() returned without touching it and fmEnterIdle() deliberately does not touch it either. calcPWM() applies the override whenever (rtm_rx_active || fm_rx_active) && rtm_rx_override_steering && thr_received >= 25, so on the next re-arm the 100 Hz generatePWM task could put that stale bearing on the motors as differential steering before runRtmLoop() recomputed it — with no gate 9 and no Phase C in the path. Normally a 100 ms window; UNBOUNDED if the re-arm lands while the loop task is frozen inside a deliberately non-abortable command (?gpssetup, ?wifiupd), since runRtmLoop() is then not running at all. FIX: runRtmLoop() now forces rtm_steer_override = 127 on every tick where BOTH rtm_rx_active and fm_rx_active are false. The !fm_rx_active term is load-bearing, not defensive — runRtmLoop() runs BEFORE runFmLoop() in loop(), so an unconditional reset would publish a neutral 127 for one preemption window per cycle throughout every Follow-Me engagement (RTM is inactive for all of one) and stutter FM steering to centre at 10 Hz; same hazard fm_throttle_cap has its own global to avoid. Subtract-only: writes the neutral value only, never a turn, never throttle. No confStruct change, sizeof stays 192, SW_VERSION stays 35.
// V2.5-Evo - 2026-08-16 - Follow-up to the edge-clear fix below; three items, all in the heading-disagreement guard. (1) THE LATCH NOW ALSO CLEARS AT THE ARM EDGE, not only at the disarm edge. runRtmLoop() calls getRtmHeading() unconditionally every 100 ms to fill the fm_status telemetry byte, so the disagreement dwell runs during ORDINARY MANUAL RIDING with nothing armed: a rotated or uncalibrated compass plus five seconds of coasting above rtm_cog_min_speed_kmh with the trigger released (snapshot still refreshing, COG live) was enough to latch the fault before the rider had armed anything. Once both clears became edges, nothing forgave that. The rider then declared FM, separation latched, and FM sat in FM_ARMED for the rest of the session - silently, because fm_flags bit 2 (armed-not-ready) DROPS once separation latches so the TX reads "ready", and the HEADING DISAGREE FAULT printf only runs when FM was already engaged. RTM had the same dead end by a different road: gates 2-8 set rtm_rx_emergency_stop WITHOUT clearing rtm_rx_active, so a TX that dies mid-tow leaves rtm_rx_active stuck true, the disarm edge never fires, and the "fresh agreement" clear is unreachable because the emergency stop pins throttle at 0, so the buggy cannot move, so COG never goes valid. Both are answered by clearing at the START of an engagement as well as the end - the rtm_rx_active false -> true edge in runRtmLoop(), and the FM_IDLE -> armed edge in runFmLoop(). This cannot weaken the guard: the flag only ever SUBTRACTS eligibility, and a genuine disagreement re-proves itself within kHeadingDisagreeMs (5 s) of the run starting, so the cost is at most 5 s of protection at the very beginning of an engagement. (2) THE 15 s EVIDENCE CAP BOUNDED THE OTHER DEFECT BUT DID NOT REMOVE IT: it measures the SPAN from the start of the dwell, not the age of the last measurement, so one disagreeing tick at t=0 and one more at t=14 s passed the cap and then satisfied "held continuously for 5 s" immediately - a fault built from two isolated one-tick samples fourteen seconds apart. Persistence is now required to be CONTINUOUS: consecutive measured disagreements no more than kHeadingDisagreeGapMaxMs (2 s) apart, so at least four samples must span the dwell. The freeze-don't-clear behaviour the short COG handover needs is untouched. (3) Comment-only: the prose in front of the COG hold claimed the function hands out no heading at all once the latch sets. It still serves a HELD COG at confidence 2 for up to kCogHoldMs, deliberately - the latch is evidence against the COMPASS, and a held COG is not the compass - so the comment now states what the code does instead of asserting more. No threshold retuned, no ordering changed, no confStruct change, sizeof stays 192, SW_VERSION stays 35.
// V2.5-Evo - 2026-08-16 - The heading-disagree backstop was INERT, and it was inert in the exact mode it was extended to protect. fmEnterIdle() zeroed heading_disagree_since_ms and heading_disagree_fault on every call, and runFmLoop() calls fmEnterIdle() on every 100 ms tick whenever FM is idle — a condition that INCLUDES rtm_rx_active. So for the whole duration of any RTM run the dwell timer and the latch were being wiped ten times a second: the kHeadingDisagreeMs (5 s) dwell could never accumulate, the fault could never be set, and the check that consumes it in getRtmHeading() (in front of the compass fallback) could never see anything. In RTM-only operation — the field case this was built for, a buggy that veered at close range and never came back — the advertised sticky backstop did nothing. FIX 1: the clear in fmEnterIdle() is now EDGE-triggered on a genuine transition into FM idle (a real end of an FM declaration), not re-applied every tick, so the latch survives across ticks WITHIN an engagement. FIX 2: RTM gained its own end-of-engagement clear at the top of runRtmLoop() — an edge on rtm_rx_active going true -> false — so a proven fault is forgiven when the rider disarms and cannot outlive the run that produced it (it is NOT a power-cycle latch). Deliberately an edge there too: clearing it at every tick RTM is inactive would recreate the same bug for Follow-Me, which only ever runs while RTM is inactive. FIX 3 (the freeze-age MEDIUM): the "no comparison possible" branch froze the dwell timestamp indefinitely, so two one-tick disagreements minutes apart escalated as one "sustained 5 s" fault; a part-finished proof older than kHeadingDisagreeEvidenceMaxMs (3x the dwell) is now discarded, while a PROVEN fault is untouched. Nothing here grants a heading, raises a cap or extends an engagement — the guard can still only subtract. cog_last_good_deg/ms are no longer static so Logger.ino can read (never write) them. No confStruct change, sizeof stays 192, SW_VERSION stays 35.
// V2.5-Evo - 2026-07-25 - STAGE 2 (RX heading-source trust guards, RTM + FM): the bench ?diag on the repaired board read "COG : 7.4 timestamp-updates/s vs 0.0 value-changes/s [value frozen 533 s]" — the GPS repeated ONE course seven times a second for nine minutes while gps_last_course_ms kept refreshing, so getRtmHeading()'s 1500 ms freshness gate passed the whole time and the ladder steered on a dead number, then fell back to the EMI-biased compass. That is the Follow-Me veer. GUARD 1: the COG branch now also requires the course VALUE to have moved within kRtmCogFrozenMs (3 s), read from the existing Stage 0 tracker g_diag_cog_change_ms — and it is SPEED-GATED to the same gps_last_speed_kmh >= rtm_cog_min_speed_kmh term the branch already used, so a stationary buggy (whose constant course is correct, not faulty) is never touched. A COG frozen WHILE MOVING additionally suppresses the mode-1 compass promotion: one source is provably dead and the other unverifiable, so we hold straight instead of steering on the survivor. GUARD 2: when a live COG and a simultaneous (< kHeadingCompareSnapMs) compass snapshot are both available in hybrid mode, a shortest-angular-distance gap > kHeadingDisagreeDeg (45) returns NO heading (confidence 0) so updateRtmSteering() holds straight; sustained past kHeadingDisagreeMs (5 s) it sets heading_disagree_fault, which joins the EXISTING FM fault chain alongside A3's diverge_fault (FM_STOPPING -> cap 0 -> ramp -> FM_IDLE, re-arm required) — no new state, no new exit path. Both guards can only REMOVE eligibility: no throttle cap is raised, no engagement extended, the deadman (thr_received < 25) is untouched. Logger.ino's inline getRtmHeading() duplicate is updated in lockstep so rtm_source/rtm_confidence keep telling the truth. All compile-time constants (shared, in BREmote_V2_Rx.h); no confStruct change, sizeof stays 184, SW_VERSION stays 34.
// V2.5-Evo - 2026-07-25 - F3-c (RX FM): the 8 m engage-distance floor guarded ONLY the manual fm_engage_dist_m value. The AUTO branch — kFmEngageFactor (1.5) x (min_dist_m + followme_smoothing_band_m) — used its product raw, and neither of those two SPIFFS fields has a lower bound, so a small tuning such as min_dist 1 + band 1 produced d_engage = 3 m: BELOW the measured 20 ft / 6.10 m tow rope, letting Follow-Me latch and engage with the rider still ON the rope. Same hazard the floor exists to prevent, reached through the other branch. FIX: each branch now only computes its candidate and ONE kFmEngageDistFloorM clamp is applied to the final d_engage regardless of origin. The clamp can only RAISE d_engage (engage later, never earlier), and it is a no-op at the owner's 4+2 tuning, which yields 9.0 m. Also swept the stale "6.7-7.6 m" tow-rope prose in this file to the measured 6.10 m. No confStruct change, sizeof stays 184, SW_VERSION stays 34.
// V2.5-Evo - 2026-07-25 - F3-b (RX FM; comment + shared-constant move, no behaviour change): kFmEngageDistFloorM is no longer DEFINED in this file. It now has exactly one definition, in BREmote_V2_Rx.h, raised 5.0 -> 8.0 m — 5.0 m sat below the tow rope it exists to clear (the owner's rope is 20 ft = 6.10 m), so a manual fm_engage_dist_m of 5.0-6.1 m was legal and let FM engage with the rider still ON the rope. ConfigService.ino's duplicate bare 5.0f literal is gone with it; both the validator and the read-site clamp in runFmLoop() now reference the one shared constant. The clamp itself is unchanged in shape — legacy stored values still clamp UP to the floor, now 8.0 m. No confStruct change, sizeof stays 184, SW_VERSION stays 34.
// V2.5-Evo - 2026-07-25 - Batch A follow-up (Rex A3 NO-GO: F1/F3/F5/F7), RX FM only. (F1) the A3 divergence detector was a BARE THRESHOLD on a 3000 ms dwell while the engage ramp is 3500 ms, so it could fire BEFORE the ramp even finished and aborted ordinary engagements (at engage, dist_m is typically 13-21 m against an 18 m ceiling, and the align cap of 13/255 means the gap GROWS first). It now mirrors runPhaseC()'s actual shape: the distance at dwell start is captured (fm_diverge_start_dist_m) and the fault is raised at dwell expiry ONLY if the buggy is not closing (dist_m >= start - kFmDivergeCloseEpsM 2.0 m); if it has closed by more than that it IS following, just far, so the timer clears and no fault fires. Plus an engage grace: the detector is skipped and its dwell parked for kFmEngageRampMs + kFmDivergeMs (6.5 s) after every engagement so the buggy is allowed to ramp and align before it is judged. (F3) fm_engage_dist_m gained a 5 m floor — a stored value of, say, 3 m IS the engage distance in metres and is SHORTER than the 6.7-7.6 m tow rope, which defeated the separation interlock entirely; cfgValidateCrossField() now accepts only 0 (auto) or >= 5.0 m, and the use site clamps defensively so a pre-existing stored value cannot slip through. (F5) corrected the A2 comment that claimed d_engage feeds the distance Schmitt hysteresis - it does not; the Schmitt uses min_dist / min_dist+band. (F7) the divergence Serial.printf now runs AFTER fm_throttle_cap = 0 so UART backpressure can never defer the hard stop. All compile-time constants; no confStruct change, sizeof stays 184, SW_VERSION stays 34.
// V2.5-Evo - 2026-07-25 - Batch A (A2+A3), RX FM only. (A2) fm_engage_dist_m is now READ: >0 sets the FM engage distance directly in metres (rope x ~1.15), 0 keeps the previous auto behaviour (kFmEngageFactor x d_follow) bit-for-bit; latch, dwell and Schmitt hysteresis untouched. (A3) FM divergence FAULT — runFmLoop() gained the upper distance bound it never had: condition 8 is a lower bound only, and runPhaseC()'s convergence check is RTM-only (called from runRtmLoop, never runFmLoop), so a wrong heading let FM steer away indefinitely. dist_m > kFmDivergeFactor(2.0) x D_engage sustained kFmDivergeMs(3000) while FM_ACTIVE now routes through the EXISTING fault path (FM_STOPPING ramp -> FM_IDLE, re-arm required, same haptic/St semantics as conditions 2-7). Non-blocking first-exceed timestamp, cleared on any condition/state/data-trust change. Subtract-only: adds no throttle, extends no engagement, does not touch the deadman. All compile-time constants; no confStruct change; SW_VERSION stays 34.
// V2.5-Evo - 2026-07-19 - P3 FM (DESIGN_FOLLOW_ME.md sections 4-7): Follow-Me autonomous following. Adds runFmLoop() 10Hz state machine (IDLE/ARMED/ACTIVE/DEMOTED incl. the missing 0xFF->usrConf.followme_mode fallback — SUPERSEDED 2026-07-20, see R0 below), all 9 activation/hold conditions with Schmitt hysteresis on distance and side-zone, the lag-anchor trailing target-point geometry, and the 5-stage subtract-only throttle cap chain. Reuses the existing EMA filter / P+D / heading ladder / authority / wrap pipeline unchanged - updateRtmSteering() only gains a target selector (RTM = rider position, FM = trailing point). telemetry.fm_status bit0 now reports FM engaged rather than FM mode selected. No confStruct change; SW_VERSION stays 33.
// V2.5-Evo - 2026-07-20 - FM engagement semantics (R0/R1/R2): (R0) BOTH 0xFF->usrConf.followme_mode fallbacks removed — 0xFF now means FM_IDLE always, killing the latently-armed factory boot; usrConf.followme_mode is the TX arm-gesture seed only. (R1) separation latch: FM's FIRST entry into ACTIVE now also requires dist > kFmEngageFactor(1.5) x d_follow sustained kFmSepDwellMs(2000) — the tow rope (6.7-7.6 m) is longer than the old engage distance, so FM could engage mid-tow; existing Schmitt hysteresis governs after the latch sets. (R2) two clears originally included a throttle-release latch clear. [SUPERSEDED 2026-08-26: release now preserves the latch; explicit disarm/idle remains the session boundary.] No 0xF2 refresh for kFmModeAgeMs(95 s) -> FM_IDLE. P3 geometry/cap/steering untouched. No confStruct change; SW_VERSION stays 33.
// V2.5-Evo - 2026-07-20 - FM control "brain" (Fable v1.4): (A) holds-vs-faults — condition 1=DEADMAN (throttle, never a fault), 8/9=HOLD (cap 0, stays ARMED, auto-resume, +kFmSpeedHystKmh speed hysteresis), 2-7=FAULT (FM_STOPPING ramp 0->255 over kFmStopRampMs -> FM_IDLE, re-arm required); heading loss (cond 6) is now ALWAYS a fault regardless of rtm_compass_required. (C) originally implemented steer-cancel while ACTIVE. [SUPERSEDED 2026-08-26: manual steering temporarily wins without changing FM state or latch.] (D) fm_flags telemetry byte (repurposed reserved_tx_imu): armed/engaged/armed-not-ready/fault-stop-sticky(kFmFaultStickyMs). FM_DEMOTED renamed FM_HOLD; FM_STOPPING added. All compile-time constants; no confStruct change; SW_VERSION stays 33.
// V2.5-Evo - 2026-07-19 - Rex hardening: reset D-term continuity statics (prev_heading_src_valid/prev_heading_error_deg/prev_steering_update_ms) in the override-disabled early return so an off->on toggle can't differentiate a stale error across the gap
// V2.5-Evo - 2026-07-19 - FM triage (Fable audit §5): (1) no-fix engagement guard — getRtmHeading() returns confidence 0 unless a fresh RX GPS fix exists, so RTM/FM cannot report confidence 2 or engage with datetime_unix=0; (2) D-term differentiated only across consecutive same heading-source samples — skip the step on a source switch (COG<->compass) or a compass-snapshot re-snap to kill the ±300°/s Kd spikes
// V2.5-Evo - 2026-05-22 - SW32: Two-phase RTM throttle — align phase suppresses throttle until heading < rtm_align_threshold_deg; run phase GPS speed governor
// V2.5-Evo - 2026-05-11 - Phase C fix: VESC ERPM check now verifies data freshness via vesc.last_packet before comparing to GPS speed
// V2.5-Evo - 2026-05-08 - Bundle 1: P+D+filter steering controller; preset table; bearing filter for FM path-following
// V2.5-Evo - 2026-05-06 - D5: getRtmHeading() layered heading source; updateRtmSteering() rewritten; Gate 6 accepts any source; updateCompassSnapshot() called from runRtmLoop top
// V2.5-Evo - 2026-05-03 - C1/M2 audit fix: gps_tx_ok uses timestamp age on both paths; 0.0 lat/lng sentinel removed
// V2.5-Evo - 2026-05-01 - Fix D: gps_tx_ok relaxed for FM/idle; never reset rtm_distance to 0xFF when RTM inactive
// V2.5-Evo - 2026-05-01 - Fix C: FM bar keep-last-known on GPS dropout; only 0xFF if TX GPS never received
// V2.5-Evo - 2026-05-01 - Fix B: encode rtm_distance always when GPS valid; feeds FM bar and enables correct pre-arm block within stop distance
// V2.5-Evo - 2026-04-30 - Gate 9 clean disengagement (handoff to manual, no emergency stop); re-arm fix (0xFF when inactive); approach decel zone computation
// V2.5-Evo - 2026-04-25 - P7: RX RTM state machine, 10 safety gates, Phase C anti-spoofing.
// V2.5-Evo - 2026-04-27 - P8: runRtmLoop() encodes RX→TX distance into telemetry.rtm_distance (index 5)
// V2.5-Evo - 2026-04-28 - P9 Bug1A/1B/1C: Gate9 zero-guard; always-compute dist before gates
// V2.5-Evo - 2026-04-28 - Security: Gate 1 resets rtm_steer_override=127 on throttle release
// V2.5-Evo - 2026-04-29 - Fix 6-1: Gate 4 + Phase C check 3 now use
//   usrConf.tx_gps_stale_timeout_ms instead of hardcoded 2000ms
// V2.5-Evo - 2026-04-29 - Fix 6-2: runRtmLoop() revokes gps_phase_b_ok
//   when TX GPS age exceeds 2× tx_gps_stale_timeout_ms
//
// The RTM state machine runs in loop() at ~10Hz (100ms rate-limit).
// When rtm_rx_active is set true by a 0xF1 meta-packet, this module:
//   1. Checks all 10 safety gates every iteration (any fail → emergency stop).
//   2. Computes compass bearing toward TX GPS position.
//   3. Converts bearing error to a steering override (0-255, 127=straight).
//   4. Runs Phase C: convergence check, VESC ERPM speed check, TX GPS freshness.
//
// All outputs are written to volatile globals read by calcPWM() and triggeredReceive().

extern bool gps_phase_b_ok;   // V2.5-Evo - P7 fix: defined in Radio.ino (Phase B section)
// V2.5-Evo - 2026-05-06 - D5: extern declarations for D1+D2 capture globals.
extern float         gps_last_course_deg;       // From GPS.ino (D1) — last valid GPS course-over-ground (0-360 deg, -1.0 if none)
extern unsigned long gps_last_course_ms;        // From GPS.ino (D1) — millis() of last course update (0 if none)
extern float         compass_snapshot_heading;  // From Compass.ino (D2) — clean compass heading captured during motor-idle (0-360 deg, -1.0 if none)
extern unsigned long compass_snapshot_ms;       // From Compass.ino (D2) — millis() of snapshot capture (0 if none)
extern void          updateCompassSnapshot();   // From Compass.ino (D2) — captures clean compass heading when motor idle
// ============================================================
// RTM/FM STEERING CONTROLLER PRESETS — Bundle 1 (2026-05-08)
//
// PD-style controller: output = Kp * clamped_error + Kd * d(error)/dt
// Plus a low-pass filter on TARGET POSITION (lat/lng) for FM path-following
// — surfer's high-frequency bottom turns are smoothed out, buggy follows
// the surfer's path rather than chasing every wobble.
//
// For RTM (TX stationary), filter τ is set very low so behavior is essentially
// unfiltered — the filter doesn't hurt because there's nothing to smooth.
//
// 5 presets cover flat-water-to-heavy-surf range. Operator picks via WebUI
// before each session based on conditions. Default = Normal (index 2).
// ============================================================
struct SteerPreset {
  float error_clamp_deg;     // Saturation: heading error clamped to ±this before P/D math.
  float kp;                  // Proportional gain (PID Kp). 1.0 = baseline.
  float kd;                  // Derivative gain (PID Kd). 0.0 disables D term entirely.
  float target_filter_tau_s; // Low-pass filter time constant on target position (seconds).
                             // 0.5 ≈ no smoothing for RTM; 1-5 path-following for FM.
};

static const SteerPreset kSteerPresets[5] = {
  // {clamp,    Kp,   Kd,   tau_s }
  {  150.0f,  0.70f, 0.50f, 5.00f },  // 0 Very Soft   — heavy surf, aggressive surfer
  {  120.0f,  0.85f, 0.40f, 3.00f },  // 1 Soft        — choppy normal session
  {   90.0f,  1.00f, 0.30f, 2.00f },  // 2 Normal      — DEFAULT, mixed conditions
  {   60.0f,  1.20f, 0.20f, 1.00f },  // 3 Sharp       — calm water, RC use
  {   45.0f,  1.40f, 0.10f, 0.50f },  // 4 Very Sharp  — glass-flat, no waves
};

// ---- Bundle 1 module-level state for P+D controller and bearing filter ----
static float         prev_heading_error_deg    = 0.0f;
static unsigned long prev_steering_update_ms   = 0;
// D-term source-continuity tracking (2026-07-19 FM triage). We only differentiate
// heading_error across two samples that came from the SAME continuous heading source.
// prev_heading_src_id is a discriminator that changes on a source switch (COG<->compass)
// AND on a compass-snapshot re-snap; prev_heading_src_valid gates the very first sample.
static uint32_t      prev_heading_src_id       = 0;
static bool          prev_heading_src_valid    = false;

// Non-static globals exported to Logger.ino via extern (Bundle 1 tuning telemetry).
// 0x7FFF is the "no data" sentinel (non-zero).
int16_t g_heading_error_dx10 = 0x7FFF;  // Last heading error × 10 deg; 0x7FFF = no data
int16_t g_d_error_dx10       = 0x7FFF;  // Last derivative × 10 deg/s; 0x7FFF = no data

// V2.5-Evo - 2026-08-17 - Read-only accessor for the heading-disagreement latch, forward-declared
// here so gate 6 below can ask whether the compass has been withdrawn from the heading ladder —
// while it has, that gate stands aside instead of refusing the run (see the note at the gate).
// Two other readers use the same accessor and neither is in the control path: Logger.ino's heading
// mirror, and (V2.5-Evo - 2026-08-17) the ?diag line in System.ino that lets a rider ask the board
// whether the latch is standing.
// WHY AN ACCESSOR AND NOT THE FLAG ITSELF: heading_disagree_fault is a file-scope static declared
// ~170 lines further down, alongside the long block that documents it. A variable is not visible
// before its declaration — unlike a function, which the Arduino builder auto-prototypes — and
// moving the state up here would separate it from its documentation for no benefit. The accessor
// is defined immediately after the flag. It only reads: it cannot set, clear or age anything.
static bool headingDisagreeLatched();


// ============================================================
// V2.5-Evo - 2026-07-25 - STAGE 2: HEADING-DISAGREEMENT STATE (guard 2)
//
// Two module-private timers. They are NOT confStruct fields, NOT telemetry, NOT logged as new
// columns — a guard firing is already visible in an existing level-4 log through rtm_source,
// rtm_confidence and cog_frozen_s.
//
// heading_disagree_since_ms : millis() when the COG-vs-compass gap first exceeded
//   kHeadingDisagreeDeg and has held it continuously since; 0 = not currently disagreeing, or no
//   comparison was possible this tick. Same first-exceed-timestamp shape as fm_sep_over_since_ms
//   and fm_diverge_since_ms: plain millis() bookkeeping, no delay, no spin.
//
// heading_disagree_fault : set once that gap has persisted for kHeadingDisagreeMs. A transient
//   disagreement records evidence but does not veto a valid GPS course. A sustained one withdraws
//   the COMPASS (the source this hardware can bias under motor current).
//
//   V2.5-Evo - 2026-08-27 - WHAT IT DOES NOW: RTM AND FOLLOW-ME DEGRADE TO GPS COG. While it stands
//   the compass is withdrawn from the ladder, so both run on GPS course alone; it is NOT a config
//   change (nothing writes rtm_use_compass or rtm_compass_required; the rider's stored settings are
//   exactly as they left them). The separate fault verdict is persisted across reboot. Concretely,
//   while the flag stands:
//     - getRtmHeading() withdraws the COMPASS and nothing else. Above rtm_cog_min_speed_kmh the
//       live GPS course is served unchanged at confidence 3, and the kCogHoldMs held-COG bridge is
//       still served at confidence 2 across a short dropout. V2.5-Evo - 2026-08-17: the hold used
//       to be withdrawn here too, on a consistency argument ("the hold is a mode-1 feature"); it is
//       back, because the hold serves a GPS COURSE and this latch is evidence about the COMPASS.
//       Degraded therefore means exactly "mode 1 minus the compass", which is all the evidence
//       supports — see the check below the hold in getRtmHeading();
//     - RTM gate 6 stands aside exactly as it does for a rider who set rtm_compass_required = 0,
//       so an arm can never be refused for lack of a heading — the buggy starts, holds straight,
//       and picks up a course as soon as it is moving. RTM's degraded behaviour is BOUNDED: with no
//       heading the override is pinned at 127 and the align cap holds the throttle at 13/255;
//     - FOLLOW-ME uses the same degraded ladder. A live GPS COG (confidence 3) or its short hold
//       (confidence 2) satisfies condition 6 and may engage/continue FM. The latch itself is not an
//       engagement gate or a fault-stop cause. If COG is absent, stale or frozen after the hold,
//       condition 6 still fails: withdrawing the compass never invents a heading;
//     - mode 2 (compass-only, diagnostic) is the one place there is nothing to fall back to, so the
//       proven-bad compass is withheld and the function returns no heading at all. See there.
//
//   WHERE IT IS CLEARED — EXACTLY TWO ROUTES, and both are evidence that the problem is gone. The
//   engagement-boundary clears that used to exist (fmEnterIdle(), the RTM disarm edge, the arm
//   edges) are GONE. They existed only because a
//   standing fault could refuse an RTM arm and would otherwise have been un-clearable on the water;
//   RTM now degrades instead of refusing, so that reason is gone with them — and forgiving a fault
//   at every disarm meant a compass mounted 90 deg out had to be re-proven by coasting on every
//   single run, which is precisely the gap this closes. A mis-mounted compass is wrong every run.
//     1. A SUSTAINED MEASUREMENT SHOWING AGREEMENT — here, in the compare branch below. The
//        evidence is withdrawn, so the verdict goes with it. This still runs while the fault
//        stands, because compare_possible deliberately tests the CONFIGURED mode, not the degraded
//        one; without that, withdrawing the compass would have switched off the very measurement
//        that exonerates it.
//        V2.5-Evo - 2026-08-17 - AND IT NOW TAKES AS MUCH EVIDENCE TO CLEAR AS TO SET. This route
//        used to fire on the FIRST tick where a comparison was possible and the two agreed: four
//        samples over five continuous seconds to convict, one sample to acquit. That asymmetry
//        broke on the case the latch matters most for. A MIRRORED module (an axis sign flipped by
//        the mounting) reports theta - h_true instead of h_true, so the gap between compass and
//        course is 2*h_true - theta, which sweeps the whole circle as the craft turns and passes
//        below kHeadingDisagreeDeg inside two 45-deg-wide windows per revolution. A rider coasting
//        on a heading near one of those windows cleared the latch instantly with the module still
//        mirrored — and mirroring is exactly the fault ?magalign cannot detect, so this route was
//        acquitting the one defect stickiness exists for. Agreement must now be measured
//        CONTINUOUSLY for kHeadingDisagreeMs, with consecutive measured samples no more than
//        kHeadingDisagreeGapMaxMs apart (so at least four of them, the same shape as the set), and
//        that dwell is tracked by heading_agree_since_ms / heading_agree_last_seen_ms below. Any
//        measured disagreement restarts it from zero. Making a clear HARDER can only ever leave a
//        proven fault standing for longer, which is the conservative direction for this guard.
//     2. A SUCCESSFUL ?compasscal (CAL_FULL only) OR ?magalign — the rider physically fixing the
//        mounting and re-measuring it. Wired in Compass.ino via headingDisagreeClearAfterCal();
//        a PARTIAL cal (iron saved, orientation NOT re-measured) and any failed/aborted run do NOT
//        clear it, because those are exactly the runs that leave the old mounting angle in place.
//   And separately, the part-finished dwell (not the fault) ages out after
//   kHeadingDisagreeEvidenceMaxMs while no comparison is possible, is restarted from the current
//   sample if the measured disagreements themselves stop being contiguous (kHeadingDisagreeGapMaxMs),
//   and is dropped at both edges of an engagement so a half-built proof never spans two situations.
//   V2.5-Evo - 2026-08-17 - the AGREEMENT dwell added for route 1 obeys the same two bounds, for the
//   same reasons: it ages out after kHeadingDisagreeEvidenceMaxMs while nothing is being measured,
//   and a gap longer than kHeadingDisagreeGapMaxMs between agreeing samples restarts it. It is NOT
//   dropped at engagement edges, and it does not need to be — the gap bound is the tighter test.
//   The measurement goes dormant about a second into any engagement (the snapshot stops refreshing
//   under motor current), so an agreement dwell physically cannot straddle an engagement: the next
//   agreeing sample after the run arrives far more than kHeadingDisagreeGapMaxMs late and opens a
//   fresh dwell of its own.
//
//   V2.5-Evo - 2026-08-17 - WHY THE ENGAGEMENT-BOUNDARY CLEARS WENT, IN FULL, because the argument
//   that put them there is the argument that now takes them out. They were added when a proven
//   fault could REFUSE an engagement: at that point a sticky latch risked becoming an un-clearable
//   arming block on the water, so every engagement boundary forgave it. The evidence window is
//   narrow — a comparison needs a live COG AND a compass snapshot younger than kHeadingCompareSnapMs,
//   and Compass.ino only refreshes that snapshot while thr_received < 25, so the only moments the
//   two inputs coexist are a rider COASTING with the trigger released above rtm_cog_min_speed_kmh
//   and the arm ceremony itself — which meant those boundary clears were throwing away the only
//   evidence the guard ever gets, and the rider had to re-prove a bad compass by coasting on every
//   single run. The reason to keep the verdict is intact: a compass mounted 90 deg out is 90 deg
//   out on the next run too.
//   V2.5-Evo - 2026-08-27 - FOLLOW-ME ALSO USES GPS-ONLY DEGRADATION. The latch remains across state
//   boundaries to keep the suspect compass withdrawn, but it is not an engagement condition. Live
//   or briefly held GPS COG remains eligible; a genuine absence of both still fails condition 6.
//
//   WHY THE FLAG IS NEEDED AT ALL: the
//   comparison window is narrow by necessity (see kHeadingCompareSnapMs). The compass snapshot
//   stops refreshing the moment the rider squeezes the trigger, so a real disagreement can be
//   measurable on one tick and unmeasurable on the next. The flag is what keeps the compass
//   withdrawn after that evidence window closes; it deliberately does not withdraw GPS COG.
//
//   RTM CONSUMES IT TOO (this paragraph used to say only FM did). The 2026-08-16 close-range veer
//   changed that: getRtmHeading() itself withdraws the compass while the fault is latched, so every
//   caller inherits it — RTM, FM and anything added later. The original objection was that a sticky
//   fault could become an un-clearable arming block on the water. V2.5-Evo - 2026-08-17: that
//   objection is answered at the root instead of by forgetting the evidence — the fault cannot
//   block an arm any more: RTM and Follow-Me both use a valid live/held GPS course while the latch
//   stands. If that course is unavailable, FM still fails its ordinary valid-heading condition.
//   The way out of degradation is unchanged: fix the compass (?compasscal / ?magalign), or let a
//   sustained measurement show the two sources agreeing again.
//
// CONCURRENCY: every caller of getRtmHeading() (gate 6, checkFmFaultConditions, the fm_status
// telemetry block, updateRtmSteering) runs in the loop task, so these statics are single-writer.
// Logger.ino's duplicate of this logic runs in loggerTask and READS the fault through the
// headingDisagreeLatched() accessor only — a read that cannot set, clear or age anything — so that
// mirror stays side-effect-free while still describing the same heading the controller chose.
// Compass.ino calls headingDisagreeClearAfterCal() from the ?compasscal / ?magalign commands, which
// run in the loop task as well (System.ino dispatches them there), so the single-writer property
// holds for that clear route too.
// ============================================================
static unsigned long heading_disagree_since_ms = 0;
// V2.5-Evo - 2026-08-17 - volatile. It is written only by the loop task (single-writer, as the
// CONCURRENCY note above says) but it is READ from loggerTask, through headingDisagreeLatched().
// As a file-scope static whose address never escapes this translation unit, the compiler is free to
// keep it in a register across a loop and never re-read the memory the other task can see change.
// That read is log columns only — it decides nothing, caps nothing, gates nothing — so this is a
// correctness tidy on a diagnostic path, not a control fix. Costs one memory access per read.
static volatile bool heading_disagree_fault    = false;

// V2.5-Evo - 2026-08-16 - millis() of the most recent tick on which a disagreement was actually
// MEASURED — as opposed to merely still standing on record. 0 = nothing measured. It is only
// meaningful while heading_disagree_since_ms is non-zero, and it is cleared everywhere that is,
// so the two can never disagree about whether a dwell is running. See kHeadingDisagreeGapMaxMs
// below for what it is for and why the dwell needed a second, finer bound than the span cap.
static unsigned long heading_disagree_last_seen_ms = 0;

// V2.5-Evo - 2026-08-17 - THE MIRROR IMAGE OF THE TWO ABOVE, for clear route 1.
//
// heading_agree_since_ms : millis() when the two sources were first measured AGREEING and have
//   kept agreeing continuously since; 0 = not currently in an agreement dwell.
// heading_agree_last_seen_ms : millis() of the most recent tick on which agreement was actually
//   MEASURED. Only meaningful while heading_agree_since_ms is non-zero, and cleared everywhere
//   that is, so the pair can never disagree about whether a dwell is running — the same invariant
//   the disagreement pair keeps.
//
// WHY THEY EXIST. Convicting the compass needs kHeadingDisagreeMs of continuous measured
// disagreement, which is at least four samples; acquitting it used to need one agreeing tick. That
// let a mirrored module walk free every time the craft's heading wandered into one of the two
// windows per revolution where a mirrored reading happens to land within kHeadingDisagreeDeg of the
// true course. Symmetric evidence, symmetric bookkeeping: same dwell length, same continuity bound,
// same ageing rule. No new constant, no confStruct field, no SPIFFS slot, no web-UI row.
//
// Single-writer from the loop task, exactly like the pair above: only getRtmHeading() writes them,
// plus headingDisagreeClearAfterCal() which resets the whole set together.
static unsigned long heading_agree_since_ms     = 0;
static unsigned long heading_agree_last_seen_ms = 0;

// ------------------------------------------------------------
// headingDisagreeLatched - has a heading disagreement been PROVEN and not yet withdrawn?
// ------------------------------------------------------------
// V2.5-Evo - 2026-08-17. Inputs: none. Returns: the current value of heading_disagree_fault.
// Side effects: NONE — it does not set, clear or age the latch or its dwell.
//
// It exists so checkRtmSafetyGates(), which is defined above this declaration, can ask whether the
// session is degraded — gate 6 stands aside while it is. Forward-declared next to that gate; see
// the note there. Logger.ino reads it through the same accessor for the log mirror.
// ------------------------------------------------------------
static bool headingDisagreeLatched()
{
  return heading_disagree_fault;
}

// ============================================================================================
// Latch persistence - the verdict survives a power cycle
// ============================================================================================
// V2.5-Evo - 2026-08-18 - LATCH-1. heading_disagree_fault lived only in RAM, so a power cycle
// forgot it. A rider whose compass is genuinely mis-mounted saw the warning, switched off,
// switched on, and Follow-Me would engage again on the compass that had just been caught
// disagreeing with GPS course. The safety check was defeated by turning it off and on again,
// which is the first thing anyone does when something misbehaves.
//
// STORED AS A MARKER FILE, NOT A confStruct FIELD. Adding a field changes sizeof(confStruct),
// which bumps SW_VERSION, which wipes every owner's stored config on the next flash - an
// enormous cost for one bit. The file's EXISTENCE is the latch; its contents are only there so
// the boot log can say when and why.
//
// WHY PERSISTING IS NOT HARSH. The latch already clears on FIVE SECONDS OF MEASURED AGREEMENT,
// with no rider action at all. So a false positive - passing a steel bridge, a transient - heals
// by itself the next time the buggy coasts with the throttle released and the two sources agree.
// Persisting costs nothing to the innocent case and closes the power-cycle hole in the guilty
// one. The other two escapes, a FULL ?compasscal and a completed ?magalign, are unchanged.
//
// FAIL-SAFE DIRECTION: a SPIFFS write that fails leaves the fault set in RAM for this session,
// which is the conservative outcome. It is never treated as a clear.
#define HDG_FAULT_FILE "/hdg_fault"

static void headingDisagreePersist(bool latched)
{
  if (latched) {
    File f = SPIFFS.open(HDG_FAULT_FILE, FILE_WRITE);
    if (!f) {
      // Not fatal and deliberately not escalated: the fault still stands for THIS session, which
      // is the safe direction. Only the survival across a reboot is lost.
      Serial.println("HEADING [RX] WARNING: could not persist the disagreement latch to SPIFFS. "
                     "It still applies now, but will not survive a reboot.");
      return;
    }
    f.printf("heading_disagree_fault at %lu ms uptime\n", (unsigned long)millis());
    f.close();
  } else {
    if (SPIFFS.exists(HDG_FAULT_FILE)) SPIFFS.remove(HDG_FAULT_FILE);
  }
}

// Called ONCE from setup(), after SPIFFS is mounted and before RTM/FM can arm.
static void headingDisagreeRestore()
{
  if (!SPIFFS.exists(HDG_FAULT_FILE)) return;

  // ==========================================================================================
  // V2.5-Evo - 2026-08-20 - LATCH-2. ONLY RESTORE A VERDICT THE RIDER CAN STILL CLEAR.
  //
  // Found by Rex auditing LATCH-1 (shipped the same day, 2026-08-18). LATCH-1 made the latch
  // survive a reboot, and in doing so removed clear route 3 — the reboot itself. That is correct
  // in mode 1 with a compass fitted, where routes 1 and 2 both work. It is a TRAP everywhere
  // else, because BOTH remaining routes need a working compass in hybrid mode:
  //
  //   Route 1, five seconds of measured agreement, runs only inside compare_possible, which
  //   begins `(mode == 1) && cog_valid && ...` — so on a COG-only board it can NEVER fire.
  //   Route 2, ?compasscal / ?magalign, needs a compass that answers.
  //
  // So a rider who latched the fault in hybrid mode and then set rtm_use_compass 0, or whose
  // compass has since failed or been unplugged, would boot with Follow-Me blocked and NO route
  // out of it — permanently, on every subsequent boot, with the only escape being a compass they
  // no longer use or no longer have. Unrecoverable in the field is worse than the risk the latch
  // was guarding against, and LATCH-1 created it.
  //
  // The stale marker is DELETED rather than left in place, so the trap cannot re-arm itself on
  // the next boot. If the rider returns to hybrid mode with a working compass, a genuine
  // disagreement re-proves itself within kHeadingDisagreeMs of coasting — the evidence is cheap
  // to regather, which is exactly why it is safe to drop a verdict that can no longer be tested.
  //
  // TRADE-OFF, STATED PLAINLY: a rider could dodge a legitimate latch by setting
  // rtm_use_compass 0. That is not a new hole. Rex confirmed a mode-0 board cannot set the latch
  // in the first place, and cfgValidateCrossField() already force-zeroes rtm_compass_required for
  // that rider class — COG-only is a deliberate, documented, supported configuration, and a rider
  // who selects it has explicitly said the compass is not in use. This removes an unrecoverable
  // state; it does not create a bypass that was not already there by design.
  // ==========================================================================================
  if (usrConf.rtm_use_compass != 1 || !compass_detected) {
    SPIFFS.remove(HDG_FAULT_FILE);
    Serial.printf("HEADING [RX] a stored heading-disagreement verdict was found but DISCARDED: "
                  "%s, so neither route that could clear it is available. Follow-Me is not "
                  "blocked.\n",
                  (usrConf.rtm_use_compass != 1) ? "this board is configured COG-only "
                                                   "(rtm_use_compass 0)"
                                                 : "no compass is detected");
    return;
  }

  heading_disagree_fault = true;
  // heading_degrade_announced is deliberately left false, so the standing degradation notice is
  // printed once on this boot too. A rider who power-cycled needs telling again, not silence.

  Serial.println("HEADING [RX] a heading disagreement was PROVEN before the last reboot and has "
                 "not been withdrawn. The compass stays excluded; Follow-Me and FM Return may "
                 "use a valid GPS course.");
  Serial.println("HEADING [RX] it clears on its own after 5 s of the compass and GPS course "
                 "measured agreeing while coasting, or immediately after a full ?compasscal or "
                 "a completed ?magalign.");
}

// V2.5-Evo - 2026-08-17 - true once the rider has been TOLD, on serial, that this session has been
// degraded to GPS-course-only. Reset by every clear route, so a fault that latches again later
// announces itself again. Notification bookkeeping only: nothing reads it as a control input.
static bool heading_degrade_announced = false;

// ------------------------------------------------------------
// headingDisagreeAnnounceDegraded - tell the rider, once, that the compass has been withdrawn
// ------------------------------------------------------------
// V2.5-Evo - 2026-08-17. Inputs: none. Outputs: none. Side effects: one Serial block, and it sets
// heading_degrade_announced so it cannot repeat.
//
// WHY IT EXISTS. The degradation is otherwise invisible from the bench: the buggy still arms, still
// runs, and simply has no heading until it is moving faster than rtm_cog_min_speed_kmh. A rider who
// does not know why deserves to be told what happened, what the craft will do now, and how to fix
// it — so the message says all three rather than naming a fault code.
//
// RATE LIMIT. It prints on the transition into the degraded state and never again while that state
// stands, which is the strongest rate limit available: the caller sits inside a 10 Hz ladder that
// is itself called several times per tick, so anything weaker would flood the UART. The flag is
// re-armed only when the fault is genuinely cleared (agreement, calibration).
//
// It still waits for trigger release. Although the latch itself no longer stops FM, withdrawing the
// compass can expose a simultaneous no-COG condition that does. Deferring diagnostic output keeps a
// potentially blocking Serial write out of every motor-live path; ?diag remains available on demand.
//
// V2.5-Evo - 2026-08-17 - AND THE RETRY IS NOW WHERE IT HAS TO BE FOR THAT SENTENCE TO BE TRUE.
// "Prints on the first tick after the rider lets go" assumed something would call this again on
// that tick. Nothing did: the only caller was the if (disagree_now) branch in getRtmHeading(), so a
// retry required a fresh MEASURED disagreement — which needs a compass snapshot that only refreshes
// with the trigger released, so a rider who stayed on the throttle for the rest of the session
// never produced one and the notice was LOST, not deferred. getRtmHeading() now calls this
// unconditionally at its top while the fault stands, so the retry happens on the first released-
// trigger tick regardless of what is measurable. The deferral above is unchanged.
//
// It writes NO control variable: not a cap, not a gate, not the override, not usrConf.
// ------------------------------------------------------------
static void headingDisagreeAnnounceDegraded()
{
  if (heading_degrade_announced) return;
  if (thr_received >= 25) return;   // F7: never print while the motor can be live — retry on release
  heading_degrade_announced = true;

  Serial.printf("HEADING [RX] COMPASS WITHDRAWN: the compass disagreed with the GPS course by more than %.0f deg for %lu ms.\n",
                (double)kHeadingDisagreeDeg, (unsigned long)kHeadingDisagreeMs);
  Serial.println("        Follow-Me and FM Return now steer on GPS COURSE ONLY. Your stored settings are UNCHANGED.");
  Serial.println("        FOLLOW-ME MAY ENGAGE with a valid live or briefly held GPS course; the disagreement latch itself is not an engagement gate.");
  Serial.printf("        Expect NO heading, and straight-ahead steering, whenever the buggy is slower than %u km/h and the last good course is more than 3 s old.\n",
                (unsigned)usrConf.rtm_cog_min_speed_kmh);
  Serial.println("        To get the compass back: re-run ?compasscal (a full two-circle run) or ?magalign. A reboot does NOT clear it (the verdict is stored in SPIFFS since 2026-08-18). It also clears on its own once the two sources are measured AGREEING for 5 continuous seconds.");
}

// ------------------------------------------------------------
// headingDisagreeClearAfterCal - the rider has re-measured the compass, so drop the verdict
// ------------------------------------------------------------
// V2.5-Evo - 2026-08-17. Inputs: what - a short name of the command that earned the clear, printed
// so the log says which one it was. Outputs: none. Side effects: clears heading_disagree_fault and
// its dwell bookkeeping, re-arms the degradation notice, prints one line.
//
// WHY THIS IS A CLEAR ROUTE AT ALL, when engagement boundaries are not. The latch is evidence about
// the COMPASS. A disarm is not evidence about anything — the compass is exactly as wrong after it
// as before. A successful ?compasscal or ?magalign IS: the rider has physically re-measured the
// mounting, so the number the verdict was formed against no longer exists. This is the escape hatch
// that keeps a sticky latch honest: fix the compass and you get hybrid mode back on the spot, with
// no reboot and no coasting to re-prove anything.
//
// CALLER'S CONTRACT (enforced in Compass.ino, not here): call this ONLY from a run that actually
// re-measured the mounting — CAL_FULL from runCompassCalibration(), or a completed runMagAlign().
// A PARTIAL cal keeps the OLD mag_orientation, which is usually the very thing that caused the
// disagreement, so it must not clear it. This function cannot check that for itself; it is a
// deliberate, documented split.
//
// It writes NO control variable: no cap, no gate, no override, and nothing in usrConf.
// ------------------------------------------------------------
static void headingDisagreeClearAfterCal(const char *what)
{
  if (!heading_disagree_fault && heading_disagree_since_ms == 0) return;  // nothing to forgive

  heading_disagree_since_ms     = 0;
  heading_disagree_last_seen_ms = 0;
  heading_disagree_fault        = false;
  headingDisagreePersist(false);         // LATCH-1: the stored verdict goes with it
  heading_degrade_announced     = false;
  // V2.5-Evo - 2026-08-17 - the agreement dwell goes with them. It is bookkeeping toward a clear
  // that has just happened by another route, so carrying it forward would describe a measurement
  // of a mounting that no longer exists. Pair cleared together, like every other dwell here.
  heading_agree_since_ms        = 0;
  heading_agree_last_seen_ms    = 0;

  Serial.printf("HEADING [RX] compass re-measured by %s — the heading-disagreement latch is cleared and hybrid heading is available again.\n",
                (what != NULL) ? what : "calibration");
}

// V2.5-Evo - 2026-08-16 - The oldest part-finished disagreement proof that may still be counted.
// It exists because the "no comparison possible" branch of getRtmHeading() FREEZES
// heading_disagree_since_ms instead of clearing it (so a dwell survives a COG dropout mid-proof).
// Frozen without a limit, the timestamp stops measuring a sustained condition and starts measuring
// the wall clock: two one-tick disagreements minutes apart would satisfy "held continuously for
// kHeadingDisagreeMs" and escalate, which is not what the constant is documented to mean.
// 3x the dwell is the smallest multiple that is comfortably clear of the case the freeze was
// written for - a real disagreement measurable either side of one handover, which spans a second
// or two - while refusing to treat evidence from a different minute as part of the same event.
// This is a compile-time constant, like every other k* in this file: no confStruct field, no SPIFFS
// slot, no web-UI row, no SW_VERSION bump.
static const unsigned long kHeadingDisagreeEvidenceMaxMs = 3UL * (unsigned long)kHeadingDisagreeMs;  // ms (15 s at the shipped 5 s dwell)

// V2.5-Evo - 2026-08-16 - The largest gap allowed BETWEEN two measured disagreements for them to
// count as one continuous event.
// WHAT THE SPAN CAP ABOVE STILL LET THROUGH. kHeadingDisagreeEvidenceMaxMs bounds the total SPAN of
// a dwell, measured from heading_disagree_since_ms — the START of it — and it is only ever tested on
// ticks where no comparison is possible. So: one disagreeing tick at t = 0 opens the dwell; the
// comparison is impossible for the next fourteen seconds; one more disagreeing tick arrives at
// t = 14 s. The span test is (14000 - 0) > 15000, which is false, so the evidence survives — and
// then (now - since) >= kHeadingDisagreeMs is true on that same tick, because the dwell clock has
// been frozen the whole time. A fault built from two isolated one-tick samples fourteen seconds
// apart: precisely the defect the span cap was added to kill, merely bounded at 15 s instead of
// unbounded. The consequence is a nuisance FM fault-stop mid-wave, not a danger — but it is still
// the guard reporting something it did not measure.
// WHAT THIS ADDS. Continuity. A dwell only carries forward while the disagreeing samples KEEP
// ARRIVING; a longer silence than this and the next disagreement opens a fresh dwell from itself.
// Span is not evidence of persistence — arrival rate is — so the guard now bounds both.
// WHY 2000 ms. It has to sit comfortably above every gap that is a genuine hiccup in the two inputs
// a comparison needs, and comfortably below the tens of seconds the counterexample depends on. The
// compass snapshot is only comparable for kHeadingCompareSnapMs (1000 ms) after a motor-idle
// capture, and the COG freshness gate in getRtmHeading() is 1500 ms; 2000 ms clears the looser of
// those two, so any gap longer than this means an input was genuinely ABSENT, not merely late —
// which is the ordinary COG dropout the freeze exists for, and one of those spans a second or two,
// not tens of seconds. At the 10 Hz ladder cadence a truly continuous disagreement re-measures every
// ~100 ms, so 2000 ms is ~20 consecutive missed opportunities before continuity is called broken:
// nothing continuous comes near it. And it forces at least four measured samples to span the 5 s
// dwell, so the two-samples-far-apart fault is no longer reachable at any spacing.
// DIRECTION OF THE CHANGE: strictly harder to set the fault, never easier. Restarting a dwell can
// only delay an escalation, and the per-tick verdict (a measured disagreement returns confidence 0
// on that tick regardless) is untouched, so no tick loses protection.
// Compile-time constant like every other k* in this file: no confStruct field, no SPIFFS slot, no
// web-UI row, no SW_VERSION bump.
static const unsigned long kHeadingDisagreeGapMaxMs = 2000UL;  // ms; max spacing between two disagreeing samples


// V2.5-Evo - 2026-08-16 - Last-good COG, held briefly across a dropout.

// RTM governs the craft to rtm_target_speed_kmh (default 4.0 km/h) while COG is abandoned

// below rtm_cog_min_speed_kmh (default 3). That 1 km/h margin is 0.278 m/s, which is inside

// the speed signal's OWN noise - so the heading source flips on sensor noise alone, before any

// wave or gust. Every flip on a rotated compass injects a large heading error.

// Holding the last good COG across a short dip removes the flapping without changing either

// threshold, and without inventing a heading: a course from 2 seconds ago on a craft doing

// 4 km/h is a far better estimate than a compass that is 87-100 deg wrong under motor load.

// V2.5-Evo - 2026-08-16 - These two are deliberately NOT static any more. Logger.ino's duplicate
// heading ladder READS them (it never writes them) so the rtm_source / rtm_confidence columns can
// report a held COG instead of misreporting it as a compass reading or as no source at all. Same
// export pattern as g_heading_error_dx10 above: a plain global here, picked up by a local extern
// in Logger.ino. Single writer - getRtmHeading(), loop task; loggerTask only reads, and the worst
// a preemption between the two reads can cost is ~100 ms of apparent age in a log column, never a
// control decision.
float                cog_last_good_deg = -1.0f;

unsigned long        cog_last_good_ms  = 0;


// V2.5-Evo - 2026-05-06 - D5: Layered heading source for RTM steering.
//
// Returns the best available heading (deg, 0-360 clockwise from North) based on
// usrConf.rtm_use_compass mode and current sensor state. Three modes:
//   0 = GPS COG only — no compass fallback. Safest choice for builds where compass
//       is biased by motor current (this hardware's bench-tested behavior).
//   1 = Hybrid (DEFAULT) — GPS COG primary; compass snapshot when buggy is too slow
//       for COG to be reliable. Compass snapshot is updated only when motor is idle
//       (thr_received < 25), so it represents an unbiased reading.
//   2 = Compass only — DIAGNOSTIC ONLY. Should NOT be used on water on builds with
//       known motor EMI. Available for non-EMI builds with proven clean compass
//       behavior under load.
//
// Confidence levels (output param):
//   3 = HIGH:   GPS COG, fresh and above min_speed threshold
//   2 = MEDIUM: compass snapshot < 1000ms old, or compass-only mode (legacy)
//   1 = LOW:    compass snapshot 1000-8000ms old (degraded — caller should reduce steering authority)
//   0 = NONE:   no valid heading source — caller must hold straight (rtm_steer_override = 127)
//
// Returns true if heading is valid (any non-zero confidence), false if no source.
//
// V2.5-Evo - 2026-07-25 - STAGE 2: two trust guards now sit inside the COG branch:
//   GUARD 1 (COG liveness by value change): a COG whose VALUE has not moved for kRtmCogFrozenMs
//     while the buggy is moving is not a heading, it is a repeated number. Speed-gated, so a
//     stationary buggy — whose constant course is CORRECT — is never faulted.
//   GUARD 2 (compass vs COG cross-check): if the two independent sources are more than
//     kHeadingDisagreeDeg apart, the disagreement is recorded and may withdraw the compass.
//     A valid GPS COG remains usable and may engage Follow-Me.
//
// V2.5-Evo - 2026-08-17 - A TEMPORARY LADDER THAT IS NOT A SETTING: while guard 2's latch stands,
// the COMPASS is withdrawn from this function — the live GPS course and the kCogHoldMs held course
// are both still served, so the ladder becomes "mode 1 minus the compass" rather than any stored
// mode. usrConf is READ, never written — the rider's rtm_use_compass and rtm_compass_required
// survive untouched, and the withdrawal disappears on the next boot or the moment the compass is
// re-measured or exonerated. Mode 2 is the exception: it has no COG to fall back on, so a standing
// fault there returns no heading at all.
// WHY THE HOLD STAYS (this was the other way round for one pass): the hold serves a last-good GPS
// COURSE and the latch is evidence about the COMPASS, so withdrawing the hold on that evidence
// removed something the guard has nothing against — and it cost a real bridge across the ordinary
// COG dropout. See the check that now sits BELOW the hold.
//
// SAFETY: This function still never modifies sensor state, config, telemetry or any control
//         variable. The only state it writes is the guard-2 bookkeeping declared directly above —
//         heading_disagree_since_ms, heading_disagree_last_seen_ms and heading_disagree_fault
//         (V2.5-Evo - 2026-08-16: last_seen added; the list previously named two of them),
//         V2.5-Evo - 2026-08-17 the heading_agree_since_ms / heading_agree_last_seen_ms pair that
//         measures the agreement dwell for clear route 1, plus the heading_degrade_announced
//         notification flag it sets through headingDisagreeAnnounceDegraded() — and the
//         cog_last_good_deg / cog_last_good_ms pair used by the COG hold. All of it is single-writer from the loop task.
//         Caller (updateRtmSteering) must handle confidence=0 as a hold-straight
//         scenario, not as a steering command.
static bool getRtmHeading(float* out_heading, uint8_t* out_confidence)
{
  uint16_t mode           = usrConf.rtm_use_compass;
  uint16_t cog_min_speed  = usrConf.rtm_cog_min_speed_kmh;
  unsigned long now       = millis();

  // ============================================================
  // V2.5-Evo - 2026-08-17 - THE DEGRADATION NOTICE RETRIES HERE, EVERY TICK, UNTIL IT PRINTS.
  //
  // WHAT WAS WRONG. headingDisagreeAnnounceDegraded() defers while the trigger is held and returns
  // WITHOUT setting its one-shot flag, which is correct (see the F7 note on the function). But its
  // only call site was inside the if (disagree_now) branch far below — so the retry needed another
  // MEASURED disagreement, and a measurement needs a live COG together with a compass snapshot
  // younger than kHeadingCompareSnapMs, which only refreshes while thr_received < 25. Put those two
  // facts together and the deferral was not a deferral: if the dwell completed inside the ~1 s
  // window after a squeeze the notice was suppressed, and if the rider then finished the session
  // under power and never coasted above rtm_cog_min_speed_kmh again, no further comparison was ever
  // possible and THE NOTICE WAS LOST. They rode the whole session with the compass withdrawn and
  // no indication anywhere except a manual ?diag.
  //
  // WHAT THIS DOES. Decouples the retry from measurability. While the verdict stands, every call of
  // this function offers to announce it, whether or not anything is currently comparable. Nothing
  // about the notice itself changes: heading_degrade_announced still makes it print exactly once per
  // fault, and the thr_received >= 25 deferral inside the function is untouched, so it still returns
  // early without arming that flag while the motor can be live.
  //
  // WHY THIS PLACEMENT CANNOT PUT A SERIAL CALL BETWEEN A PROVEN FAULT AND A MOTOR-STOPPING WRITE.
  // The function prints only when thr_received < 25, and every motor-relevant write this guard feeds
  // is only consequential while the trigger is HELD — calcPWM() applies rtm_steer_override and the
  // caps under thr_received >= 25, and below that the deadman already has the motor at 0. So on any
  // tick where this can print there is nothing left to defer. Call site by call site:
  //   gate 6 in checkRtmSafetyGates() — unreachable below 25 counts, because gate 1 returns first,
  //     so this line prints NOTHING there and cannot precede that gate's rtm_rx_emergency_stop;
  //   checkFmFaultConditions() — does run on released-trigger ticks and is upstream of
  //     fm_throttle_cap = 0, which is exactly the case the deferral was written for and exactly the
  //     case where cap 0 is redundant, the trigger being released already;
  //   updateRtmSteering() — reached only with the trigger held (RTM past its gates, or FM_ACTIVE
  //     via hard_ok), so again nothing prints;
  //   the fm_status telemetry call in runRtmLoop() — has no motor-stopping write downstream of it.
  // Placed ABOVE the no-fix guard below on purpose: a fault that is standing must be announceable
  // even on a tick with no usable GPS, which is the whole point of decoupling it from measurement.
  //
  // It writes no control variable — not a cap, not a gate, not the override, not usrConf.
  // ============================================================
  if (heading_disagree_fault) headingDisagreeAnnounceDegraded();

  // ---- No-fix engagement guard (2026-07-19 FM triage, Fable audit §5) ----
  // A heading source is only meaningful for RTM/FM steering when the RX has a real
  // GPS position fix: the steering bearing is computed from gps_last_lat/lng, which are
  // 0,0 with no fix. The Fable log showed RTM reporting confidence=2 from a compass
  // snapshot while datetime_unix=0 (no fix) — that must never engage or be reported.
  // Require a fresh RX GPS fix (same age window as Gate 5) before granting ANY confidence.
  // A stationary buggy that HAS a fix still passes (gps_last_ms updates while stopped).
  if (gps_last_ms == 0 || (now - gps_last_ms) > 6000UL) {
    *out_heading = -1.0f;
    *out_confidence = 0;
    return false;
  }

  // ---- Mode 2: Compass only (legacy/diagnostic) ----
  // Use compass directly; valid only if compass returns non-error.
  // SAFETY: This mode SHOULD NOT be used on water — see field-service note in BREmote_V2_Rx.h.
  //
  // V2.5-Evo - 2026-08-17 - A LATCHED DISAGREEMENT LEAVES THIS MODE WITH NOTHING. Elsewhere the
  // fault withdraws the COMPASS and leaves the GPS course (live and held) in place. Mode 2 never
  // consults COG at all, so the only source it has is the one the guard has measured 45+ deg away
  // from the other. Handing that out would be steering a towing buggy at a person on an instrument
  // we have evidence against, and "the operator opted in" is not consent to a sensor that has since
  // been shown to conflict — they opted into a compass they believed was good.
  // SO WE WITHHOLD IT: no heading, confidence 0, and every consumer holds straight. That is
  // conservative in the direction this whole guard runs — it subtracts a source, it never adds one
  // — and it is NOT a refusal to operate: RTM still runs on the rider's throttle, holding straight.
  // FM mode 2 cannot engage because it explicitly excludes COG and condition 6 still requires a
  // real heading. Modes 0/1 can engage on valid GPS COG.
  // HOW IT CAN ARISE AT ALL: the comparison that sets the fault only runs in mode 1, so this state
  // needs a rider who latched a fault in hybrid mode and then switched to mode 2 in the same power
  // cycle. HOW TO GET OUT: ?compasscal / ?magalign. A reboot does NOT clear it (the verdict is stored in SPIFFS since 2026-08-18). Route 1 (a live measurement
  // showing agreement) cannot fire here, because no comparison is made in mode 2 — which is another
  // reason not to trust the compass in the meantime: in this mode nothing can exonerate it.
  if (mode == 2) {
    if (heading_disagree_fault) {
      *out_heading = -1.0f;
      *out_confidence = 0;
      return false;
    }
    float h = getCompassHeading();
    if (h < 0.0f) {
      *out_heading = -1.0f;
      *out_confidence = 0;
      return false;
    }
    *out_heading = h;
    *out_confidence = 2;  // MEDIUM — known biased under load but user opted in
    return true;
  }

  // ---- GPS COG (preferred for modes 0 and 1) ----
  // Valid if: course was captured (ms > 0), course is in valid range,
  //           course age < 1500ms, GPS speed >= cog_min_speed_kmh,
  //           and (STAGE 2 guard 1) the course VALUE is not frozen while moving.
  bool cog_captured = (gps_last_course_ms > 0) && (gps_last_course_deg >= 0.0f);
  bool cog_fresh_ts = cog_captured && ((now - gps_last_course_ms) < 1500UL);
  bool cog_moving   = (gps_last_speed_kmh >= (float)cog_min_speed);

  // ============================================================
  // GUARD 1 — COG LIVENESS BY VALUE CHANGE, NOT BY TIMESTAMP
  // V2.5-Evo - 2026-07-25 - STAGE 2
  //
  // WHAT THE BUG WAS. The four sub-conditions above ask "is a course present, in range, recent,
  // and are we fast enough for it to mean anything". None of them asks the only question that
  // actually matters: IS IT STILL MOVING. gps_last_course_ms is re-stamped by GPS.ino on every
  // valid course sentence, including a sentence that repeats the previous heading exactly. The
  // owner's bench ?diag caught precisely that: 7.4 timestamp-updates/s against 0.0
  // value-changes/s, one heading held for 533 s. Every tick of those nine minutes this branch
  // returned confidence 3 (HIGH) on a number that had not moved since before the run started,
  // and the steering controller drove on it. That is the Follow-Me veer.
  //
  // WHAT THE FIX DOES. It reuses the Stage 0 tracker rather than duplicating it: GPS.ino already
  // stamps g_diag_cog_change_ms every time the course VALUE moves by more than kDiagCogChangeDeg
  // (0.05 deg). If that stamp is older than kRtmCogFrozenMs, the number is frozen and COG is not
  // a heading source this tick. Nothing in GPS.ino is touched — this is a read.
  //
  // WHY IT IS SPEED-GATED, AND WHY THAT IS NOT OPTIONAL. A stationary buggy legitimately reports
  // the same course forever: that is a correct reading, not a fault, and faulting it would break
  // the low-speed RTM arm the owner relies on (it is exactly why the compass snapshot fallback
  // exists). So the freeze verdict is only formed while gps_last_speed_kmh >= cog_min_speed — the
  // same gate this branch already applies. Below that speed the guard does not exist, and COG is
  // rejected for being too slow exactly as it always was, falling through to the compass.
  //
  // WHY IT ALSO REQUIRES A FRESH TIMESTAMP. The verdict "the value is frozen" is only meaningful
  // while the module is actively pushing course sentences — that is the measured signature
  // (timestamp fresh, value stuck). If the timestamp itself has gone stale, that is the ordinary
  // stale-COG case which already falls through to the compass, and it is left behaving exactly as
  // it does today. This keeps guard 1 aimed at the one failure it was written for.
  //
  // SENTINEL: g_diag_cog_change_ms == 0 means no COG value has EVER been captured this session
  // (?diagz deliberately does not clear it). That cannot coexist with cog_fresh_ts, but if it
  // somehow did we form NO freeze verdict — we never suppress a source on the strength of a
  // contradiction, only on positive evidence.
  // ============================================================
  bool cog_frozen_moving = false;
  if (cog_moving && cog_fresh_ts) {
    unsigned long cog_change_ms = (unsigned long)g_diag_cog_change_ms;
    cog_frozen_moving = (cog_change_ms != 0) &&
                        ((now - cog_change_ms) >= (unsigned long)kRtmCogFrozenMs);
  }

  bool cog_valid = cog_fresh_ts && cog_moving && !cog_frozen_moving;

  // ============================================================
  // GUARD 2 — COMPASS vs COG DISAGREEMENT
  // V2.5-Evo - 2026-07-25 - STAGE 2
  //
  // WHAT IT DOES. When both independent heading estimates are available AND simultaneous, it
  // measures the shortest angular distance between them. Beyond kHeadingDisagreeDeg it records
  // evidence against the compass; sustained disagreement withdraws that compass from the ladder.
  // A valid GPS COG remains eligible on the same tick and may engage Follow-Me.
  //
  // WHY ONLY IN MODE 1 (HYBRID). Mode 1 is the only mode in which the ladder can actually PROMOTE
  // the compass to a steering source, so it is the only mode where a bad compass can take the
  // buggy anywhere. In mode 0 the operator has declared the compass untrusted and it is excluded
  // from the ladder entirely — letting that same excluded sensor veto a good COG would invert the
  // meaning of the setting and import faults into the mode chosen to avoid them (and mode 0 needs
  // no help here: with COG gone it already returns nothing). Mode 2 returns above this point and
  // never consults COG at all; it is documented DIAGNOSTIC-ONLY, not for water.
  //
  // WHY THE COMPASS SNAPSHOT AND NOT A LIVE READ. getCompassHeading() is an I2C transaction that
  // takes the shared i2cMutex, and this function is called several times per 10 Hz tick from four
  // call sites — that is 40-60 extra bus transactions a second inserted into the control loop.
  // Worse, a live compass read under motor current is exactly the EMI-biased value this hardware
  // is known for, so it would manufacture disagreements and fault every engagement. The snapshot
  // is captured motor-idle by Compass.ino, which is the only compass reading this firmware trusts.
  //
  // TIMING HONESTY. Because the snapshot only refreshes with the trigger released, the comparison
  // is live around the moment of the squeeze and goes dormant while the rider holds it (see
  // kHeadingCompareSnapMs). Dormant means "not measured", never "measured and passed".
  // ============================================================
  bool compare_possible = (mode == 1) && cog_valid &&
                          (compass_snapshot_heading >= 0.0f) && (compass_snapshot_ms > 0) &&
                          ((now - compass_snapshot_ms) < (unsigned long)kHeadingCompareSnapMs);

  bool disagree_now = false;
  if (compare_possible) {
    // Shortest angular distance, wrap-correct across 0/360 (e.g. 359 vs 1 is 2 deg, not 358).
    // Written out inline rather than calling fmAngleDiff(): that helper is defined ~900 lines
    // below in this same file, and a safety guard should not depend on the Arduino builder's
    // auto-prototype ordering to compile.
    float d = gps_last_course_deg - compass_snapshot_heading;
    while (d >  180.0f) d -= 360.0f;
    while (d < -180.0f) d += 360.0f;
    disagree_now = (fabsf(d) > kHeadingDisagreeDeg);
  }

  // Persistence bookkeeping. A transient disagreement does not veto the valid GPS COG. Sustained
  // disagreement past kHeadingDisagreeMs proves the fault and withdraws the compass.
  // V2.5-Evo - 2026-08-27 - The resulting latch no longer blocks or ends Follow-Me: live COG and
  // the short held-COG bridge stay available. Without either, there is still no usable heading.
  // Besides setting the flag this block TELLS THE RIDER, once.
  // WHY PRINTING HERE IS STILL F7-SAFE. The F7 rule is
  // "motor to 0 first, explain afterwards", because a full USB CDC TX buffer can block inside
  // Serial for as long as the host takes to drain it. This print sits UPSTREAM of the FM stop:
  // checkFmFaultConditions() calls this function, and fm_throttle_cap = 0 is written further down
  // the same tick. So headingDisagreeAnnounceDegraded() refuses to print while the trigger is held
  // (thr_received >= 25) and prints at the next release instead — the notice keeps its one-shot
  // behaviour, and nothing can sit between a proven fault and the motor reaching 0. The rider can
  // also ask the board at any time with ?diag, which reports the latch without printing anything
  // in the control path.
  if (disagree_now) {
    // V2.5-Evo - 2026-08-17 - a measured disagreement ends any agreement dwell outright. The two
    // dwells are mutually exclusive by construction: one disagreeing sample is proof that agreement
    // has NOT held continuously, so the exoneration must start again from the next agreeing sample.
    heading_agree_since_ms     = 0;
    heading_agree_last_seen_ms = 0;

    if (heading_disagree_since_ms == 0) {
      heading_disagree_since_ms = now;
    } else if ((now - heading_disagree_last_seen_ms) > kHeadingDisagreeGapMaxMs) {
      // V2.5-Evo - 2026-08-16 - CONTINUITY CHECK. The last time we actually MEASURED a
      // disagreement is too long ago for this sample to be a continuation of it, so this sample
      // opens a NEW dwell instead of completing the old one. Without this, the frozen dwell clock
      // in the else-branch below let two isolated samples far apart be added together and reported
      // as one sustained condition (see kHeadingDisagreeGapMaxMs for the worked example).
      // Restarting a dwell can only ever DELAY a fault, never create one.
      heading_disagree_since_ms = now;
    } else if ((now - heading_disagree_since_ms) >= (unsigned long)kHeadingDisagreeMs) {
      heading_disagree_fault = true;
      headingDisagreePersist(true);       // LATCH-1: survive a power cycle
      headingDisagreeAnnounceDegraded();   // one line, once per fault, on the transition only
    }
    // Stamped AFTER the continuity test above, which reads the PREVIOUS measurement's time.
    heading_disagree_last_seen_ms = now;
  } else if (compare_possible) {
    // Measured, and the two agree. This is clear route 1, and V2.5-Evo - 2026-08-17 it is the only
    // one that can fire without the rider doing anything — which is why compare_possible above
    // deliberately tests the CONFIGURED mode rather than the withdrawn one. Withdrawing the compass
    // must not switch off the measurement that exonerates it, or the latch really would need a
    // reboot.
    //
    // V2.5-Evo - 2026-08-17 - AN AGREEING SAMPLE IS NO LONGER AN ACQUITTAL ON ITS OWN. Convicting
    // takes kHeadingDisagreeMs of continuous measured disagreement — at least four samples — and
    // this branch used to undo all of it on ONE tick. A mirrored module is the case that breaks on:
    // it reports theta - h_true, so the compass-to-course gap is 2*h_true - theta and dips below
    // kHeadingDisagreeDeg in two 45-deg-wide windows per revolution. A rider coasting on a heading
    // in one of those windows produced agreeing samples with the module still mirrored, and the
    // latch vanished. ?magalign cannot detect mirroring either, so this was the defect the
    // stickiness existed for. Now agreement has to hold for the SAME dwell, with the SAME
    // continuity bound, before the verdict is withdrawn — no new constant, no new tuning.
    //
    // DIRECTION OF THE CHANGE: strictly harder to clear, never easier. A dwell that restarts can
    // only leave a proven fault standing for longer.
    if (heading_agree_since_ms == 0) {
      heading_agree_since_ms = now;
    } else if ((now - heading_agree_last_seen_ms) > kHeadingDisagreeGapMaxMs) {
      // Continuity broken: the previous agreeing sample is too old to be part of this run of
      // agreement, so this sample opens a fresh dwell instead of completing the old one. Same test,
      // same constant and same reasoning as the disagreement side above.
      heading_agree_since_ms = now;
    }
    // Stamped AFTER the continuity test, which reads the PREVIOUS measurement's time.
    heading_agree_last_seen_ms = now;

    // An unfinished DISAGREEMENT proof is dropped immediately, exactly as before: a measured
    // agreement is positive evidence against a dwell that has not yet convicted anything, and
    // dropping it can only delay a fault. Only the FAULT itself now waits for the full dwell.
    heading_disagree_since_ms     = 0;
    heading_disagree_last_seen_ms = 0;

    if ((now - heading_agree_since_ms) >= (unsigned long)kHeadingDisagreeMs) {
      if (heading_disagree_fault) {
        // Transition only — the flag below re-arms the notice, so this cannot repeat while the
        // sources keep agreeing. Safe to print here under the F7 rule: this branch only ever
        // RESTORES capability (the compass comes back, FM becomes eligible again), so there is no
        // motor-stopping write downstream of it that a blocked UART could defer.
        Serial.printf("HEADING [RX] COMPASS RESTORED: compass and GPS course measured agreeing continuously for %lu ms — the disagreement latch is cleared and compass fallback is available again.\n",
                      (unsigned long)kHeadingDisagreeMs);
      }
      heading_disagree_fault    = false;
      headingDisagreePersist(false);       // LATCH-1: the stored verdict goes with it
      heading_degrade_announced = false;   // re-arm the notice if it ever latches again
    }
  } else {
    // No comparison possible this tick. Restart the dwell rather than carry a half-finished proof
    // across a gap (same discipline as fm_diverge_since_ms). A fault already PROVEN is kept — it is
    // cleared by a fresh agreement above, by a successful ?compasscal / ?magalign. A reboot does NOT clear it (the verdict is stored in SPIFFS since 2026-08-18).
    // V2.5-Evo - 2026-08-16 - FREEZE, do not clear. This branch runs when the comparison is

    // IMPOSSIBLE (COG invalid), not when the sources agreed. Zeroing here discarded partial

    // evidence at precisely the handover the tester's failure happened on: a disagreement that

    // had been building for 4 of the required 5 seconds was wiped the moment COG dropped out,

    // so the escalation could never fire on the way into the slow zone. Leaving the timestamp

    // in place is safe - the fault only escalates inside if(disagree_now), which still requires

    // a live comparison, so a stale clock cannot fault anything on its own.

    //
    // V2.5-Evo - 2026-08-16 - ...but a frozen proof does not keep FOREVER. WHAT WAS STILL WRONG:
    // with the timestamp held for an unbounded time, two one-tick disagreements minutes apart
    // satisfied "the gap has held continuously for kHeadingDisagreeMs" and escalated to a fault -
    // the dwell was measuring the wall clock between two unrelated moments, not a sustained
    // condition. WHAT THIS DOES: a part-finished proof that has sat unmeasured for longer than
    // kHeadingDisagreeEvidenceMaxMs is discarded, and the next disagreement opens a fresh dwell.
    // It cannot weaken the guard in the case the freeze was written for: a genuinely continuous
    // disagreement escalates at 5 s and never reaches this age, and evidence spanning one COG
    // handover is seconds old, not tens of seconds. A fault already PROVEN is untouched here -
    // only the unfinished proof ages out.
    //
    // V2.5-Evo - 2026-08-16 - ...and this cap alone was not enough, because it bounds the SPAN of a
    // dwell (age measured from its first sample) and not the age of the last MEASUREMENT, and it is
    // only tested here, on ticks where no comparison is possible. Two isolated samples 14 s apart
    // therefore still escalated together. The companion bound is the continuity test in the
    // if(disagree_now) branch above: consecutive disagreements more than kHeadingDisagreeGapMaxMs
    // apart start a fresh dwell rather than completing an old one. The two work together — this one
    // discards evidence that has gone cold, that one refuses to stitch unrelated moments into a
    // single "sustained" condition — and neither can set a fault, only drop unfinished proof.
    if (heading_disagree_since_ms != 0 &&
        (now - heading_disagree_since_ms) > kHeadingDisagreeEvidenceMaxMs) {
      heading_disagree_since_ms     = 0;
      heading_disagree_last_seen_ms = 0;
    }

    // V2.5-Evo - 2026-08-17 - THE AGREEMENT DWELL AGES OUT ON THE SAME TERMS, and for the same
    // reason: a part-finished exoneration that has sat unmeasured longer than
    // kHeadingDisagreeEvidenceMaxMs is measuring the wall clock, not a sustained condition. The
    // continuity test in the agree branch above already refuses to stitch two distant samples
    // together; this is its companion, so a stale agreement clock cannot survive a long dormant
    // stretch and then acquit on the first sample after it. Pair cleared together, as everywhere
    // else here. Discarding it can only make a clear LATER, never earlier.
    if (heading_agree_since_ms != 0 &&
        (now - heading_agree_since_ms) > kHeadingDisagreeEvidenceMaxMs) {
      heading_agree_since_ms     = 0;
      heading_agree_last_seen_ms = 0;
    }
  }

  // A measured disagreement records evidence against the compass but no longer vetoes a valid GPS
  // course on this tick. The live-COG branch below therefore remains usable immediately; once the
  // dwell latches, the existing guard below the COG hold prevents any compass fallback.

  if (cog_valid) {
    cog_last_good_deg = gps_last_course_deg;

    cog_last_good_ms  = now;

    *out_heading = gps_last_course_deg;
    *out_confidence = 3;  // HIGH
    return true;
  }

  // ---- Mode 0: GPS COG only — no fallback ----
  // If COG is invalid (slow speed, stale, or guard-1 frozen), return no source.
  // updateRtmSteering() will hold straight.
  //
  // ============================================================
  // V2.5-Evo - 2026-08-17 - THE HEADING-DISAGREEMENT TERM THAT WAS ADDED TO THIS RETURN IS GONE
  // AGAIN, AND IS BACK BELOW THE COG HOLD WHERE IT STARTED.
  //
  // WHAT IT WAS DOING HERE. For one pass a standing heading_disagree_fault left the ladder at this
  // line, which withdrew the kCogHoldMs held-COG bridge as well as the compass. The stated reason
  // was consistency: the hold is a mode-1 feature, mode 0 never had it, so a session running as
  // "mode 0" should not have it either.
  //
  // WHY THAT WAS WRONG. The hold serves a LAST-GOOD GPS COURSE. The latch is evidence about the
  // COMPASS — and only about the compass; the cross-check cannot even say which of the two sources
  // is the liar. Withdrawing a GPS-derived value on compass evidence is outside this guard's
  // charter, and it was the one part of that pass that removed something the latch has no evidence
  // against. It cost real behaviour in both loops: a COG dropout longer than 1.5 s failed FM's
  // condition 6 and forced a full stop-and-re-arm the hold would have bridged, and on the RTM side
  // the documented cog_valid flicker in the 3-4 km/h approach band (rtm_target_speed_kmh 4.0
  // against rtm_cog_min_speed_kmh 3, a margin inside the speed signal's own noise) alternated the
  // steering override between centre and bearing at 10 Hz — the exact flapping the hold was written
  // to stop.
  //
  // WHAT IT MEANS NOW. Withdrawn = "mode 1 minus the compass", which is precisely what the evidence
  // supports: live COG at confidence 3, held COG at confidence 2 for up to kCogHoldMs, and no
  // compass. cog_last_good_deg / cog_last_good_ms are maintained in the live-COG branch above this
  // point, so they keep being refreshed while the latch stands — this is a move of one test, not
  // new plumbing.
  //
  // NOT A CONFIG WRITE, unchanged: usrConf.rtm_use_compass is READ here and never written; the
  // rider's stored mode is untouched and ?conf still reports what they set. The separate fault
  // verdict is persisted so reboot cannot silently restore the suspect compass.
  // ============================================================
  if (mode == 0) {
    *out_heading = -1.0f;
    *out_confidence = 0;
    return false;
  }

  // ============================================================
  // GUARD 1, PART 2 — A FROZEN COG DOES NOT PROMOTE THE COMPASS
  // V2.5-Evo - 2026-07-25 - STAGE 2
  //
  // THE SUBTLE PART, AND THE REASON THE VEER HAPPENED. Guard 1 invalidates COG. Left alone, the
  // ladder below would then do what it has always done and fall through to the compass — which is
  // EXACTLY the handover that steered the buggy the wrong way. Think about what we actually know
  // in this state: the GPS is provably dead (a register that stopped moving is not an opinion),
  // and the compass is unverifiable — the one instrument that could have cross-checked it is the
  // source we just threw away, and on this hardware the compass is biased 100 deg+ by motor
  // current at exactly the moment we would be relying on it (the buggy is moving, so the trigger
  // is held, so the snapshot is not even refreshing).
  //
  // One provably dead source plus one unverifiable source is not a heading. HOLDING STRAIGHT IS
  // SAFER THAN STEERING ON THE SURVIVOR: a buggy that holds its line is predictable and the rider
  // can see it and let go of the trigger, whereas a buggy steering confidently on a bad compass
  // goes somewhere nobody chose. So we return NOTHING.
  //
  // WHAT EACH MODE DOES WITH "NOTHING" — no new code path is invented for either:
  //   RTM: gate 6 stops the run only when rtm_compass_required = 1 AND the heading-disagreement
  //        latch is NOT standing; otherwise updateRtmSteering() holds the override at 127
  //        (straight) and RTM continues under the throttle governor with runPhaseC()'s convergence
  //        check still watching.
  //        V2.5-Evo - 2026-08-17 - CORRECTION, because this used to say flatly that gate 6 stops
  //        the run at the default setting. It does not during a session in which the latch stands:
  //        that gate now stands aside whenever headingDisagreeLatched() is true (see the note at
  //        the gate), so a frozen COG on such a session behaves like the rtm_compass_required = 0
  //        case above — hold straight, keep running — rather than stopping. Stating it the old way
  //        described a stop that does not happen. The bypass is deliberately NOT narrowed here;
  //        narrowing it is a separate change with its own argument to make.
  //   FM : condition 6 in checkFmFaultConditions() fails, which is already classified as a FAULT
  //        — FM_STOPPING, cap 0, ramp back to manual, FM_IDLE, re-arm required. A valid live/held
  //        COG never reaches this path and remains sufficient for engagement.
  //
  // THIS IS SELF-CLEARING AND STRICTLY NARROW. It only exists while the buggy is MOVING and the
  // GPS is repeating itself; the instant the course value moves again, or the buggy slows below
  // cog_min_speed, the ordinary ladder (including the compass fallback) returns unchanged. The
  // stationary/low-speed RTM arm sequence never reaches this line.
  // ============================================================
  if (cog_frozen_moving) {
    *out_heading = -1.0f;
    *out_confidence = 0;
    return false;
  }

  // ---- Mode 1 (Hybrid): try compass snapshot ----
  // Snapshot is captured by updateCompassSnapshot() in Compass.ino during motor-idle.
  // Age determines confidence:
  //   < 1000ms : MEDIUM (likely still fresh)
  //   1000-8000ms : LOW (degraded; reduce steering authority) — SW45: extended from 3000ms for stationary RTM arm
  //   > 8000ms : NONE (too stale)
  // ============================================================

  // V2.5-Evo - 2026-08-16 - FIELD BUG: RTM veered at close range.

  //

  // Reported by a beta tester: RTM tracked straight toward him, then turned hard at ~5-7 m.

  // The cause is a HANDOVER. RTM decelerates inside rtm_approach_zone_m, speed falls below

  // rtm_cog_min_speed_kmh, cog_valid goes false, and the heading source switches from GPS COG

  // to the compass snapshot. If the compass is mounted rotated - and before SW35 there was no

  // mag_orientation to correct it - the heading STEPS by the mounting angle at that instant and

  // the steering obeys it.

  //

  // Two guards should have caught that and neither did:

  //

  //   1. The per-tick cross-check needs compare_possible, which requires cog_valid. It can only

  //      run while COG is trustworthy - which is exactly NOT the moment the compass takes over.

  //      The check protects you while you do not need it and stands down when you do.

  //

  //   2. The sticky escalation heading_disagree_fault DID survive, but it was consumed only by

  //      runFmLoop(). Follow-Me was protected against a lying compass; Return-to-Me was not -

  //      runRtmLoop() never read it. An asymmetry between two loops sharing one sensor stack.

  //

  //      V2.5-Evo - 2026-08-16 CORRECTION to point 2: "DID survive" was wrong, and it was the

  //      false premise this whole block rested on. The latch survived nothing during RTM -

  //      fmEnterIdle() zeroed it on every 100 ms tick, and runFmLoop() calls fmEnterIdle() while

  //      rtm_rx_active is set. So the check added below was reading a flag that was being wiped

  //      ten times a second and could never be true in an RTM-only run.

  //

  //      V2.5-Evo - 2026-08-17 SECOND CORRECTION: the fix for that was to make both clears

  //      edge-triggered, and those two edge clears are now GONE ENTIRELY - the latch is cleared

  //      only by evidence (a sustained measured agreement, a successful ?compasscal / ?magalign)

  //      A reboot does NOT clear it (the verdict is stored in SPIFFS since 2026-08-18). It therefore survives across engagements, and the withdrawal described below

  //      is exactly what a standing fault does: it takes the COMPASS out of the ladder and nothing

  //      else. The check is a few lines below, between the COG hold and the compass fallback.

  //

  // Fixed at the SOURCE rather than in either loop: if the compass has been caught disagreeing

  // with COG by more than kHeadingDisagreeDeg for kHeadingDisagreeMs, THE COMPASS FALLBACK BELOW

  // is withdrawn - from RTM, from FM, and from any future consumer. Both callers already treat

  // 'no heading' correctly: updateRtmSteering() sets rtm_steer_override = 127 and holds straight,

  // which at close range is the safe outcome and is what the buggy should have done here.

  //

  // V2.5-Evo - 2026-08-17 - SAY WHAT THE CODE DOES (third pass, and it is back to the second one).

  // The paragraph that first sat here explained why a HELD COG is still served while the latch

  // stands: the check sits BELOW the hold, and a held COG is a GPS measurement, not the compass

  // under suspicion. The pass in between moved the check above the hold and declared that

  // reasoning moot. It was not moot - it was the correct reading of what the latch is evidence

  // about, and the move quietly withdrew a GPS-derived value on compass evidence. The check is

  // back below the hold, so the original paragraph stands again and is restated here:

  //

  // A STANDING FAULT WITHDRAWS THE COMPASS AND NOTHING ELSE. Above rtm_cog_min_speed_kmh a live

  // GPS course is served at confidence 3; for up to kCogHoldMs after that it is served held, at

  // confidence 2; after that there is no heading and every consumer holds straight. The cross-check

  // cannot say WHICH of the two sources is lying, so it may only remove the one it is about.

  //

  // THE DIRECTION IS STILL SUBTRACTION, which is the only direction this guard is allowed to move:

  // the compass leaves the ladder and nothing is added. What the previous pass cost, and what

  // moving the check back recovers, is real in both loops - a COG dropout longer than 1.5 s failed

  // FM's condition 6 and forced a stop-and-re-arm, and the cog_valid flicker in the 3-4 km/h

  // approach band alternated the RTM override between centre and bearing at 10 Hz, which is the

  // exact flapping the hold below was written to stop.

  // ============================================================

  // ============================================================
  // V2.5-Evo - 2026-08-16 - HOLD the last good COG across a short dropout.
  //
  // Before falling back to the compass, prefer a COG that was valid moments ago. RTM drives at
  // 4.0 km/h with the COG floor at 3 - a 0.278 m/s margin, inside the speed signal's own noise -
  // so cog_valid flickers on noise alone. Each flicker previously handed steering to a compass
  // that is 87-100 deg wrong under motor load, and the resulting hard turn SLOWED the craft,
  // which kept COG invalid, which kept the bad compass in charge. A closed loop with no exit:
  // that is the 'veers off and never recovers' a tester filmed.
  //
  // Four independent codebases - Betaflight, iNav, Ardumower, ArduPilot/PX4 - blend COG and
  // magnetometer continuously rather than switching between them. A full blend is a bigger
  // change than this craft needs today; holding the last good COG gets most of the benefit for
  // a fraction of the risk, and changes NEITHER threshold.
  //
  // Confidence is deliberately dropped to 2 (MEDIUM): a 2-second-old course is still a real
  // measurement, but it is stale and the caller should not treat it like a live one.
  // ============================================================
  if (cog_last_good_deg >= 0.0f && cog_last_good_ms > 0 &&
      (now - cog_last_good_ms) < (unsigned long)kCogHoldMs) {
    *out_heading = cog_last_good_deg;
    *out_confidence = 2;   // MEDIUM - real course, but stale
    return true;
  }

  // ============================================================
  // V2.5-Evo - 2026-08-17 - GUARD 2's CONSEQUENCE: THE COMPASS FALLBACK IS WITHDRAWN.
  //
  // A compass that has been caught disagreeing with the GPS course by more than kHeadingDisagreeDeg
  // for kHeadingDisagreeMs does not get to steer this craft again until it is exonerated. Every
  // consumer inherits that from here — RTM, FM and anything added later — so no loop has to
  // remember to check the flag for itself.
  //
  // WHY THE CHECK IS HERE AND NOT HIGHER UP THE LADDER. It sits BELOW the live-COG branch and BELOW
  // the kCogHoldMs hold, and above the compass fallback, because those are GPS courses and this is
  // evidence about the COMPASS. For one pass it lived up at the mode-0 return, which also withdrew
  // the hold; that was consistency reasoning ("the hold is a mode-1 feature") applied to a value
  // the latch has nothing against, and it cost a COG-dropout bridge that both loops rely on. The
  // cross-check proves the two sources cannot both be right, not which one is wrong, so it may only
  // subtract the source it names.
  //
  // WHAT THE CALLERS DO WITH THE "NO HEADING" THIS PRODUCES, unchanged: updateRtmSteering() sets
  // rtm_steer_override = 127 and holds straight, which at close range is the safe outcome and is
  // what the buggy should have done in the veer this guard was written for; FM's condition 6 treats
  // it as a fault. Live/held COG returns above this point, so this path means GPS heading is truly
  // unavailable rather than merely that the compass disagreement latch is standing.
  // ============================================================
  if (heading_disagree_fault) {
    *out_heading = -1.0f;
    *out_confidence = 0;
    return false;
  }

  if (compass_snapshot_heading >= 0.0f && compass_snapshot_ms > 0) {
    unsigned long age_ms = now - compass_snapshot_ms;
    if (age_ms < 1000UL) {
      *out_heading = compass_snapshot_heading;
      *out_confidence = 2;  // MEDIUM
      return true;
    } else if (age_ms < 8000UL) {
      *out_heading = compass_snapshot_heading;
      *out_confidence = 1;  // LOW — caller should reduce steering authority
      return true;
    }
  }

  // ---- No valid heading source ----
  *out_heading = -1.0f;
  *out_confidence = 0;
  return false;
}

// ============================================================
// FOLLOW-ME (FM) AUTONOMOUS FOLLOWING - state, tuning constants, geometry
// V2.5-Evo - 2026-07-19 - P3 (DESIGN_FOLLOW_ME.md sections 4-7)
//
// WHAT FM DOES, IN PLAIN ENGLISH
// After the whip, the rider lets go of the rope and surfs the wave. FM makes the
// buggy trail the rider at a set distance and angle, steering itself, so the rider
// can keep their eyes on the wave instead of on the buggy.
//
// THE SAFETY RULES THIS CODE OBEYS (identical to RTM, non-negotiable):
//   1. The buggy ONLY moves while the rider physically holds the throttle trigger
//      (thr_received >= 25). FM never creates motion on its own.
//   2. FM ONLY steers and SUBTRACTS throttle. It can never add throttle. The human
//      trigger stays the one and only throttle source.
//   3. Releasing the trigger stops the buggy immediately (unchanged base architecture).
//   4. Every failure path - GPS, compass, LoRa, bad geometry - drives the motor to 0
//      by writing fm_throttle_cap = 0.
//
// FM writes exactly two things that can reach the motor: fm_throttle_cap (a cap that
// can only reduce throttle) and rtm_steer_override (steering only, and only while the
// trigger is held). Nothing else in this module touches the motor path.
// ============================================================

// ---- FM tuning constants (not user-configurable; the 8 SPIFFS FM params cover tuning) ----

// Rider speed below which a course derived from GPS positions is too noisy to trust.
// Measured: course noise roughly doubles below ~3 mph. Below this we drop to the
// degraded "hold station" geometry (no diagonal) rather than chase a bad course.
static const float    kFmCourseValidSpeedKmh = 5.0f;   // km/h

// How much faster than the rider the buggy is allowed to run while closing the gap.
// Feeds throttle cap 3 (speed governor): target = min(boogie_vmax, rider_speed + this).
static const float    kFmClosingMarginKmh    = 10.0f;  // km/h

// Stateful F1-F4 speed governor. The old stateless law multiplied the available throttle by
// (1 - speed/target), which made the cap zero AT the requested speed and therefore guaranteed a
// steady-state speed below the target. This PI controller instead learns the cap needed to hold the
// target. GPS speed and the F4 gap-derived target are filtered independently; the small deadband
// rejects speed quantisation. Cap removal is deliberately faster than cap restoration.
static const float    kFmSpeedFilterTauS          = 0.75f;
static const float    kFmSpeedTargetFilterTauS    = 1.00f;
static const float    kFmSpeedDeadbandKmh         = 0.50f;
static const float    kFmSpeedKp                  = 18.0f;  // cap counts per km/h
static const float    kFmSpeedKi                  = 4.0f;   // cap counts per km/h/second
static const float    kFmSpeedCapRisePerS         = 35.0f;
static const float    kFmSpeedCapFallPerS         = 100.0f;
static const float    kFmSpeedOverspeedBandKmh    = 2.0f;   // target+2 km/h -> hard speed cap 0

// Align-phase throttle cap (~5% of 255). While the heading error is large the buggy
// should pivot toward the target, not drive away from it. Same value RTM's align phase uses.
static const uint8_t  kFmAlignCap            = 13;     // 0-255

// Engage ramp length. On every entry into FM_ACTIVE the throttle cap ramps 0 -> full
// over this time so re-engagement is always a smooth build, never a throttle jump.
static const uint32_t kFmEngageRampMs        = 3500;   // ms

// Minimum time between rider course/speed samples. The rider's position arrives at 2 Hz,
// so we need a baseline of a few hundred ms for a stable course rather than differentiating
// two nearly identical filtered positions and getting noise.
static const float    kFmMotionBaselineS     = 0.4f;   // seconds

// ---- Engagement-semantics constants (V2.5-Evo - 2026-07-20) ----
// These numbers implement the separation latch and the stationary RETURN proof. They are deliberately
// compile-time only: no new confStruct fields, no SW_VERSION bump, no SPIFFS reset.

// How much further than the steady-state follow distance the rider must get before FM is
// allowed to engage for the first time. D_engage = kFmEngageFactor * d_follow.
// WHY THIS EXISTS: before this change the engage distance EQUALLED the follow distance
// (d_follow = min_dist_m + band). The tow rope MEASURES 20 ft = 6.10 m, which is LONGER than
// d_follow at the intended 4+2 = 6 m tuning — so FM could engage while the rider was still
// on the rope, i.e. autonomous steering mid-tow. 1.5x gives 9 m at 4+2 tuning: 9 m clears
// the measured 6.10 m rope by ~1.48x.
// V2.5-Evo - 2026-07-25 - F3-c prose fix: this comment used to quote a "6.7-7.6 m" tow rope.
// That number was an early estimate and it contradicted the MEASURED 6.10 m figure that
// BREmote_V2_Rx.h, ConfigService.ino and the web UI text all use. There is one rope length in
// this project and it is 6.10 m; the estimate is gone so the documentation stops arguing with itself.
// IMPORTANT: this factor is NOT the only thing protecting the engage distance. min_dist_m and
// followme_smoothing_band_m have no lower bound of their own, so a small tuning can make this
// product tiny — which is why kFmEngageDistFloorM is applied to the RESULT of this multiplication
// as well, not just to a manually typed fm_engage_dist_m. See the F3-c clamp in runFmLoop().

// How long the rider must stay beyond D_engage before the separation latch sets.
// The rider position arrives at 2 Hz, so 2000 ms = 4 consecutive independent GPS fixes.
// WHY: the logs contain single-fix GPS spikes implying 41-144 mph. A spike moves the
// apparent distance for one fix; it cannot SUSTAIN it for four. The dwell converts a
// noise-triggerable threshold into one that needs real, persistent separation.
static const uint32_t kFmSepDwellMs          = 2000;   // ms

// Fixed definition of "the foiler has stopped" used only by FM_RETURN. The 2 s dwell rejects one
// low GPS-speed sample; radial distance is used because course/along-track is undefined at rest.
static const float    kFmLatchResetSpeedKmh  = 2.0f;   // foiler below this = stationary for RETURN
static const uint32_t kFmLatchResetDwellMs   = 2000;   // stationary + outside D_engage must persist

// FM_RETURN uses the same stationary threshold and dwell as the latch lifecycle so there is one
// definition of "the foiler has stopped". The rider-moving exit has hysteresis, preventing a
// 1.9/2.1 km/h GPS wobble from switching between following and returning. Return remains bounded
// even if a declaration and trigger are held indefinitely.
static const float    kFmReturnResumeSpeedKmh = 3.0f;
static const uint32_t kFmReturnResumeDwellMs  = 1000;
static const uint32_t kFmReturnMaxRuntimeMs   = 60000;
static const uint32_t kFmReturnCheckMs        = 5000;
static const float    kFmReturnCloseEpsM      = 0.5f;
static const float    kFmReturnDefaultSpeedKmh = 5.0f;
static const float    kFmReturnHardMaxSpeedKmh = 8.0f;

// How long the RX keeps a TX-declared FM mode alive without a refresh.
// The TX re-sends 0xF2/mode every 30 s while armed, so 95 s is ~3 missed keepalives.
// WHY: without this the RX stored the declared mode FOREVER. If the TX's disarm burst
// (0xF2/0) was lost in the air, the RX stayed armed for the rest of the session with no
// way to find out. This is the backstop that expires a declaration nobody is refreshing.
static const uint32_t kFmModeAgeMs           = 95000;  // ms

// ---- FM fault/notification constants ----
// Compile-time only: no confStruct fields, no SW_VERSION bump, no SPIFFS reset.

// FAULT stop ramp. On a fault (conditions 2-7) FM hands throttle back to the rider by ramping the
// cap 0 -> 255 over this window, then drops to FM_IDLE (re-arm required). WHY the ramp: the rider
// may still be holding the trigger, so returning full manual throttle instantly would lurch.
static const uint32_t kFmStopRampMs          = 2000;   // ms

// How long the surprise-gated fault-stop notification stays sticky in fm_flags bit 3. WHY:
// telemetry rotates every ~2.4 s, so a one-shot notification could land between rotations and
// never reach the TX. 6 s guarantees the TX sees it and can fire the St + stop buzz exactly once.
static const uint32_t kFmFaultStickyMs       = 6000;   // ms

// ---- FM divergence-fault constants (V2.5-Evo - 2026-07-25 - A3) ----
// Compile-time only, like every other kFm* above: no confStruct fields, no SW_VERSION bump, no
// SPIFFS reset.
//
// WHAT WAS MISSING. While FM is ACTIVE the distance condition (condition 8, in runFmLoop) is
// "dist_m >= min_dist" — a LOWER bound only. It answers "is the buggy far enough away to be safe?"
// and nothing else. If the steering is wrong — a mirrored steering_inverted, a compass 180 out, a
// bad course estimate — the buggy drives AWAY from the rider and that condition keeps passing more
// and more comfortably the further it gets. RTM does have a divergence net (runPhaseC's convergence
// check, "distance must be decreasing"), but runPhaseC() is only ever called from runRtmLoop() and
// never from runFmLoop(), so FM had no upper bound at all and would steer away indefinitely for as
// long as the rider held the trigger. These two numbers add that bound.
//
// WHY A CEILING AND NOT runPhaseC's "must be decreasing". RTM's rule is right for RTM: the buggy is
// commanded to close on a stationary rider, so any non-decreasing distance is wrong. FM is not
// closing — it deliberately holds station d_follow behind a MOVING rider, so distance legitimately
// rises and falls every wave and "must be decreasing" would fire constantly. What is never
// legitimate in FM is being far outside the follow geometry. So we keep runPhaseC's bookkeeping
// shape (a single sustained-condition timer, cleared the instant the condition stops holding) and
// change only the test itself.

// Absolute distance beyond which the sustained not-closing test may classify the buggy as
// diverging. Since 2026-08-27 this comes from usrConf.fm_diverge_dist_m through
// fmEffectiveDivergeDistance(). Explicit values are metres, with a dynamic minimum of
// 2 x D_engage and an absolute maximum of 100 m. Zero reconstructs the old 6 x D_engage limit for
// existing SW35 configs before the 100 m cap. Dwell and closure epsilon remain unchanged.

// How long the distance must stay beyond that limit before it counts as divergence. Rider position
// arrives at 2 Hz, so 3000 ms is ~6 consecutive independent fixes — the same spike-proofing argument
// as kFmSepDwellMs. A single bad fix cannot trip it; a genuinely diverging buggy trips it in 3 s.
static const uint32_t kFmDivergeMs           = 3000;   // ms

// V2.5-Evo - 2026-07-25 - F1: how much closer the buggy must have got over the dwell window to be
// judged "following, just far" rather than "running away".
// WHY THIS EXISTS AT ALL. The first cut of this detector was a BARE THRESHOLD: beyond the ceiling for
// kFmDivergeMs = a fault, full stop. That is wrong for two reasons that together aborted ordinary
// engagements. (1) The engage ramp is kFmEngageRampMs = 3500 ms, LONGER than the 3000 ms dwell, so the
// fault could fire before the buggy had even been given full throttle. (2) During align the cap is
// kFmAlignCap = 13/255 (~5%), so the buggy pivots on the spot and the distance GROWS before it starts
// to shrink — while at the engagement instant dist_m is typically 13-21 m against an 18 m ceiling
// (2 x 9 m). The result was FM aborting ~3 s into most real engagements and forcing a mid-session
// re-arm. THE FIX: judge the DERIVATIVE, not the level — exactly what runPhaseC()'s convergence check
// does ("dist_m >= rtm_prev_dist_m" -> not closing -> fail). We snapshot the distance when the dwell
// starts and, at dwell expiry, only fault if the buggy has NOT closed by more than this epsilon.
// WHY 2.0 m. The buggy's closing speed is capped by cap 3, the speed governor, at rider speed +
// kFmClosingMarginKmh = 10 km/h = 2.78 m/s, so over the 3 s dwell a genuinely closing buggy recovers
// up to ~8.3 m — comfortably more than 2 m. 2 m is meanwhile larger than ordinary GPS scatter at these
// distances, so noise alone cannot fake "closing" and cancel a real divergence.
static const float    kFmDivergeCloseEpsM    = 2.0f;   // metres of closure over the dwell

// V2.5-Evo - 2026-07-25 - F3-b: the hard floor for the MANUAL fm_engage_dist_m override,
// kFmEngageDistFloorM, is NOT defined here any more. It used to sit in this block as 5.0f while
// ConfigService.ino carried a SECOND bare 5.0f literal, on the false premise that the Arduino
// concatenation order stopped the two files sharing a constant. It now has exactly one definition,
// in BREmote_V2_Rx.h — raised there to 8.0 m, because 5.0 m was below the tow rope it exists to
// clear (the owner's rope is 20 ft = 6.10 m). Both the config validator and the read-site clamp in
// runFmLoop() below reference that one constant. See BREmote_V2_Rx.h for the full rationale.

// ---- FM state machine (DESIGN_FOLLOW_ME.md section 4) ----
//   FM_IDLE    : FM off (mode 0), RTM owns the buggy, or GPS/FM disabled.
//                No throttle cap (255) and no steering override - fully manual buggy.
//   FM_ARMED   : a mode (1-4) is selected and all monitoring runs, but FM has not engaged
//                yet. The throttle chain is INACTIVE (cap 255) so the rider still has full
//                manual control of the buggy while FM waits for the follow geometry.
//   FM_ACTIVE  : FM has engaged at least once and owns the session until FM_RETURN, explicit disarm
//                or fault. fm_rx_active says whether automatic steering is live on
//                THIS tick. Trigger release keeps FM_ACTIVE and needs no cap write because the trigger
//                already commands zero. Geometry/front loss is warning-only. Crossing min_dist_m
//                latches cap 0 until trigger release; release then clears the separation proof and
//                exposes manual cap 255, so autonomy must re-prove >D_engage. Every automatic resume
//                restarts the engage ramp and P+D state.
//   FM_RETURN  : a stationary rider outside D_engage is retrieved directly. Entry clears the
//                separation latch. Arrival or sustained rider motion exits to FM_ARMED, never
//                directly to FM_ACTIVE or FM_IDLE; a fresh >D_engage proof is required.
//   FM_STOPPING: FM was engaged and a FAULT dropped out - conditions 2-7 (Phase A/B, TX/RX GPS
//                stale, heading invalid, LoRa). Something actually broke, so autonomy ends for
//                this run: the throttle cap ramps 0 -> 255 over kFmStopRampMs (throttle always
//                returns, never a lurch under a held trigger), then FM drops to FM_IDLE and a
//                fresh TX declaration is required to re-arm. A surprise-gated St + stop buzz fires
//                (fm_flags bit 3) only if the trigger was held at the fault instant.
enum FmState : uint8_t {
  FM_IDLE = 0,
  FM_ARMED = 1,
  FM_ACTIVE = 2,
  FM_STOPPING = 4,
  FM_RETURN = 5
};
static FmState fm_state = FM_IDLE;

// ---- FM rider tracking state ----
// fm_filt_* is FM's EMA-filtered rider position. It uses the SAME first-order filter
// formula and the SAME preset time constant as RTM's tx_pos_filtered_* (see
// updateRtmSteering) - the filter must keep ignoring the rider's carves so the buggy
// follows the low-passed path instead of mirroring bottom turns (measured p95 turn rate
// 49 deg/s at ~5 m radius). FM keeps its own copy because tracking has to stay warm while
// FM is only ARMED, whereas RTM's filter only runs while RTM is actively steering.
static double        fm_filt_lat         = 0.0;
static double        fm_filt_lng         = 0.0;
static bool          fm_filt_init        = false;
static unsigned long fm_filt_prev_ms     = 0;     // last EMA update (for the filter dt)

// Previous filtered position, used as the baseline for deriving rider course and speed.
static double        fm_prev_filt_lat    = 0.0;
static double        fm_prev_filt_lng    = 0.0;
static unsigned long fm_prev_filt_ms     = 0;

// Rider motion derived from successive FILTERED positions.
// fm_rider_course_deg is -1.0f when the rider is too slow for a trustworthy course.
static float         fm_rider_course_deg = -1.0f;  // 0-360 deg clockwise from North, or -1 = invalid
static float         fm_rider_speed_kmh  = 0.0f;   // km/h

// F1-F4 speed-governor memory. Updated only on a fresh accepted RX GPS-speed sample, so the 10 Hz
// navigation loop cannot integrate the same measurement repeatedly. The integrator starts open at
// 255; the independent engage ramp still provides the safe 0->full activation edge.
static bool          fm_speed_gov_init             = false;
static uint8_t       fm_speed_gov_mode             = 0xFF;
static unsigned long fm_speed_gov_last_gps_ms      = 0;
static float         fm_speed_filtered_kmh         = 0.0f;
static float         fm_speed_target_filtered_kmh  = 0.0f;
static float         fm_speed_integrator           = 255.0f;
static float         fm_speed_cap_slewed            = 255.0f;
static bool          fm_speed_other_cap_active     = false;

// Side-zone Schmitt state: true = apply the diagonal offset, false = sit directly behind.
static bool          fm_diagonal_engaged = false;

// millis() at the moment FM entered FM_ACTIVE. Drives the engage ramp. 0 = not engaged.
static unsigned long fm_engage_ms        = 0;

// ---- Separation latch state (V2.5-Evo - 2026-07-20) ----
// True after every F1-F4 mode has proven radial dist > D_engage for kFmSepDwellMs while the trigger
// is held. It is not changed by side/front geometry or an ordinary trigger release. Reaching
// min_dist_m creates a separate hard-stop latch; releasing the trigger after that stop clears both
// latches, so automatic control cannot resume until radial separation is proven again. IDLE/fault
// boundaries also clear it.
static bool          fm_sep_latched      = false;

// millis() when the rider first went beyond D_engage; 0 = not currently beyond it.
// Counts the dwell that defeats single-fix GPS spikes.
static unsigned long fm_sep_over_since_ms = 0;

// Hard stop at min_dist_m. This is intentionally distinct from fm_sep_latched: it stays set while
// the trigger remains held even if GPS distance grows again. The release edge clears it, clears the
// separation proof, and publishes cap 255 for manual repositioning before a fresh >D_engage proof.
static bool          fm_min_dist_stop_latched = false;

// ---- FM_RETURN lifecycle ----
// The candidate proof is deliberately separate from fm_sep_latched: RETURN is trigger-independent,
// while normal F1-F4 engagement proof requires the trigger. Both use radial D_engage because RETURN
// aims directly at a stationary rider and rider-relative course geometry is undefined at rest.
static unsigned long fm_return_candidate_since_ms = 0;
static unsigned long fm_return_start_ms            = 0;
static unsigned long fm_return_resume_since_ms     = 0;
static unsigned long fm_return_check_ms            = 0;
static float         fm_return_prev_dist_m         = -1.0f;

// Both normal RETURN exits lead to FM_ARMED and preserve the live F1-F4 declaration. If the trigger
// is still held at that edge, ARMED/manual throttle must remain inhibited until one release;
// otherwise changing cap 0 -> 255 could make the buggy surge at the rider. This interlock is not an
// FM lifecycle state and never restores the cleared separation proof.
static bool          fm_return_exit_hold           = false;

// ---- A3 fault-stop state (V2.5-Evo - 2026-07-20) ----
// millis() when FM entered FM_STOPPING; drives the 0 -> 255 fault ramp. 0 = not stopping.
static unsigned long fm_stop_ms          = 0;

// millis() of the last SURPRISING fault stop (a fault that occurred while the trigger was held).
// Drives the sticky fm_flags bit 3 for kFmFaultStickyMs so the TX cannot miss the stop
// notification across the ~2.4 s telemetry rotation. 0 = no recent surprising fault. Deliberately
// NOT cleared by fmEnterIdle() — the notification must survive the transition into FM_IDLE.
static unsigned long fm_fault_alarm_ms   = 0;

// Transient ACTIVE conditions exported in the two remaining fm_flags bits. These are not lifecycle
// states: clearing the geometry restores the same FM_ACTIVE session. The TX repeats one medium
// warning pulse every three seconds while either flag is present, even with the trigger released.
static bool          fm_geometry_warning = false;  // bit6: radial/separation geometry warning only
static bool          fm_front_warning    = false;  // bit7: F4 front-position warning only

// ---- Level-4 Follow-Me engage diagnostics ----
// runFmLoop() publishes one coherent snapshot on every 10 Hz control tick. The logger task only
// copies this snapshot; it never re-runs a gate or calls getRtmHeading(), so instrumentation cannot
// change controller state or report a decision from a different code path. The critical section is
// a few scalar copies and is entered by the logger only at the configured log rate (3 Hz default).
static portMUX_TYPE      fm_log_diag_mux = portMUX_INITIALIZER_UNLOCKED;
static FmLogDiagSnapshot fm_log_diag_snapshot = {
  0, 0xFFFF, 0xFFFF, 0xFFFF, 0, 0x7FFF,
  0xFF, (uint8_t)FM_IDLE, (uint8_t)FM_LOG_BLOCK_NO_DECLARATION, 255
};

static uint32_t    fm_diag_gate_flags       = 0;
static uint8_t     fm_diag_block_reason     = FM_LOG_BLOCK_NO_DECLARATION;
static float       fm_diag_distance_m       = -1.0f;
static float       fm_diag_d_engage_m       = -1.0f;
static float       fm_diag_front_angle_deg  = -1.0f;
static bool        fm_diag_return_candidate = false;
static bool        fm_diag_can_be_active    = false;
static bool        fm_diag_manual_steer     = false;
static bool        fm_diag_diverge_fault    = false;

static uint16_t fmDiagUnsignedDx10(float value)
{
  if (!isfinite(value) || value < 0.0f) return 0xFFFF;
  float scaled = value * 10.0f;
  if (scaled > 65534.0f) scaled = 65534.0f;
  return (uint16_t)lroundf(scaled);
}

static int16_t fmDiagSignedDx10(float value)
{
  if (!isfinite(value) || value < 0.0f) return 0x7FFF;
  float scaled = value * 10.0f;
  if (scaled > 32766.0f) scaled = 32766.0f;
  return (int16_t)lroundf(scaled);
}

static void fmPublishLogDiagSnapshot(unsigned long now)
{
  FmLogDiagSnapshot next = {};
  uint32_t gates = fm_diag_gate_flags;

  if (fm_sep_latched)             gates |= FM_LOG_GATE_SEP_LATCHED;
  if (fm_diag_return_candidate)   gates |= FM_LOG_GATE_RETURN_CANDIDATE;
  if (fm_min_dist_stop_latched)   gates |= FM_LOG_GATE_MIN_DIST_STOP;
  if (heading_disagree_fault)     gates |= FM_LOG_GATE_HEADING_DISAGREE;
  if (fm_geometry_warning)        gates |= FM_LOG_GATE_GEOMETRY_WARNING;
  if (fm_front_warning)           gates |= FM_LOG_GATE_FRONT_WARNING;
  if (fm_diag_can_be_active)      gates |= FM_LOG_GATE_CAN_BE_ACTIVE;
  if (fm_return_exit_hold)        gates |= FM_LOG_GATE_RETURN_EXIT_HOLD;
  if (fm_diag_manual_steer)       gates |= FM_LOG_GATE_MANUAL_STEER;
  if (fm_diag_diverge_fault)      gates |= FM_LOG_GATE_DIVERGENCE_FAULT;

  next.gate_flags       = gates;
  next.distance_dx10    = fmDiagUnsignedDx10(fm_diag_distance_m);
  next.d_engage_dx10    = fmDiagUnsignedDx10(fm_diag_d_engage_m);
  next.rider_speed_dx10 = fmDiagUnsignedDx10(fm_rider_speed_kmh);
  next.front_angle_dx10 = fmDiagSignedDx10(fm_diag_front_angle_deg);
  next.mode             = fm_mode_runtime.load(std::memory_order_relaxed);
  next.state            = (uint8_t)fm_state;
  next.block_reason     = fm_diag_block_reason;
  next.throttle_cap     = fm_throttle_cap.load(std::memory_order_relaxed);

  if (fm_sep_latched) {
    next.sep_dwell_ms = (uint16_t)kFmSepDwellMs;
  } else if (fm_sep_over_since_ms != 0) {
    unsigned long elapsed = now - fm_sep_over_since_ms;
    if (elapsed > kFmSepDwellMs) elapsed = kFmSepDwellMs;
    next.sep_dwell_ms = (uint16_t)elapsed;
  }

  portENTER_CRITICAL(&fm_log_diag_mux);
  fm_log_diag_snapshot = next;
  portEXIT_CRITICAL(&fm_log_diag_mux);
}

static bool fmReadLogDiagSnapshot(FmLogDiagSnapshot *out)
{
  if (out == NULL) return false;
  portENTER_CRITICAL(&fm_log_diag_mux);
  *out = fm_log_diag_snapshot;
  portEXIT_CRITICAL(&fm_log_diag_mux);
  return true;
}

// Stack guard: C++ destruction runs on every return path, including the many safety exits below.
// That makes "one snapshot per executed FM tick" structural rather than dependent on remembering a
// publish call beside every future return statement.
struct FmLogDiagPublishGuard {
  unsigned long now;
  ~FmLogDiagPublishGuard() { fmPublishLogDiagSnapshot(now); }
};

// V2.5-Evo - 2026-07-25 - A3: millis() when dist_m first exceeded the effective absolute limit while
// FM was ACTIVE; 0 = not currently beyond it. Counts the kFmDivergeMs dwell for the divergence fault.
// Reset discipline is copied from runPhaseC's rtm_prev_dist_m and from fm_sep_over_since_ms: cleared
// the moment the condition stops holding, whenever the distance is untrustworthy (trigger released,
// GPS stale/rejected, link down), whenever FM is not ACTIVE, and on entry to FM_IDLE. A half-finished
// proof is never carried across a data gap or a state change.
static unsigned long fm_diverge_since_ms = 0;

// V2.5-Evo - 2026-07-25 - F1: the buggy-to-rider distance in metres captured at the instant
// fm_diverge_since_ms started, i.e. the baseline the closure test compares against at dwell expiry.
// -1.0f = no dwell running / no baseline. This is the FM twin of runPhaseC's rtm_prev_dist_m: it turns
// the detector from "are you far?" (a level) into "are you failing to close?" (a derivative), which is
// what actually distinguishes a buggy running away from one that is following from further back than
// we would like. Cleared in lockstep with fm_diverge_since_ms everywhere, so a baseline can never
// outlive its own dwell or be compared against a distance from a different engagement.
static float         fm_diverge_start_dist_m = -1.0f;

// The computed trailing target point FM steers toward. Written by computeFmTarget() and
// read by updateRtmSteering() when fm_rx_active is set.
static double        fm_target_lat       = 0.0;
static double        fm_target_lng       = 0.0;

// ---- Compute FM steering override (P+D controller) ----
// V2.5-Evo - 2026-05-08 - Bundle 1: Replaced fixed ±90° clamp with preset-driven P+D controller.
// Added first-order low-pass filter on TX target position for FM path-following smoothness.
// Heading source still comes from getRtmHeading() (GPS COG primary, snapshot fallback).
// LOW-confidence sources reduce steering authority by 50% (unchanged from D5).
// Filter state + D-term reset on invalid heading to satisfy the heading-filter rule.
static void updateFmSteering()
{
  if (!usrConf.rtm_rx_override_steering) {
    rtm_steer_override = 127;
    // Reset D-term continuity statics here too: with override disabled we produce no
    // steering samples, so on a later off->on toggle the D-term must not differentiate
    // a fresh heading error against a stale pre-toggle sample across the gap (Kd spike).
    prev_heading_src_valid  = false;
    prev_heading_error_deg  = 0.0f;
    prev_steering_update_ms = 0;
    g_heading_error_dx10 = 0x7FFF;
    g_d_error_dx10 = 0x7FFF;
    return;
  }

  float current_heading;
  uint8_t confidence;
  bool valid = getRtmHeading(&current_heading, &confidence);

  if (!valid) {
    // No valid heading — hold straight. Reset filter + D-term state so we don't
    // resume with stale data on next cycle. (project rule)
    rtm_steer_override = 127;
    prev_heading_error_deg = 0.0f;
    prev_heading_src_valid = false;   // no same-source prior sample to differentiate against
    g_heading_error_dx10 = 0x7FFF;
    g_d_error_dx10 = 0x7FFF;
    return;
  }

  // Lookup active preset — clamp index defensively
  uint16_t idx = usrConf.rtm_steer_response;
  if (idx > 4) idx = 2;  // fallback to Normal on bad config
  const SteerPreset &p = kSteerPresets[idx];

  // ---- Bearing-target low-pass filter (for FM path-following) ----
  // First-order exponential moving average on TX position.
  // alpha = dt / (tau + dt). dt is loop period, tau is preset's filter time constant.
  unsigned long now = millis();
  float dt_s = (prev_steering_update_ms == 0) ? 0.1f : ((now - prev_steering_update_ms) / 1000.0f);
  if (dt_s <= 0.0f || dt_s > 1.0f) dt_s = 0.1f;  // sanity clamp
  prev_steering_update_ms = now;

  // The rider tracker already filters the target while ARMED, so applying a second position filter
  // here would add lag. FM_RETURN writes the live rider position into the same target variables.
  double steer_target_lat = fm_target_lat;
  double steer_target_lng = fm_target_lng;

  // Bearing from RX GPS to the selected FM target.
  double bearing_deg = TinyGPSPlus::courseTo(
      gps_last_lat, gps_last_lng, steer_target_lat, steer_target_lng);

  // Heading error (signed, wrapped to ±180°)
  float heading_error = (float)(bearing_deg - current_heading);
  while (heading_error >  180.0f) heading_error -= 360.0f;
  while (heading_error < -180.0f) heading_error += 360.0f;

  // Saturate (preset clamp angle)
  float clamped = heading_error;
  if (clamped >  p.error_clamp_deg) clamped =  p.error_clamp_deg;
  if (clamped < -p.error_clamp_deg) clamped = -p.error_clamp_deg;

  // P term (normalized to ±127 at full clamp)
  float p_term = (clamped / p.error_clamp_deg) * 127.0f * p.kp;

  // ---- D term: differentiate ONLY across consecutive same-source samples ----
  // Fable audit §5: when the heading source switches (GPS COG <-> compass) or the
  // compass snapshot re-snaps, heading_error steps by tens of degrees in a single
  // 100ms tick. Differentiating across that step injects a false ±300°/s rate into
  // Kd and commands a violent phantom turn. Build a source id that changes on a
  // source switch AND on a snapshot re-snap; skip the D term (d_error=0) whenever
  // the id differs from the previous sample so we never differentiate through a step.
  uint32_t heading_src_id;
  if (confidence == 3) {
    heading_src_id = 1;    // GPS COG — updates smoothly with motion, safe to differentiate
  } else if (usrConf.rtm_use_compass == 2) {
    heading_src_id = 2;    // live compass-only (diagnostic) — continuous reading
  } else {
    // Hybrid compass snapshot: the value is held constant until re-snapped. Fold the
    // snapshot timestamp into the id (high bit set so it can never collide with 1/2)
    // so each re-snap is treated as a new source and the D step across it is skipped.
    heading_src_id = 0x80000000UL | ((uint32_t)compass_snapshot_ms & 0x7FFFFFFFUL);
  }

  float d_error;
  if (prev_heading_src_valid && heading_src_id == prev_heading_src_id) {
    // heading_error lives on a circle. Subtracting its normalized representations directly
    // creates a false +/-360 deg jump at the branch cut (for example +179 -> -179). Differentiate
    // the shortest signed angular delta instead, so that example is +2 deg rather than -358 deg.
    float delta_error = heading_error - prev_heading_error_deg;
    while (delta_error >  180.0f) delta_error -= 360.0f;
    while (delta_error < -180.0f) delta_error += 360.0f;
    d_error = delta_error / dt_s;
  } else {
    d_error = 0.0f;  // source switched or snapshot re-snapped — do not differentiate across the step
  }
  float d_term = p.kd * d_error;
  prev_heading_error_deg = heading_error;
  prev_heading_src_id    = heading_src_id;
  prev_heading_src_valid = true;

  // Confidence: LOW conf reduces total authority by 50% (preserves D5 behavior)
  float authority = (confidence == 1) ? 0.5f : 1.0f;

  // d_error is the derivative of the same signed error used by P. Adding it provides damping:
  // while a corrective turn closes the error, d_error is negative and reduces the command.
  float output = 127.0f + authority * (p_term + d_term);
  if (output < 0.0f)   output = 0.0f;
  if (output > 254.0f) output = 254.0f;
  rtm_steer_override = (uint8_t)output;

  // Export for logger (with sentinel-safe conversion)
  g_heading_error_dx10 = (int16_t)(heading_error * 10.0f);
  g_d_error_dx10       = (int16_t)(d_error * 10.0f);
  if (g_heading_error_dx10 == 0x7FFF) g_heading_error_dx10 = 0x7FFE;  // avoid sentinel collision
  if (g_d_error_dx10       == 0x7FFF) g_d_error_dx10       = 0x7FFE;

  #ifdef DEBUG_RX
  Serial.printf("FM steer[%u]: bear=%.1f head=%.1f err=%.1f d_err=%.1f P=%.1f D=%.1f auth=%.2f ovr=%d\n",
                idx, (float)bearing_deg, current_heading, heading_error, d_error,
                p_term, d_term, authority, (int)rtm_steer_override);
  #endif
}


// ---- Shared FM navigation/telemetry service — call before runFmLoop() ----
// Standalone RTM no longer has a state or activation path. This 10 Hz service keeps the shared
// heading snapshot, FM telemetry and TX/RX distance current. The historical wire-field name
// rtm_distance is retained for packet-layout compatibility.
void runFmNavigationLoop()
{
  updateCompassSnapshot();

  static unsigned long last_nav_ms = 0;
  unsigned long now = millis();
  if (now - last_nav_ms < 100UL) return;
  last_nav_ms = now;

  // Fail closed against legacy state. 0xF1 is ignored by Radio.ino, and these assignments ensure
  // no old in-memory flag or cap can ever recreate the removed standalone mode.
  rtm_rx_active         = false;
  rtm_rx_emergency_stop = false;
  rtm_approach_cap      = 255;

  if (gps_last_course_deg >= 0.0f && gps_last_course_ms > 0 &&
      (now - gps_last_course_ms) < 3000UL) {
    telemetry.rx_heading = (uint8_t)((uint16_t)gps_last_course_deg / 2u);
  } else {
    telemetry.rx_heading = 0xFF;
  }

  if (g_heading_error_dx10 == 0x7FFF) {
    telemetry.fm_heading_err = 127;
  } else {
    int16_t e = g_heading_error_dx10 / 10;
    if (e < -126) e = -126;
    if (e >  126) e = 126;
    telemetry.fm_heading_err = (uint8_t)(e + 127);
  }

  // fm_status: [7]=aux2 [6]=aux1 [5]=VESC [4]=wetness [3:2]=heading confidence
  //            [1]=FM_RETURN [0]=FM automatic steering active
  bool fm_heading_available = false;
  {
    uint8_t st = 0;
    if (fm_rx_active)          st |= (1u << 0);
    if (fm_state == FM_RETURN) st |= (1u << 1);
    float unused_heading;
    uint8_t confidence;
    getRtmHeading(&unused_heading, &confidence);
    fm_heading_available = (confidence > 0);
    st |= (confidence & 0x03u) << 2;
    if (telemetry.error_code == 71) st |= (1u << 4);
    if ((now - last_uart_packet) < ((uint32_t)usrConf.vesc_timeout_s * 1000UL)) st |= (1u << 5);
    if (rx_aux_flags & (1u << 0)) st |= (1u << 6);
    if (rx_aux_flags & (1u << 1)) st |= (1u << 7);
    telemetry.fm_status = st;
  }

  // fm_flags: [0] armed [1] driving [2] not-ready [3] fault [4] return [5] reserved legacy done
  //           [6] radial/separation warning [7] F4 front-position warning (both warning-only).
  {
    uint8_t f = 0;
    FmState s = fm_state;
    if (s == FM_ARMED || s == FM_ACTIVE || s == FM_RETURN) f |= FM_FLAG_ARMED;
    if (fm_rx_active) f |= FM_FLAG_ENGAGED;
    if ((s == FM_ARMED || (s == FM_ACTIVE && !fm_rx_active)) &&
        (!fm_sep_latched || fm_min_dist_stop_latched || !fm_heading_available)) {
      f |= FM_FLAG_NOTREADY;
    }
    if (fm_fault_alarm_ms != 0 && (now - fm_fault_alarm_ms) < kFmFaultStickyMs) f |= FM_FLAG_FAULT;
    if (s == FM_RETURN) f |= FM_FLAG_RETURN;
    // FM_FLAG_DONE is retained in the packet ABI for compatibility with older firmware, but this RX
    // no longer emits it: a normal RETURN exit preserves the declaration and enters FM_ARMED.
    if (fm_geometry_warning) f |= FM_FLAG_GEOMETRY;
    if (fm_front_warning)    f |= FM_FLAG_FRONT_LOST;
    telemetry.fm_flags = f;
  }

  // Expire the pair-position approval if the remote stops refreshing its GPS.
  unsigned long phase_b_stale = (uint32_t)usrConf.tx_gps_stale_timeout_ms * 2UL;
  if (rx_tx_gps_timestamp == 0 || (now - rx_tx_gps_timestamp) > phase_b_stale) {
    gps_phase_b_ok = false;
  }

  // Keep the existing byte encoding because it is part of the telemetry packet ABI.
  bool gps_rx_ok = gps_last_ms > 0 && (now - gps_last_ms) < 6000UL;
  bool gps_tx_ok = rx_tx_gps_timestamp > 0 && (now - rx_tx_gps_timestamp) < 10000UL;
  if (gps_rx_ok && gps_tx_ok) {
    float d = (float)TinyGPSPlus::distanceBetween(
        gps_last_lat, gps_last_lng, rx_tx_gps_lat, rx_tx_gps_lng);
    if (d < 10.0f) {
      telemetry.rtm_distance = (uint8_t)(d * 10.0f);
    } else {
      uint8_t whole_m = (uint8_t)(d > 164.0f ? 164.0f : d);
      telemetry.rtm_distance = 90u + whole_m;
    }
    double bearing = TinyGPSPlus::courseTo(
        gps_last_lat, gps_last_lng, rx_tx_gps_lat, rx_tx_gps_lng);
    telemetry.rx_bearing_to_tx = (uint8_t)((uint16_t)bearing / 2u);
  }

  // Clear shared controller history only while FM has no steering ownership. The first active
  // FM/RETURN tick cold-starts the derivative explicitly, so no old target can leak into it.
  if (!fm_rx_active) {
    rtm_steer_override       = 127;
    prev_heading_error_deg   = 0.0f;
    prev_heading_src_valid   = false;
    prev_steering_update_ms  = 0;
    g_heading_error_dx10     = 0x7FFF;
    g_d_error_dx10           = 0x7FFF;
  }
}

// ============================================================
// FOLLOW-ME GEOMETRY HELPERS
// V2.5-Evo - 2026-07-19 - P3 (DESIGN_FOLLOW_ME.md section 6)
// ============================================================

// ------------------------------------------------------------
// projectPoint - move a lat/lng a given distance along a given compass bearing
// ------------------------------------------------------------
// What it does:
//   Standard spherical "destination point given start, bearing and distance" formula.
//   Used to place the lag anchor ahead of the rider and the trailing target behind them.
//
// Inputs:
//   lat, lng     - start position in degrees (WGS84)
//   bearing_deg  - direction to travel, degrees CLOCKWISE FROM NORTH (0=N, 90=E, 180=S, 270=W).
//                  This is the same bearing convention TinyGPSPlus::courseTo(), the compass,
//                  and GPS course-over-ground all use on this board.
//   dist_m       - distance to travel in metres
//
// Outputs:
//   *out_lat, *out_lng - the resulting position in degrees
//
// Side effects: none (pure function).
//
// Precision note: all maths is double, matching the project rule that FM position maths must
// never drop to float - float carries only ~7 significant digits, which is visible error at the
// sub-10 m distances FM steers by.
// ------------------------------------------------------------
static void projectPoint(double lat, double lng, float bearing_deg, float dist_m,
                         double* out_lat, double* out_lng)
{
  const double kEarthRadiusM = 6371000.0;

  double br     = (double)bearing_deg * M_PI / 180.0;   // bearing in radians
  double ang    = (double)dist_m / kEarthRadiusM;       // angular distance in radians
  double lat_r  = lat * M_PI / 180.0;
  double lng_r  = lng * M_PI / 180.0;

  double sin_lat = sin(lat_r);
  double cos_lat = cos(lat_r);
  double sin_ang = sin(ang);
  double cos_ang = cos(ang);

  double new_lat_r = asin(sin_lat * cos_ang + cos_lat * sin_ang * cos(br));
  double new_lng_r = lng_r + atan2(sin(br) * sin_ang * cos_lat,
                                   cos_ang - sin_lat * sin(new_lat_r));

  *out_lat = new_lat_r * 180.0 / M_PI;
  *out_lng = new_lng_r * 180.0 / M_PI;
}

// ------------------------------------------------------------
// fmAngleDiff - smallest absolute angle between two compass bearings
// ------------------------------------------------------------
// Inputs:  a, b - bearings in degrees (any range)
// Returns: the absolute difference wrapped into 0-180 degrees
// Side effects: none.
// ------------------------------------------------------------
static float fmAngleDiff(float a, float b)
{
  float d = a - b;
  while (d >  180.0f) d -= 360.0f;
  while (d < -180.0f) d += 360.0f;
  return fabsf(d);
}

// ------------------------------------------------------------
// fmFrontGeometry - rider-relative position for mode 4 (In Front)
// ------------------------------------------------------------
// Projects the rider->buggy vector onto the rider's course axis. A positive along-track value
// means the buggy is physically ahead of the rider; a negative value means it is behind. This is
// the safety distinction a radial distance cannot make. Cross-track is signed (right positive in
// this clockwise bearing convention) and off-axis is its absolute angular representation.
// A rider course is mandatory: while stationary there is no defensible meaning of "in front".
// ------------------------------------------------------------
static bool fmFrontGeometry(float* out_along_m, float* out_cross_m, float* out_off_axis_deg)
{
  *out_along_m      = 0.0f;
  *out_cross_m      = 0.0f;
  *out_off_axis_deg = 180.0f;

  if (fm_rider_course_deg < 0.0f) return false;

  float dist = (float)TinyGPSPlus::distanceBetween(
      rx_tx_gps_lat, rx_tx_gps_lng, gps_last_lat, gps_last_lng);
  float bearing = (float)TinyGPSPlus::courseTo(
      rx_tx_gps_lat, rx_tx_gps_lng, gps_last_lat, gps_last_lng);
  if (!isfinite(dist) || !isfinite(bearing)) return false;

  float rel = bearing - fm_rider_course_deg;
  while (rel >  180.0f) rel -= 360.0f;
  while (rel < -180.0f) rel += 360.0f;

  float rel_rad = rel * ((float)M_PI / 180.0f);
  *out_along_m      = dist * cosf(rel_rad);
  *out_cross_m      = dist * sinf(rel_rad);
  *out_off_axis_deg = fabsf(rel);
  return true;
}

// ------------------------------------------------------------
// updateFmRiderTracking - EMA-filter the rider position and derive course + speed
// ------------------------------------------------------------
// What it does (DESIGN_FOLLOW_ME.md section 6 steps 1-2):
//   1. Low-pass filters the raw rider position that arrives from the TX at 2 Hz in the 0xF3
//      meta-packet. Same first-order EMA and same preset time constant RTM uses, so the buggy
//      follows the rider's smoothed path and ignores individual carves.
//   2. Derives the rider's course and speed from two successive FILTERED positions.
//      An EMA does not change steady-state velocity (it only adds lag), so differentiating the
//      filtered track gives a clean speed while rejecting per-sample GPS jitter.
//      Below kFmCourseValidSpeedKmh the course is marked invalid (-1) and FM falls back to the
//      degraded hold-station geometry instead of chasing a meaningless heading.
//
// Called every FM tick (10 Hz) whether FM is ARMED, DEMOTED or ACTIVE, so that rider motion is
// already warm the instant the activation conditions are met.
//
// Inputs:  reads rx_tx_gps_lat/lng/timestamp, usrConf.rtm_steer_response
// Outputs: writes fm_filt_lat/lng, fm_rider_course_deg, fm_rider_speed_kmh and the
//          fm_prev_filt_* baseline.
// Side effects: none outside those module globals. Never touches the motor path.
// ------------------------------------------------------------
static void updateFmRiderTracking()
{
  // No rider position has ever arrived - nothing to track. Leave the last known motion alone.
  if (rx_tx_gps_timestamp == 0) return;

  unsigned long now = millis();

  // Use the active steering preset's filter time constant - the same tau RTM filters with.
  uint16_t idx = usrConf.rtm_steer_response;
  if (idx > 4) idx = 2;                       // defensive fallback to Normal on bad config
  float tau = kSteerPresets[idx].target_filter_tau_s;

  // Filter timestep, sanity-clamped exactly the way updateRtmSteering() clamps it.
  float dt_s = (fm_filt_prev_ms == 0) ? 0.1f : ((now - fm_filt_prev_ms) / 1000.0f);
  if (dt_s <= 0.0f || dt_s > 1.0f) dt_s = 0.1f;
  fm_filt_prev_ms = now;

  // ---- Step 1: EMA filter of the raw rider position ----
  if (!fm_filt_init) {
    fm_filt_lat  = rx_tx_gps_lat;
    fm_filt_lng  = rx_tx_gps_lng;
    fm_filt_init = true;
  } else if (tau > 0.0f) {
    float alpha = dt_s / (tau + dt_s);
    fm_filt_lat += alpha * (rx_tx_gps_lat - fm_filt_lat);
    fm_filt_lng += alpha * (rx_tx_gps_lng - fm_filt_lng);
  } else {
    fm_filt_lat = rx_tx_gps_lat;
    fm_filt_lng = rx_tx_gps_lng;
  }

  // ---- Step 2: derive rider course + speed from successive filtered positions ----
  if (fm_prev_filt_ms == 0) {
    // First sample - just seed the baseline, no motion available yet.
    fm_prev_filt_lat = fm_filt_lat;
    fm_prev_filt_lng = fm_filt_lng;
    fm_prev_filt_ms  = now;
    return;
  }

  float dt2 = (now - fm_prev_filt_ms) / 1000.0f;
  if (dt2 < kFmMotionBaselineS) return;   // baseline too short for a stable course - wait

  float d_m = (float)TinyGPSPlus::distanceBetween(
      fm_prev_filt_lat, fm_prev_filt_lng, fm_filt_lat, fm_filt_lng);

  fm_rider_speed_kmh = (d_m / dt2) * 3.6f;

  if (fm_rider_speed_kmh >= kFmCourseValidSpeedKmh) {
    fm_rider_course_deg = (float)TinyGPSPlus::courseTo(
        fm_prev_filt_lat, fm_prev_filt_lng, fm_filt_lat, fm_filt_lng);
  } else {
    // Too slow for a trustworthy course - degrade to hold-station geometry (no diagonal).
    fm_rider_course_deg = -1.0f;
  }

  fm_prev_filt_lat = fm_filt_lat;
  fm_prev_filt_lng = fm_filt_lng;
  fm_prev_filt_ms  = now;
}

// ------------------------------------------------------------
// computeFmTarget - compute the selected Follow-Me steering point
// ------------------------------------------------------------
// What it does (DESIGN_FOLLOW_ME.md section 6 steps 3-4 and 6):
//
//   d_follow = min_dist_m + followme_smoothing_band_m   (owner default 4 + 2 = 6 m)
//
//   1. LAG ANCHOR. A first-order filter always trails a moving target by roughly v * tau
//      (13-18 m at 15-20 mph with tau = 2 s - larger than the follow gap itself, and it grows
//      with speed). If we simply sat d_follow behind the filtered position the buggy would fall
//      further behind the faster the rider went. So we first push an anchor point FORWARD along
//      the rider's course by min(v_rider * tau, 2 * d_follow), which cancels the filter lag and
//      makes the geometry speed-independent. The cap at 2 * d_follow stops a bad speed estimate
//      from throwing the anchor far up the track. This is deliberately NOT solved by shrinking
//      tau: the filter has to keep ignoring the rider's carves.
//
//   2. TRAILING POINT. target = anchor + d_follow at bearing (course + 180 + offset).
//
//   3. SIDE-ZONE SCHMITT. The diagonal offset is only applied while the buggy is reasonably
//      lined up behind the rider. We measure how far off the directly-behind axis the buggy
//      currently sits and run a Schmitt trigger on it (engage below zone_angle_enter_deg,
//      release above zone_angle_exit_deg, owner defaults 35 / 45). Outside the zone we fall
//      back to pure-behind. Without this hysteresis an unstable rider course could whip the
//      target point across the wake from one side to the other.
//
//   DEGRADED MODE. If the rider is too slow for a valid course, there is no meaningful "behind".
//   We hold station instead: put the target d_follow from the rider along the current
//   rider->buggy bearing, with no diagonal. The buggy holds its distance without manoeuvring
//   around what may be a rider in the water.
//
// ============================================================
// !!! OFFSET SIGN CONVENTION - VERIFY BEFORE WATER !!!
//
// This whole board works in ONE bearing convention: degrees CLOCKWISE FROM NORTH
// (0 = North, 90 = East, 180 = South, 270 = West). TinyGPSPlus::courseTo(),
// getCompassHeading() and GPS course-over-ground all return that convention, and
// projectPoint() above consumes it. Adding degrees to a bearing therefore rotates
// CLOCKWISE.
//
// Given that, with the rider travelling along course C:
//   directly behind the rider  = bearing C + 180
//   the rider's right-hand side = bearing C + 90   (clockwise from their heading)
//   the rider's left-hand side  = bearing C - 90
//
// So a "behind and to the rider's RIGHT" spot lies between C+180 and C+90, i.e. at
// C + 180 - near_diag_offset_deg. A "behind and to the rider's LEFT" spot lies between
// C+180 and C+270, i.e. at C + 180 + near_diag_offset_deg.
//
// Written as target_bearing = C + 180 + offset, that gives:
//   mode 1 Near-Right -> offset = -near_diag_offset_deg     (NEGATIVE)
//   mode 2 Behind     -> offset = 0
//   mode 3 Near-Left  -> offset = +near_diag_offset_deg     (POSITIVE)
//
// which matches DESIGN_FOLLOW_ME.md section 6 step 4 exactly. The single line that sets
// this sign is the "offset = -/+ usrConf.near_diag_offset_deg" pair below.
//
// ASSUMPTION BEING MADE: "Near-Right" means to the RIDER'S right as the rider faces along
// their direction of travel (not the buggy's right, and not the right of someone watching
// from the beach). Default mode 1 exists because the owner rides left-foot-forward, which
// puts the right side on their open/visible side - consistent with rider-relative.
//
// This must be confirmed against Tools/FollowMe Settings Visualizer.html before any water
// test. If the visualizer shows the mirror image, flip ONLY the two signs below - no other
// part of the geometry depends on this choice.
// ============================================================
//
// Inputs:  reads fm_filt_lat/lng, fm_rider_course_deg, fm_rider_speed_kmh, gps_last_lat/lng,
//          usrConf FM params, fm_mode_runtime
// Outputs: *out_lat / *out_lng - the target point to steer at
// Side effects: updates the fm_diagonal_engaged Schmitt latch.
// ------------------------------------------------------------
static void computeFmTarget(double* out_lat, double* out_lng)
{
  // Follow gap = hard-stop radius plus the smoothing band (owner default 4 + 2 = 6 m).
  float d_follow = usrConf.min_dist_m + usrConf.followme_smoothing_band_m;
  if (d_follow < 0.5f) d_follow = 0.5f;   // guard against a degenerate config

  uint8_t m = fm_mode_runtime.load(std::memory_order_relaxed);

  // ---- Degraded mode: no trustworthy rider course ----
  if (fm_rider_course_deg < 0.0f) {
    // F4 geometry is warning-only, so a missing rider course must not withdraw control. Keep the
    // buggy going straight along its own trusted heading until the rider course returns. This does
    // not pretend to know where "front" is; fm_front_warning stays asserted for that whole interval.
    if (m == 4) {
      float buggy_heading = 0.0f;
      uint8_t heading_confidence = 0;
      if (getRtmHeading(&buggy_heading, &heading_confidence)) {
        projectPoint(gps_last_lat, gps_last_lng, buggy_heading, d_follow, out_lat, out_lng);
      } else {
        // checkFmFaultConditions() normally prevents this same-tick race. If the source disappears
        // between the two calls, a coincident target plus the align cap is the subtract-only fallback.
        *out_lat = gps_last_lat;
        *out_lng = gps_last_lng;
      }
      return;
    }
    float b_rider_to_buggy = (float)TinyGPSPlus::courseTo(
        fm_filt_lat, fm_filt_lng, gps_last_lat, gps_last_lng);
    projectPoint(fm_filt_lat, fm_filt_lng, b_rider_to_buggy, d_follow, out_lat, out_lng);
    return;
  }

  float course = fm_rider_course_deg;

  // ---- Lag anchor: push forward along the rider's course to cancel the filter lag ----
  uint16_t idx = usrConf.rtm_steer_response;
  if (idx > 4) idx = 2;
  float tau     = kSteerPresets[idx].target_filter_tau_s;
  float v_ms    = fm_rider_speed_kmh / 3.6f;
  float lag_m   = v_ms * tau;
  float max_lag = 2.0f * d_follow;
  if (lag_m > max_lag) lag_m = max_lag;
  if (lag_m < 0.0f)    lag_m = 0.0f;

  double anchor_lat, anchor_lng;
  projectPoint(fm_filt_lat, fm_filt_lng, course, lag_m, &anchor_lat, &anchor_lng);

  // ---- Mode 4 In Front: station point plus a forward steering lookahead ----
  // The buggy may reach the station point itself. Steering directly at a coincident GPS point
  // makes the bearing noise-dominated and can flip it by 180 degrees, so F4 aims farther along the
  // same course. The station distance still comes entirely from the existing min_dist + band
  // geometry; the derived lookahead adds no config field and changes no throttle target.
  if (m == 4) {
    float lookahead_m = usrConf.followme_smoothing_band_m;
    float half_follow = 0.5f * d_follow;
    if (lookahead_m < half_follow) lookahead_m = half_follow;
    if (lookahead_m < 2.0f)        lookahead_m = 2.0f;
    if (lookahead_m > d_follow)    lookahead_m = d_follow;

    // Never place the steering point behind a buggy that is already farther ahead than its
    // station. That would command a U-turn back toward the rider. Excess lead is removed only by
    // the speed cap; steering continues forward along the rider's course.
    float steer_distance_m = d_follow + lookahead_m;

    // Measure the buggy from the SAME lag-compensated anchor the target is projected from. Using
    // rider-relative along distance here would not be sufficient when the lag cap leaves the anchor
    // behind the raw rider position: the nominally "ahead" target could then still land beside or
    // behind the buggy. Same-origin projection makes the guarantee geometric, not approximate.
    float anchor_to_buggy_m = (float)TinyGPSPlus::distanceBetween(
        anchor_lat, anchor_lng, gps_last_lat, gps_last_lng);
    float anchor_to_buggy_bearing = (float)TinyGPSPlus::courseTo(
        anchor_lat, anchor_lng, gps_last_lat, gps_last_lng);
    if (isfinite(anchor_to_buggy_m) && isfinite(anchor_to_buggy_bearing)) {
      float rel = anchor_to_buggy_bearing - course;
      while (rel >  180.0f) rel -= 360.0f;
      while (rel < -180.0f) rel += 360.0f;
      float anchor_to_buggy_along_m = anchor_to_buggy_m *
          cosf(rel * ((float)M_PI / 180.0f));
      float ahead_of_buggy_m = anchor_to_buggy_along_m + lookahead_m;
      if (ahead_of_buggy_m > steer_distance_m) steer_distance_m = ahead_of_buggy_m;
    } else {
      // Defensive finite-data fallback. Aim from the buggy itself, so even this exceptional tick
      // cannot manufacture a target behind it.
      projectPoint(gps_last_lat, gps_last_lng, course, lookahead_m, out_lat, out_lng);
      return;
    }
    projectPoint(anchor_lat, anchor_lng, course, steer_distance_m, out_lat, out_lng);
    return;
  }

  // ---- Side-zone Schmitt: is the buggy lined up enough behind the rider to use the diagonal? ----
  float b_rider_to_buggy = (float)TinyGPSPlus::courseTo(
      fm_filt_lat, fm_filt_lng, gps_last_lat, gps_last_lng);
  float off_axis = fmAngleDiff(b_rider_to_buggy, course + 180.0f);

  if (!fm_diagonal_engaged && off_axis < usrConf.zone_angle_enter_deg) {
    fm_diagonal_engaged = true;
  } else if (fm_diagonal_engaged && off_axis > usrConf.zone_angle_exit_deg) {
    fm_diagonal_engaged = false;
  }

  // ---- Trailing point. See the OFFSET SIGN CONVENTION block above before touching these signs. ----
  // V2.5-Evo - 2026-07-20 - R0: the "0xFF falls back to usrConf.followme_mode" line was removed
  // here. 0xFF means the TX has never declared a mode this session; runFmLoop() now sends that
  // straight to FM_IDLE, so this function cannot be reached with m == 0xFF. If it somehow were,
  // neither branch below matches and the offset stays 0 (plain Behind) — the safe geometry.
  float offset = 0.0f;                                          // mode 2 Behind
  if (fm_diagonal_engaged) {
    if (m == 1)      offset = -(float)usrConf.near_diag_offset_deg;   // mode 1 Near-Right
    else if (m == 3) offset = +(float)usrConf.near_diag_offset_deg;   // mode 3 Near-Left
  }

  float target_bearing = course + 180.0f + offset;
  projectPoint(anchor_lat, anchor_lng, target_bearing, d_follow, out_lat, out_lng);
}

// ------------------------------------------------------------
// checkFmFaultConditions - the FM FAULT conditions (2-7)
// ------------------------------------------------------------
// What it does (DESIGN_FOLLOW_ME.md section 5, transient-inhibit-vs-fault classification):
//   Evaluates the six FAULT conditions only - the ones that mean something actually BROKE, so FM
//   must end for the run and a fresh declaration is required to re-arm. Two conditions are handled
//   by the CALLER, not here, because they are not faults:
//     - Condition 1 (throttle >= 25) is the DEADMAN. A trigger release is never a fault (treating
//       it as one would end FM on every release, worse than the original bug); the caller reads it
//       as thr_held and the motor is already 0 by the base architecture when it is low.
//     - Follow geometry is warning-only. It never changes steering authority, throttle cap, state or
//       separation proof. The retired configurable rider-speed gate no longer participates; only the
//       fixed <2 km/h RETURN proof can temporarily hold the buggy while it matures.
//   Like RTM, any one of these six failing means FM must not be steering. This function does NOT
//   set rtm_rx_emergency_stop - FM stops the motor through its own fm_throttle_cap so the two
//   systems can never fight over one flag.
//
// Returns: true only if all six fault conditions hold.
// Side effects: none (read-only on all globals).
// ------------------------------------------------------------
static bool checkFmFaultConditions()
{
  unsigned long now = millis();

  // 2. Phase A: the RX's own GPS has not been rejected as implausible/spoofed.
  if (gps_rejected) {
    fm_diag_block_reason = FM_LOG_BLOCK_GPS_REJECTED;
    return false;
  }
  fm_diag_gate_flags |= FM_LOG_GATE_GPS_NOT_REJECTED;

  // 3. Phase B: the TX<->RX cross-validation handshake is currently passing.
  if (!gps_phase_b_ok) {
    fm_diag_block_reason = FM_LOG_BLOCK_PHASE_B;
    return false;
  }
  fm_diag_gate_flags |= FM_LOG_GATE_PHASE_B_OK;

  // 4. The rider's (TX) GPS position is fresh.
  if (rx_tx_gps_timestamp == 0 ||
      (now - rx_tx_gps_timestamp) > (uint32_t)usrConf.tx_gps_stale_timeout_ms) {
    fm_diag_block_reason = FM_LOG_BLOCK_TX_GPS_STALE;
    return false;
  }
  fm_diag_gate_flags |= FM_LOG_GATE_TX_GPS_FRESH;

  // 5. The buggy's (RX) GPS position is fresh (same 6 s window as RTM gate 5).
  if (gps_last_ms == 0 || (now - gps_last_ms) > 6000UL) {
    fm_diag_block_reason = FM_LOG_BLOCK_RX_GPS_STALE;
    return false;
  }
  fm_diag_gate_flags |= FM_LOG_GATE_RX_GPS_FRESH;

  // 6. A valid heading source exists. V2.5-Evo - 2026-07-20 - A3: FM ALWAYS requires a heading
  //    source, regardless of rtm_compass_required. That flag was an RTM-arming convenience; a
  //    missing heading source in FM means the buggy would steer blind at the ~5% align cap, which
  //    is a FAULT, not something to silently permit. This is the R4 heading-source-loss fix.
  {
    float h_unused; uint8_t conf_unused;
    if (!getRtmHeading(&h_unused, &conf_unused)) {
      // A disagreement is not an engage gate. getRtmHeading() has already tried live GPS COG and
      // the short held-COG bridge before excluding the suspect compass. Reaching this branch means
      // there is genuinely no usable GPS-derived heading, so report that fact; the independent
      // fm_heading_disagree column still records why the compass fallback was unavailable.
      fm_diag_block_reason = FM_LOG_BLOCK_NO_HEADING;
      return false;
    }
    fm_diag_gate_flags |= FM_LOG_GATE_HEADING_OK;
  }

  // 7. The LoRa link is healthy.
  if (now - last_packet > usrConf.failsafe_time) {
    fm_diag_block_reason = FM_LOG_BLOCK_LINK;
    return false;
  }
  fm_diag_gate_flags |= FM_LOG_GATE_LINK_OK;

  return true;
}

// Cold-start the F1-F4 speed governor. Called at every genuine automatic-control edge and while FM
// is idle. Starting the learned cap open at 255 avoids suppressing acceleration below the requested
// speed; the engage ramp remains the independent, subtract-only activation limiter.
static void fmResetSpeedGovernor()
{
  fm_speed_gov_init            = false;
  fm_speed_gov_mode            = 0xFF;
  fm_speed_gov_last_gps_ms     = 0;
  fm_speed_filtered_kmh        = 0.0f;
  fm_speed_target_filtered_kmh = 0.0f;
  fm_speed_integrator          = 255.0f;
  fm_speed_cap_slewed          = 255.0f;
  fm_speed_other_cap_active    = false;
}

// Compute the requested F1-F4 vehicle speed. F1-F3 may close at rider speed + 10 km/h. F4 varies
// between rider speed - 10 and rider speed + 10 from signed along-track gap error. In every mode a
// non-zero boogie_vmax is an absolute ceiling; zero skips only that clamp and never disables the
// rider-relative governor.
static float fmSpeedTargetKmh(float front_along_m, uint8_t mode)
{
  float rider_kmh = fm_rider_speed_kmh;
  if (!isfinite(rider_kmh) || rider_kmh < 0.0f) rider_kmh = 0.0f;

  float target_kmh;
  if (mode == 4) {
    float d_front = usrConf.min_dist_m + usrConf.followme_smoothing_band_m;
    if (d_front < 0.5f) d_front = 0.5f;

    float control_band = usrConf.followme_smoothing_band_m;
    if (control_band < 1.0f) control_band = 1.0f;

    float along_m = isfinite(front_along_m) ? front_along_m : 0.0f;
    float correction = (d_front - along_m) / control_band;
    if (correction >  1.0f) correction =  1.0f;
    if (correction < -1.0f) correction = -1.0f;
    target_kmh = rider_kmh + (kFmClosingMarginKmh * correction);
  } else {
    target_kmh = rider_kmh + kFmClosingMarginKmh;
  }

  if (target_kmh < 0.0f) target_kmh = 0.0f;
  float absolute_max_kmh = usrConf.boogie_vmax_in_followme_kmh;
  if (isfinite(absolute_max_kmh) && absolute_max_kmh > 0.1f &&
      target_kmh > absolute_max_kmh) {
    target_kmh = absolute_max_kmh;
  }
  return target_kmh;
}

// Stateful PI speed limiter for F1-F4. It updates only when gps_last_ms changes. The integrator
// learns the cap required at zero speed error instead of forcing cap 0 at the requested speed.
// Anti-windup blocks positive integration while approach/align/engage is the tighter cap; negative
// error may still remove stored cap. A separate 2 km/h overspeed band is the deterministic backstop.
static uint16_t fmComputeSpeedGovernorCap(float front_along_m, uint8_t mode)
{
  float raw_target_kmh = fmSpeedTargetKmh(front_along_m, mode);
  float raw_speed_kmh  = gps_last_speed_kmh;
  if (!isfinite(raw_speed_kmh) || raw_speed_kmh < 0.0f) raw_speed_kmh = 0.0f;

  if (!fm_speed_gov_init) {
    fm_speed_gov_init             = true;
    fm_speed_gov_mode             = mode;
    fm_speed_gov_last_gps_ms      = gps_last_ms;
    fm_speed_filtered_kmh         = raw_speed_kmh;
    fm_speed_target_filtered_kmh  = raw_target_kmh;
    fm_speed_integrator           = 255.0f;
    fm_speed_cap_slewed           = 255.0f;
    fm_speed_other_cap_active     = false;
  } else if (fm_speed_gov_mode != mode) {
    // Bumpless mode transfer: the target filter moves to the new geometry immediately, while the
    // learned cap starts from the value that was actually being exposed before the mode change.
    fm_speed_gov_mode             = mode;
    fm_speed_target_filtered_kmh  = raw_target_kmh;
    fm_speed_integrator           = fm_speed_cap_slewed;
    fm_speed_other_cap_active     = false;
  }

  bool fresh_speed_sample = (gps_last_ms != 0 && gps_last_ms != fm_speed_gov_last_gps_ms);
  if (fresh_speed_sample) {
    float dt_s = (float)(gps_last_ms - fm_speed_gov_last_gps_ms) / 1000.0f;
    if (dt_s < 0.05f || dt_s > 1.5f) dt_s = 0.1f;
    fm_speed_gov_last_gps_ms = gps_last_ms;

    float speed_alpha = dt_s / (kFmSpeedFilterTauS + dt_s);
    float target_alpha = dt_s / (kFmSpeedTargetFilterTauS + dt_s);
    fm_speed_filtered_kmh += speed_alpha * (raw_speed_kmh - fm_speed_filtered_kmh);
    fm_speed_target_filtered_kmh +=
        target_alpha * (raw_target_kmh - fm_speed_target_filtered_kmh);

    float error_kmh = fm_speed_target_filtered_kmh - fm_speed_filtered_kmh;
    if (fabsf(error_kmh) < kFmSpeedDeadbandKmh) error_kmh = 0.0f;

    float p_term = kFmSpeedKp * error_kmh;
    float unsaturated = fm_speed_integrator + p_term;
    bool pushes_high = (unsaturated >= 255.0f && error_kmh > 0.0f);
    bool pushes_low  = (unsaturated <=   0.0f && error_kmh < 0.0f);
    bool blocked_by_other_cap = fm_speed_other_cap_active && error_kmh > 0.0f;

    if (!pushes_high && !pushes_low && !blocked_by_other_cap) {
      fm_speed_integrator += kFmSpeedKi * error_kmh * dt_s;
      if (fm_speed_integrator > 255.0f) fm_speed_integrator = 255.0f;
      if (fm_speed_integrator <   0.0f) fm_speed_integrator =   0.0f;
    }

    float requested_cap = fm_speed_integrator + p_term;
    if (requested_cap > 255.0f) requested_cap = 255.0f;
    if (requested_cap <   0.0f) requested_cap =   0.0f;

    float max_rise = kFmSpeedCapRisePerS * dt_s;
    float max_fall = kFmSpeedCapFallPerS * dt_s;
    if (requested_cap > fm_speed_cap_slewed + max_rise) {
      fm_speed_cap_slewed += max_rise;
    } else if (requested_cap < fm_speed_cap_slewed - max_fall) {
      fm_speed_cap_slewed -= max_fall;
    } else {
      fm_speed_cap_slewed = requested_cap;
    }
  }

  if (fm_speed_target_filtered_kmh <= 0.1f) return 0;

  float cap = fm_speed_cap_slewed;
  float overspeed_kmh = fm_speed_filtered_kmh - fm_speed_target_filtered_kmh;
  if (overspeed_kmh > 0.0f) {
    float overspeed_cap = 255.0f *
        (1.0f - (overspeed_kmh / kFmSpeedOverspeedBandKmh));
    if (overspeed_cap < 0.0f) overspeed_cap = 0.0f;
    if (overspeed_cap < cap) cap = overspeed_cap;
  }

  if (cap > 255.0f) cap = 255.0f;
  if (cap <   0.0f) cap =   0.0f;
  return (uint16_t)cap;
}

// ------------------------------------------------------------
// fmComputeThrottleCap - the FM throttle cap chain
// ------------------------------------------------------------
// What it does (DESIGN_FOLLOW_ME.md section 7):
//   Runs five independent caps and returns the LOWEST. Every cap can only ever reduce the
//   rider's throttle - none of them can raise it. calcPWM() then applies the result with a
//   plain "if (cap < throttle) throttle = cap", so the human trigger stays the only throttle
//   source and FM can only subtract.
//
//   Cap 1 Hard stop      - dist <= min_dist_m. Handled by the caller as a stop latch: cap 0 remains
//                          until trigger release, then the separation proof must be rebuilt.
//   Cap 2 Approach ramp  - F1-3: linear 255 -> 0 across the smoothing band, same shape as RTM's
//                          approach decel zone. F4 omits it because slowing while the rider catches
//                          the buggy would collapse the front gap; the hard stop still applies.
//   Cap 3 Speed governor - stateful PI limiter. F1-3 target rider speed + closing margin. F4 varies
//                          that target around rider speed from signed along-track error. A non-zero
//                          boogie_vmax clamps the target; zero skips only that absolute clamp. The
//                          learned holding cap remains non-zero at the requested speed, and a hard
//                          overspeed band removes cap between target and target + 2 km/h.
//   Cap 4 Align phase    - while the heading error is large, clamp to ~5% so the buggy pivots
//                          toward the target instead of driving away from it.
//   Cap 5 Engage ramp    - 0 -> full over kFmEngageRampMs on every entry into FM_ACTIVE, so
//                          engaging and re-engaging is always a smooth build, never a jump.
//
// Inputs:  dist_m - radial buggy-to-rider distance; front_along_m - signed F4 forward distance;
//          mode - active geometry; now - millis() for this tick
// Returns: the winning cap, 0-255
// Side effects: updates the F1-F4 speed PI/filter state.
// ------------------------------------------------------------
static uint16_t fmComputeThrottleCap(float dist_m, float front_along_m,
                                     uint8_t mode, unsigned long now)
{
  uint16_t cap   = 255;                                     // start uncapped, take the lowest
  float min_dist = usrConf.min_dist_m;
  float band     = usrConf.followme_smoothing_band_m;
  bool  in_front = (mode == 4);

  // ---- Cap 2: approach ramp across the smoothing band ----
  // Behind modes slow as the buggy approaches the rider. F4 deliberately does not apply this
  // radial ramp: when a buggy ahead is being caught, slowing it makes the front gap collapse even
  // faster. F4 instead controls the along-track gap in the speed governor below; the same radial
  // min_dist hard stop remains enforced by runFmLoop().
  if (!in_front && band > 0.01f && dist_m < (min_dist + band)) {
    float frac = (dist_m - min_dist) / band;                // 1.0 at the outer edge, 0.0 at the stop radius
    if (frac < 0.0f) frac = 0.0f;
    if (frac > 1.0f) frac = 1.0f;
    uint16_t c = (uint16_t)(frac * 255.0f);
    if (c < cap) cap = c;
  }

  // ---- Cap 3: stateful PI speed governor ----
  uint16_t speed_cap = fmComputeSpeedGovernorCap(front_along_m, mode);
  if (speed_cap < cap) cap = speed_cap;

  // ---- Cap 4: align phase ----
  float abs_err = (g_heading_error_dx10 != 0x7FFF) ?
      fabsf((float)g_heading_error_dx10 / 10.0f) : 180.0f;  // no heading data -> treat as worst case
  if (abs_err > (float)usrConf.rtm_align_threshold_deg && kFmAlignCap < cap) {
    cap = kFmAlignCap;
  }

  // ---- Cap 5: engage ramp ----
  if (fm_engage_ms > 0) {
    unsigned long elapsed = now - fm_engage_ms;
    if (elapsed < kFmEngageRampMs) {
      uint16_t c = (uint16_t)(((float)elapsed / (float)kFmEngageRampMs) * 255.0f);
      if (c < cap) cap = c;
    }
  }

  // Feed the final arbitration result back into the next PI update. Positive integration is frozen
  // while approach, align or engage is the tighter cap, so the speed integrator cannot wind up behind
  // another limiter. Overspeed correction remains allowed because it only removes stored authority.
  fm_speed_other_cap_active = (cap < speed_cap);

  return cap;
}

// ------------------------------------------------------------
// fmEnterIdle - drop FM fully out of the control path
// ------------------------------------------------------------
// Used when FM is switched off (mode 0), when RTM arms and takes the buggy, or when GPS/RTM
// is disabled in config. Clears the throttle cap back to 255 so the rider's manual throttle
// passes through completely untouched, drops the steering override, and cold-starts the rider
// tracking so the next arm begins from a clean filter rather than a stale position.
//
// Deliberately does NOT touch rtm_steer_override or the shared P+D statics - if we are here
// because RTM just armed, RTM now owns those.
//
// V2.5-Evo - 2026-08-17 - AND IF NOBODY OWNS THEM, runRtmLoop() DOES THE CLEANUP. Leaving the
// override alone here is right (we cannot know whether RTM has just taken it), but on its own it
// used to mean a value from the run that just ended stayed parked for the next one. The neutral
// reset for the "nothing engaged at all" case now lives in runRtmLoop(), gated on both
// rtm_rx_active and fm_rx_active being false - see the block there for why it cannot be gated on
// RTM alone.
// ------------------------------------------------------------
static void fmEnterIdle()
{
  // V2.5-Evo - 2026-08-16 - Was FM already idle when we got here? runFmLoop() calls this function
  // on EVERY 100 ms tick while FM is idle (it is the level-triggered "not eligible" branch), so
  // "we are in fmEnterIdle" does NOT mean "an engagement just ended". Everything else below is
  // idempotent and can safely be re-applied ten times a second; the heading-disagreement latch at
  // the bottom is the one thing that must not be, so it is gated on this edge. See the block there.
  bool fm_was_not_idle = (fm_state != FM_IDLE);

  fm_state            = FM_IDLE;
  fm_rx_active        = false;
  fm_throttle_cap     = 255;          // no cap - fully manual buggy
  fm_geometry_warning = false;
  fm_front_warning    = false;
  fm_diagonal_engaged = false;
  fm_engage_ms        = 0;
  fm_filt_init        = false;
  fm_filt_prev_ms     = 0;
  fm_prev_filt_ms     = 0;
  fm_rider_course_deg = -1.0f;
  fm_rider_speed_kmh  = 0.0f;
  fmResetSpeedGovernor();

  // V2.5-Evo - 2026-07-20 - R1/R2: leaving FM entirely drops the separation proof with it.
  // Whatever put us here (mode 0, RTM preemption, mode-age expiry, GPS/FM disabled) ends the
  // declaration, so the next arm must re-prove separation from scratch before FM may engage.
  fm_sep_latched       = false;
  fm_sep_over_since_ms = 0;
  fm_min_dist_stop_latched = false;
  fm_return_candidate_since_ms = 0;
  fm_return_start_ms            = 0;
  fm_return_resume_since_ms     = 0;
  fm_return_check_ms            = 0;
  fm_return_prev_dist_m         = -1.0f;
  fm_return_exit_hold           = false;

  // V2.5-Evo - 2026-07-20 - A3: clear the fault-ramp clock.
  // fm_fault_alarm_ms is deliberately NOT reset here: the surprise-gated stop notification must
  // stay sticky for kFmFaultStickyMs even after FM has dropped into FM_IDLE.
  fm_stop_ms = 0;

  // V2.5-Evo - 2026-07-25 - A3: leaving FM drops any part-accumulated divergence proof with it, so
  // the next engagement starts its 3 s window from scratch rather than inheriting a stale timer.
  // F1: the closure baseline is cleared in the same breath — a distance measured during the previous
  // engagement must never be the yardstick for the next one.
  fm_diverge_since_ms     = 0;
  fm_diverge_start_dist_m = -1.0f;

  // V2.5-Evo - 2026-08-17 - THE FAULT IS NO LONGER FORGIVEN HERE. This site used to clear
  // heading_disagree_fault on a genuine transition into FM idle, on the reasoning that ending a
  // declaration should let the next arm start from fresh sensor evidence. But an FM declaration
  // ending is not evidence about a compass: the compass is exactly as wrong after it as before,
  // and clearing here made the rider re-prove a mis-mounted compass by coasting before every
  // single run — which, given the proof needs kHeadingDisagreeMs of continuous measurement in a
  // window that only exists while coasting, made the guard nearly unreachable.
  //
  // The latch is not an FM engagement block: Return and Follow-Me may both use valid GPS COG while
  // it stands. Keeping it across state boundaries prevents the compass fallback from being restored
  // without evidence. Coasting above rtm_cog_min_speed_kmh with the trigger released refreshes the
  // compass snapshot and permits a sustained agreement measurement to clear it.
  //
  // The verdict now survives. Its clear routes are evidence: a sustained agreement measurement or
  // a successful ?compasscal / ?magalign. Reboot alone does not clear the persisted verdict.
  //
  // The UNFINISHED dwell is still dropped here, on the same FM-idle edge as before, so a half-built
  // proof cannot span an FM declaration and whatever comes after it.
  //
  // HISTORY BELOW, kept because it is what constrains the edge test that remains.
  // V2.5-Evo - 2026-08-16 - EDGE, NOT LEVEL. WHAT THE BUG WAS: this clear ran on every call, and
  // runFmLoop() calls fmEnterIdle() on every 100 ms tick whenever FM is idle — and its FM_IDLE
  // condition includes rtm_rx_active. So THE WHOLE TIME RTM WAS RUNNING, the latch and its dwell
  // timer were being zeroed ten times a second. The 5 s dwell could never accumulate, the fault
  // could never be set, and the check that consumes it in getRtmHeading() (in front of the compass
  // fallback) could never see anything: in RTM-only operation — exactly the field case this
  // backstop was built for, a buggy that veered off at close range and never came back — the
  // advertised sticky guard did nothing at all.
  // WHAT THE FIX DOES: clear only on a genuine TRANSITION into FM idle, i.e. when an FM engagement
  // or declaration actually ends. Within an engagement — RTM's included — the latch now survives
  // across ticks, which is the entire point of it being sticky. RTM's own engagement-boundary clear
  // lives at the top of runRtmLoop() (both edges of rtm_rx_active), so neither loop depends on the
  // other one running to have its fault forgiven. FM's matching ARM-edge clear is a few lines into
  // runFmLoop(), just past the idle gate — see the block there for why the start of an engagement
  // needs a clear as well as the end.
  //
  // V2.5-Evo - 2026-08-17 - the !rtm_rx_active guard that used to protect the FAULT clear here is
  // gone with the clear itself. It existed because runFmLoop()'s idle gate includes rtm_rx_active,
  // so the tick on which RTM ARMS drives FM into idle and the clear would have wiped a proven fault
  // a few microseconds after RTM had decided to carry it into the run (loop() calls runRtmLoop()
  // before runFmLoop()). With no fault clear left at this site, that whole hazard is structural
  // rather than guarded: neither loop can forgive a compass verdict at a state boundary.
  if (fm_was_not_idle) {
    // Unfinished proof only — the verdict is not a boundary's business.
    heading_disagree_since_ms     = 0;
    heading_disagree_last_seen_ms = 0;
  }
}

// ------------------------------------------------------------
// fmEffectiveEngageDistance - one source of truth for D_engage
// ------------------------------------------------------------
// Uses the configured override when non-zero, otherwise the existing automatic 1.5 x follow
// distance calculation, then applies the shared 8 m tow-rope floor. Normal radial activation,
// FM_RETURN entry/arrival and divergence all consume this exact same value.
static float fmEffectiveEngageDistance()
{
  return fmEffectiveEngageDistanceFromConfig(usrConf);
}

// Explicit values are absolute metres. Zero is what existing SW35 configs contain in the formerly
// reserved slot, so it reconstructs the previous 6 x D_engage limit. Every path is finally clamped
// to the requested [2 x D_engage, 100 m] interval. The upper bound wins if 2 x D_engage exceeds 100.
static float fmEffectiveDivergeDistance(float d_engage)
{
  float minimum = kFmDivergeMinEngageRatio * d_engage;
  if (minimum > kFmDivergeMaxDistM) minimum = kFmDivergeMaxDistM;

  float limit = usrConf.fm_diverge_dist_m;
  if (!(limit > 0.0f && limit <= kFmDivergeMaxDistM)) {
    limit = kFmDivergeLegacyFactor * d_engage;
  }
  if (limit < minimum) limit = minimum;
  if (limit > kFmDivergeMaxDistM) limit = kFmDivergeMaxDistM;
  return limit;
}

// FM_RETURN position proof deliberately excludes heading and trigger. A stationary rider can arm
// the return while the trigger is released, but stale or rejected coordinates can never start it.
static bool fmReturnPositionOk(unsigned long now)
{
  return !gps_rejected && gps_phase_b_ok &&
      rx_tx_gps_timestamp != 0 &&
      (now - rx_tx_gps_timestamp) <= (uint32_t)usrConf.tx_gps_stale_timeout_ms &&
      gps_last_ms != 0 && (now - gps_last_ms) <= 6000UL;
}

// RTM-equivalent safety gates for the return drive, without RTM's obsolete activation state or
// fixed stop radius. A heading may be absent only when the existing advanced gate permits it (or
// a proven compass disagreement has withdrawn the compass); updateRtmSteering then holds straight
// and the align cap limits throttle to ~5% until a trustworthy GPS course appears.
static bool fmReturnFaultOk(unsigned long now)
{
  if (!fmReturnPositionOk(now)) return false;
  if ((now - last_packet) > usrConf.failsafe_time) return false;

  if (usrConf.rtm_compass_required && !headingDisagreeLatched()) {
    float h_unused;
    uint8_t conf_unused;
    if (!getRtmHeading(&h_unused, &conf_unused)) return false;
  }
  return true;
}

// Direct-return throttle law: same RTM two-phase shape (align, GPS-speed governor, approach
// deceleration), but with effective D_engage as the arrival radius. The legacy RTM approach value
// is interpreted here as a BAND WIDTH outside D_engage; this remains useful after the standalone
// RTM mode is removed and avoids the unsafe 1 m decel band produced by absolute 12 m vs 11 m values.
static uint8_t fmComputeReturnThrottleCap(float dist_m, float d_engage, unsigned long now)
{
  uint16_t cap = 255;

  float approach_band_m = (float)usrConf.rtm_approach_zone_m;
  if (approach_band_m < 2.0f) approach_band_m = 2.0f;
  float approach_outer_m = d_engage + approach_band_m;
  if (dist_m < approach_outer_m) {
    float frac = (dist_m - d_engage) / approach_band_m;
    if (frac < 0.0f) frac = 0.0f;
    if (frac > 1.0f) frac = 1.0f;
    uint16_t c = (uint16_t)(frac * 255.0f);
    if (c < cap) cap = c;
  }

  float target_kmh = usrConf.rtm_target_speed_kmh;
  if (target_kmh <= 0.1f) target_kmh = kFmReturnDefaultSpeedKmh;
  if (target_kmh > kFmReturnHardMaxSpeedKmh) target_kmh = kFmReturnHardMaxSpeedKmh;
  if (usrConf.boogie_vmax_in_followme_kmh > 0.1f &&
      target_kmh > usrConf.boogie_vmax_in_followme_kmh) {
    target_kmh = usrConf.boogie_vmax_in_followme_kmh;
  }
  if (target_kmh <= 0.1f) {
    cap = 0;
  } else {
    float speed_frac = gps_last_speed_kmh / target_kmh;
    if (speed_frac < 0.0f) speed_frac = 0.0f;
    if (speed_frac > 1.0f) speed_frac = 1.0f;
    uint16_t c = (uint16_t)((1.0f - speed_frac) * 255.0f);
    if (c < cap) cap = c;
  }

  float abs_err = (g_heading_error_dx10 != 0x7FFF)
      ? fabsf((float)g_heading_error_dx10 / 10.0f)
      : 180.0f;
  if (abs_err > (float)usrConf.rtm_align_threshold_deg && kFmAlignCap < cap) {
    cap = kFmAlignCap;
  }

  if (fm_return_start_ms != 0) {
    unsigned long elapsed = now - fm_return_start_ms;
    if (elapsed < kFmEngageRampMs) {
      uint16_t c = (uint16_t)(((float)elapsed / (float)kFmEngageRampMs) * 255.0f);
      if (c < cap) cap = c;
    }
  }
  return (uint8_t)cap;
}

static void fmReturnFaultStop(unsigned long now, const char *reason, bool thr_held)
{
  fm_rx_active       = false;
  rtm_steer_override = 127;
  fm_throttle_cap    = 0;
  fm_geometry_warning = false;
  fm_front_warning    = false;
  fm_min_dist_stop_latched = false;
  // A RETURN fault must always reach the TX, even if it happens during a trigger-release pause.
  // Otherwise the TX keeps sending its old 0xF2 declaration and can silently re-arm the RX after
  // STOPPING. The fault flag is therefore also the declaration-ownership acknowledgement here.
  fm_fault_alarm_ms = now;
  fm_stop_ms = now;
  fm_state   = FM_STOPPING;
  Serial.printf("FM [RX] RETURN FAULT -> STOPPING: %s (thr_held=%d)\n", reason, (int)thr_held);
}

// Normal RETURN exit. Arrival and moving-rider cancellation share one deterministic destination:
// FM_ARMED with the live F1-F4 declaration preserved and the separation proof cleared. Motor posture
// is published before any diagnostic output. If the trigger is held, cap 0 remains interlocked until
// one release so exposing ARMED/manual cap 255 cannot produce a surge at the transition edge.
static void fmExitReturnToArmed()
{
  fm_rx_active       = false;
  rtm_steer_override = 127;
  fm_throttle_cap    = 0;

  fm_state                 = FM_ARMED;
  fm_sep_latched           = false;
  fm_sep_over_since_ms     = 0;
  fm_min_dist_stop_latched = false;
  fm_return_candidate_since_ms = 0;
  fm_return_start_ms            = 0;
  fm_return_resume_since_ms     = 0;
  fm_return_check_ms            = 0;
  fm_return_prev_dist_m         = -1.0f;
  fm_geometry_warning           = false;
  fm_front_warning              = false;
  fm_diagonal_engaged           = false;
  fm_engage_ms                  = 0;
  fm_diverge_since_ms           = 0;
  fm_diverge_start_dist_m       = -1.0f;

  // Direct-return and normal Follow-Me use different targets. Never let either controller lifecycle
  // differentiate its next heading error against the final sample from the other one.
  prev_heading_error_deg  = 0.0f;
  prev_heading_src_valid  = false;
  prev_steering_update_ms = 0;
  g_heading_error_dx10    = 0x7FFF;
  g_d_error_dx10          = 0x7FFF;

  fm_return_exit_hold = (thr_received >= 25);
  if (!fm_return_exit_hold) fm_throttle_cap = 255;
}

// ------------------------------------------------------------
// runFmLoop - the Follow-Me state machine. Call from loop().
// ------------------------------------------------------------
// What it does (DESIGN_FOLLOW_ME.md sections 4-7):
//   Runs at 10 Hz, after runFmNavigationLoop(). Every tick it re-evaluates the
//   activation conditions and moves FM between IDLE / ARMED / ACTIVE / RETURN. While
//   ACTIVE it computes the selected trailing/front target, hands it to the shared controller, and
//   recomputes the throttle cap chain.
//
// Inputs:  fm_mode_runtime (0xFF = no TX declaration this session = FM_IDLE), fm_mode_last_rx_ms,
//          all GPS/link globals, the eight FM SPIFFS parameters.
// Outputs: fm_rx_active, fm_throttle_cap, rtm_steer_override (via updateFmSteering),
//          fm_target_lat/lng.
// Side effects: MOTOR-RELEVANT. fm_throttle_cap can reduce throttle and rtm_steer_override can
//   redirect steering, but only ever through calcPWM()'s existing subtract-only chain and only
//   while the rider is holding the trigger.
// ------------------------------------------------------------
void runFmLoop()
{
  // Rate-limit to 10 Hz; the geometry maths costs ~1 ms per call.
  static unsigned long last_fm_ms = 0;
  unsigned long now = millis();
  if (now - last_fm_ms < 100UL) return;
  last_fm_ms = now;

  // Start a fresh diagnostic decision for this exact control tick. The guard publishes whatever
  // state every branch leaves behind, including early safety returns.
  fm_diag_gate_flags       = 0;
  fm_diag_block_reason     = FM_LOG_BLOCK_UNKNOWN;
  fm_diag_distance_m       = -1.0f;
  fm_diag_d_engage_m       = fmEffectiveEngageDistance();
  fm_diag_front_angle_deg  = -1.0f;
  fm_diag_return_candidate = false;
  fm_diag_can_be_active    = false;
  fm_diag_manual_steer     = false;
  fm_diag_diverge_fault    = false;

  uint8_t diag_mode = fm_mode_runtime.load(std::memory_order_relaxed);
  if (usrConf.gps_en && usrConf.rtm_rx_enabled && diag_mode >= 1 && diag_mode <= 4) {
    fm_diag_gate_flags |= FM_LOG_GATE_MODE_OK;
  }
  if (thr_received >= 25) fm_diag_gate_flags |= FM_LOG_GATE_TRIGGER_HELD;
  FmLogDiagPublishGuard fm_diag_publish_guard { now };

  // A normal RETURN exit already put the logical state in FM_ARMED. Keep only the motor interlock
  // alive until the rider releases a trigger that was still held at that edge. The declaration stays
  // live, but the separation proof remains cleared and cannot rebuild behind this early return.
  if (fm_return_exit_hold) {
    fm_diag_block_reason = FM_LOG_BLOCK_RETURN_EXIT_HOLD;
    fm_rx_active       = false;
    rtm_steer_override = 127;
    fm_throttle_cap    = 0;
    if (thr_received >= 25) return;
    fm_return_exit_hold = false;
    fm_throttle_cap = 255;
    Serial.println("FM [RX] RETURN exit throttle interlock released -> FM_ARMED/manual; fresh >D_engage proof required");
  }

  // Keep the rider filter and derived motion warm on every tick, in every state, so the
  // instant the conditions are met we already have a trustworthy course and speed.
  updateFmRiderTracking();

  // ---- Resolve the active mode ----
  // V2.5-Evo - 2026-07-20 - R0: the "0xFF falls back to usrConf.followme_mode" line is GONE.
  // WHAT THE BUG WAS: 0xFF means "the TX has not declared an FM mode this session". Falling
  // back to the SPIFFS value meant a factory RX (defaultConf.followme_mode = 2) booted
  // LATENTLY ARMED — no gesture, no declaration, and nothing on the display to say so. A rider
  // holding the trigger beyond the engage distance would have handed steering to FM without
  // ever asking for it. WHAT THE FIX DOES: 0xFF now means FM_IDLE, always, and 0xFF is greater
  // than 4 so the mode gate below catches it. usrConf.followme_mode keeps exactly one job —
  // it is the value the TX's arm gesture SEEDS from (TX RTMState.ino). It is never again an
  // RX-side auto-arm source. Autonomous steering now always requires a live human declaration.
  uint8_t m = fm_mode_runtime.load(std::memory_order_relaxed);

  // ---- R2(b): expire a declaration nobody is refreshing ----
  // The TX re-sends 0xF2/mode every 30 s while armed. If no refresh has arrived for
  // kFmModeAgeMs (95 s, ~3 missed keepalives), the declaration is stale — most likely the
  // TX disarmed and its 0xF2/0 burst was lost in the air, or the TX is gone. Drop to FM_IDLE
  // and reset the runtime mode to 0xFF so re-arming requires a fresh declaration.
  if (m >= 1 && m <= 4) {
    unsigned long mode_ms = fm_mode_last_rx_ms.load(std::memory_order_relaxed);
    if (mode_ms == 0 || (now - mode_ms) > kFmModeAgeMs) {
      fm_diag_block_reason = FM_LOG_BLOCK_MODE_EXPIRED;
      Serial.println("FM [RX] mode declaration expired (no 0xF2 refresh) -> IDLE");
      fm_mode_runtime.store(0xFF, std::memory_order_relaxed);
      fmEnterIdle();
      return;
    }
  }

  // ---- FM_IDLE: FM off / never declared (0xFF), or GPS/FM disabled ----
  if (!usrConf.gps_en || !usrConf.rtm_rx_enabled || m < 1 || m > 4) {
    fm_diag_block_reason = (!usrConf.gps_en || !usrConf.rtm_rx_enabled)
        ? FM_LOG_BLOCK_CONFIG_DISABLED : FM_LOG_BLOCK_NO_DECLARATION;
    fmEnterIdle();
    return;
  }

  // ============================================================
  // V2.5-Evo - 2026-08-16 - FM ARM BOUNDARY: forgive a heading disagreement proven while nothing
  // was armed. This is FM's half of the arm-edge clear; RTM's half is the rtm_rx_active
  // false -> true edge at the top of runRtmLoop(), which carries the full rationale.
  //
  // WHY IT IS HERE, ONE LINE BELOW THE IDLE GATE. Reaching this point means FM has a live TX
  // declaration, RTM does not own the buggy, and GPS/RTM are enabled — i.e. this tick is part of an
  // arm cycle. fm_state is still whatever the PREVIOUS tick left it as, and every tick that gets
  // past this gate ends by writing FM_ARMED, FM_ACTIVE or FM_STOPPING before it returns.
  // So "fm_state is still FM_IDLE here" is true on exactly one tick per arm cycle: the first one.
  // That makes this a genuine EDGE with no extra bookkeeping variable, and it covers both ways out
  // of idle (IDLE -> ARMED, and the rarer IDLE -> ACTIVE when the latch is already proven).
  //
  // AND IT IS DELIBERATELY BELOW THE GATE, NOT ABOVE IT. Above the gate this line would run on
  // every tick RTM owns the buggy — FM is held in FM_IDLE for the whole of an RTM engagement — and
  // that is exactly the wipe-every-tick bug this whole change set exists to undo.
  //
  // The clear runs BEFORE checkFmFaultConditions() below calls getRtmHeading(), so a disagreement
  // that is still genuinely present is re-measured and re-opens its dwell on this very tick; it
  // simply has to prove itself again over kHeadingDisagreeMs, from the start of the run.
  //
  // V2.5-Evo - 2026-08-17 - THIS EDGE NO LONGER CLEARS THE FAULT, ONLY THE UNFINISHED DWELL —
  // matching RTM's arm edge, and for the identical reason. The paragraph above says the fault
  // "simply has to prove itself again over kHeadingDisagreeMs, from the start of the run". IT
  // CANNOT: a comparison needs a compass snapshot younger than kHeadingCompareSnapMs, and
  // Compass.ino only refreshes that snapshot while thr_received < 25. FM engages under a HELD
  // trigger, so about one second into the run the comparison goes dormant and the 5 s dwell can
  // never complete. The evidence exists only BEFORE the run — while the rider coasts with the
  // trigger released above rtm_cog_min_speed_kmh — so clearing the verdict here deleted the one
  // thing the guard had to work with. The unfinished dwell is still dropped, because a half-built
  // proof must not span two different situations.
  //
  // V2.5-Evo - 2026-08-27 - A surviving fault now withdraws only the compass. It is not an FM
  // eligibility term: valid live COG or the short held-COG bridge may still engage FM. If neither
  // GPS-derived heading is available, condition 6 still keeps FM inactive.
  // ============================================================
  if (fm_state == FM_IDLE) {
    heading_disagree_since_ms     = 0;
    heading_disagree_last_seen_ms = 0;
  }

  // ---- FM_STOPPING: a FAULT ended FM; ramp throttle back to manual, then go IDLE (A3) ----
  // V2.5-Evo - 2026-07-20 - A3 FAULT semantics. Once a fault has stopped FM this run, autonomy is
  // over until a fresh declaration. We do NOT re-check the conditions here: even if the fault
  // clears mid-ramp, FM stays down and requires re-arm (silent resume after an anomaly is exactly
  // the unrequested autonomy this architecture forbids). We only ramp the throttle cap back up so
  // the rider regains manual control smoothly, then drop to FM_IDLE.
  // MOTOR SAFETY: the cap only ever RISES toward 255 (subtract-only, never adds throttle); the
  // rider's held trigger stays the sole throttle source, and starting the ramp from 0 means no
  // lurch. (RTM preemption / GPS-off / mode-off above still abort straight to IDLE.)
  if (fm_state == FM_STOPPING) {
    fm_diag_block_reason = FM_LOG_BLOCK_STOPPING;
    fm_rx_active       = false;
    rtm_steer_override = 127;
    fm_geometry_warning = false;
    fm_front_warning    = false;
    unsigned long stop_elapsed = now - fm_stop_ms;
    if (stop_elapsed >= kFmStopRampMs) {
      // Ramp done: require a fresh TX declaration to re-arm (mirror the mode-age expiry path so the
      // TX must re-send 0xF2/mode; the TX also learns of the fault via fm_flags and clears its own
      // fm_armed - see runRtmLoop's fm_flags bit 3).
      fm_mode_runtime.store(0xFF, std::memory_order_relaxed);
      fmEnterIdle();
      return;
    }
    fm_throttle_cap = (uint8_t)(((float)stop_elapsed / (float)kFmStopRampMs) * 255.0f);
    return;
  }

  // ---- Evaluate the conditions ----
  // DEADMAN = trigger. FAULT = sensor/link/heading gates. Geometry is diagnostic only;
  // fm_rx_active records whether automatic control was live last tick.
  bool  thr_held = (thr_received >= 25);       // condition 1 (DEADMAN — never a fault)
  int   manual_steer_dev = (int)steering_received - 127;
  if (manual_steer_dev < 0) manual_steer_dev = -manual_steer_dev;
  bool  manual_steer_requested =
      (manual_steer_dev >= (int)kFmManualSteerDeadband);
  fm_diag_manual_steer = manual_steer_requested;
  bool  fault_ok = checkFmFaultConditions();   // conditions 2-7 (FAULT)
  bool  hard_ok  = thr_held && fault_ok;       // gates automatic control and separation proof
  bool  was_controlling = fm_rx_active.load(std::memory_order_relaxed);
  bool  active_session  = (fm_state == FM_ACTIVE);
  float dist_m   = 0.0f;
  float d_engage = fmEffectiveEngageDistance();
  fm_diag_d_engage_m = d_engage;
  float diverge_limit_m = fmEffectiveDivergeDistance(d_engage);
  bool  front_mode           = (m == 4);
  bool  front_geometry_valid = false;
  float front_along_m        = 0.0f;
  float front_cross_m        = 0.0f;
  float front_off_axis_deg   = 180.0f;
  // V2.5-Evo - 2026-07-25 - A3: sustained divergence while ACTIVE. Classed as a FAULT (same family
  // as conditions 2-7), so it is routed through the SAME FM_STOPPING path below — never its own.
  bool  diverge_fault = false;
  // V2.5-Evo - 2026-07-25 - F7: the numbers the divergence message prints, captured at detection but
  // PRINTED LATER — in the fault branch, after fm_throttle_cap = 0. WHAT THE BUG WAS: the message was
  // printed at the moment of detection, which is upstream of the cap write, so a full UART TX buffer
  // could block inside Serial.printf() and delay the hard stop by however long the host took to drain
  // it. The motor must reach 0 first and the explanation can wait; nothing else reads these.
  float diverge_start_m = 0.0f;   // the distance captured when the dwell started, m

  // Position trust is narrower than fault_ok: radial thresholds need both plausible/fresh positions,
  // but no heading. If the pair is stale or rejected no distance proof is accumulated.
  bool latch_position_ok = fmReturnPositionOk(now);
  if (latch_position_ok) fm_diag_gate_flags |= FM_LOG_GATE_POSITION_OK;

  if (latch_position_ok) {
    dist_m = (float)TinyGPSPlus::distanceBetween(
        gps_last_lat, gps_last_lng, rx_tx_gps_lat, rx_tx_gps_lng);
    fm_diag_distance_m = dist_m;
    if (isfinite(dist_m) && dist_m > d_engage) {
      fm_diag_gate_flags |= FM_LOG_GATE_DIST_OVER_ENGAGE;
    }
  }

  // ---- FM_RETURN entry proof ----
  // A live FM declaration is enough; prior FM_ACTIVE is deliberately not required. Thus a rider
  // may arm FM while stationary and retrieve a buggy already beyond D_engage. The proof is radial,
  // trigger-independent and continuous for 2 s. During that proof the cap is held at zero, so the
  // buggy stands still before direct return even when this started from a stationary arm.
  bool return_entry_state = (fm_state == FM_ARMED || fm_state == FM_ACTIVE);
  bool return_candidate = return_entry_state && latch_position_ok &&
      (dist_m > d_engage) && (fm_rider_speed_kmh < kFmLatchResetSpeedKmh);
  fm_diag_return_candidate = return_candidate;
  if (return_candidate) {
    if (fm_return_candidate_since_ms == 0) {
      fm_return_candidate_since_ms = now;
    } else if ((now - fm_return_candidate_since_ms) >= kFmLatchResetDwellMs) {
      bool return_after_following   = (fm_state == FM_ACTIVE);
      fm_return_start_ms            = now;
      fm_return_resume_since_ms     = 0;
      fm_return_check_ms            = 0;
      fm_return_prev_dist_m         = -1.0f;
      fm_return_candidate_since_ms  = 0;
      // RETURN is a new autonomy phase, not a continuation of the old following proof. Clear the
      // separation latch at the state edge; every normal exit will therefore be ARMED/not-ready until
      // a fresh radial >D_engage dwell is proven.
      fm_sep_latched                = false;
      fm_sep_over_since_ms          = 0;
      fm_min_dist_stop_latched      = false;
      fm_diverge_since_ms           = 0;
      fm_diverge_start_dist_m       = -1.0f;
      fm_geometry_warning           = false;
      fm_front_warning              = false;
      fm_state                      = FM_RETURN;
      fm_rx_active                  = false;
      fm_throttle_cap               = 0;
      rtm_steer_override            = 127;
      // Direct-to-rider is a different control target. Never differentiate its first heading
      // error against the last trailing/front-target sample from normal Follow-Me.
      prev_heading_error_deg        = 0.0f;
      prev_heading_src_valid        = false;
      prev_steering_update_ms       = 0;
      g_heading_error_dx10          = 0x7FFF;
      g_d_error_dx10                = 0x7FFF;
      Serial.printf("FM [RX] ENTER RETURN: stationary %.1f km/h, dist=%.1f m > D_engage=%.1f m for %lu ms%s\n",
                    fm_rider_speed_kmh, dist_m, d_engage,
                    (unsigned long)kFmLatchResetDwellMs,
                    return_after_following ? " after following" : " from stationary arm");
    }
  } else if (fm_state != FM_RETURN) {
    fm_return_candidate_since_ms = 0;
  }

  // ---- FM_RETURN direct-to-rider control ----
  if (fm_state == FM_RETURN) {
    fm_diag_block_reason        = FM_LOG_BLOCK_RETURN_ACTIVE;
    fm_diag_return_candidate    = false;
    fm_sep_latched             = false;
    fm_sep_over_since_ms       = 0;
    fm_min_dist_stop_latched   = false;
    fm_return_candidate_since_ms = 0;

    // Arrival is immediate; the approach cap has already been tending to zero as the distance closed.
    // Preserve the declaration but return to ARMED with no separation proof and no ACTIVE shortcut.
    if (latch_position_ok && dist_m < d_engage) {
      fmExitReturnToArmed();
      fm_diag_block_reason = fm_return_exit_hold
          ? FM_LOG_BLOCK_RETURN_EXIT_HOLD : FM_LOG_BLOCK_BELOW_D_ENGAGE;
      Serial.printf("FM [RX] RETURN ARRIVED: dist=%.1f m < D_engage=%.1f m -> FM_ARMED; separation latch cleared%s\n",
                    dist_m, d_engage,
                    fm_return_exit_hold ? "; throttle inhibited until release" : "; manual cap 255");
      return;
    }

    // If the rider starts moving again, stop the direct chase. It always returns to ARMED: RETURN
    // cleared the old separation proof, so jumping directly back to the old ACTIVE lifecycle would
    // misrepresent that proof and create a needless state-history special case.
    if (latch_position_ok && fm_rider_speed_kmh > kFmReturnResumeSpeedKmh) {
      if (fm_return_resume_since_ms == 0) fm_return_resume_since_ms = now;
      else if ((now - fm_return_resume_since_ms) >= kFmReturnResumeDwellMs) {
        float resume_speed_kmh = fm_rider_speed_kmh;
        fmExitReturnToArmed();
        fm_diag_block_reason = fm_return_exit_hold
            ? FM_LOG_BLOCK_RETURN_EXIT_HOLD : FM_LOG_BLOCK_SEPARATION_DWELL;
        Serial.printf("FM [RX] RETURN cancelled: rider moving %.1f km/h -> FM_ARMED; separation latch cleared%s\n",
                      resume_speed_kmh,
                      fm_return_exit_hold ? "; throttle inhibited until release" : "; manual cap 255");
        return;
      }
    } else {
      fm_return_resume_since_ms = 0;
    }

    if (!fmReturnFaultOk(now)) {
      fmReturnFaultStop(now, "GPS/link/heading gate failed", thr_held);
      return;
    }
    if ((now - fm_return_start_ms) >= kFmReturnMaxRuntimeMs) {
      fmReturnFaultStop(now, "60 s runtime limit", thr_held);
      return;
    }

    // Trigger remains the sole source of motor power. Releasing it already commands zero at the
    // physical input, so do not manufacture a redundant cap transition. It pauses RETURN in-place,
    // resets convergence evidence, and squeezing again restarts the gentle ramp.
    if (!thr_held) {
      fm_rx_active          = false;
      rtm_steer_override    = 127;
      fm_return_check_ms    = 0;
      fm_return_prev_dist_m = -1.0f;
      fm_return_start_ms    = now;
      return;
    }

    fm_rx_active = true;
    fm_target_lat = fm_filt_init ? fm_filt_lat : rx_tx_gps_lat;
    fm_target_lng = fm_filt_init ? fm_filt_lng : rx_tx_gps_lng;
    updateFmSteering();
    fm_throttle_cap = fmComputeReturnThrottleCap(dist_m, d_engage, now);

    // RTM-style Phase-C convergence, with independent bookkeeping so no proof can leak between
    // following and return. Manual steering is rider-commanded, therefore not judged as autonomy.
    bool in_return_grace = (now - fm_return_start_ms) < (kFmEngageRampMs + kFmReturnCheckMs);
    if (in_return_grace || manual_steer_requested) {
      fm_return_check_ms    = 0;
      fm_return_prev_dist_m = -1.0f;
    } else if (fm_return_check_ms == 0) {
      fm_return_check_ms    = now;
      fm_return_prev_dist_m = dist_m;
    } else if ((now - fm_return_check_ms) >= kFmReturnCheckMs) {
      if (dist_m >= (fm_return_prev_dist_m - kFmReturnCloseEpsM)) {
        fmReturnFaultStop(now, "distance did not close over 5 s", thr_held);
        return;
      }
      fm_return_check_ms    = now;
      fm_return_prev_dist_m = dist_m;
    }
    return;
  }

  // The retired configurable low-speed gate is gone. Only a stationary rider outside D_engage can
  // build the FM_RETURN proof. Ordinary low speed inside D_engage does not pause or complete FM.
  bool return_proof_wait = return_candidate;  // RETURN branch above already consumed maturity

  // Geometry is still evaluated with the trigger released so the TX can repeat its 3 s warning.
  // These Schmitt decisions are diagnostics only: they never change cap, steering authority, state
  // or the separation latch. F1-F3 retain min/min+band as warning thresholds; F4 retains the old
  // signed-front/cone thresholds as warning thresholds.
  bool geometry_eval_ok = fault_ok && latch_position_ok;
  float min_dist = usrConf.min_dist_m;
  float band = usrConf.followme_smoothing_band_m;
  float d_follow_e = min_dist + band;
  if (d_follow_e < 0.5f) d_follow_e = 0.5f;

  if (geometry_eval_ok && front_mode) {
    front_geometry_valid = fmFrontGeometry(
        &front_along_m, &front_cross_m, &front_off_axis_deg);
    if (front_geometry_valid && isfinite(front_off_axis_deg)) {
      fm_diag_front_angle_deg = front_off_axis_deg;
    }
  }

  if (!active_session) {
    fm_geometry_warning = false;
    fm_front_warning    = false;
  } else if (geometry_eval_ok) {
    if (front_mode) {
      bool front_invalid = !front_geometry_valid ||
                           (dist_m <= min_dist) ||
                           (front_along_m <= min_dist) ||
                           (front_off_axis_deg >= usrConf.zone_angle_exit_deg);
      bool front_recovered = front_geometry_valid &&
                             (dist_m > d_follow_e) &&
                             (front_along_m > d_follow_e) &&
                             (front_off_axis_deg < usrConf.zone_angle_enter_deg);
      if (front_invalid) fm_front_warning = true;
      else if (front_recovered) fm_front_warning = false;
      fm_geometry_warning = !fm_sep_latched;
    } else {
      fm_front_warning = false;
      if (!fm_sep_latched || dist_m <= min_dist) fm_geometry_warning = true;
      else if (dist_m > d_follow_e) fm_geometry_warning = false;
    }
  }

  // ---- Separation proof: one activation geometry for every F1-F4 mode ----
  // A held trigger plus trustworthy/fault-free positions must show radial dist > effective D_engage
  // continuously for kFmSepDwellMs. Side angles and F4's signed front position are intentionally not
  // members of this decision. Trigger release interrupts an unfinished dwell but preserves a proof
  // that was already set (except for the min-distance stop release below).
  if (hard_ok && latch_position_ok && !fm_min_dist_stop_latched && dist_m > d_engage) {
    if (fm_sep_over_since_ms == 0) {
      fm_sep_over_since_ms = now;
    } else if (!fm_sep_latched && (now - fm_sep_over_since_ms) >= kFmSepDwellMs) {
      fm_sep_latched = true;
      Serial.printf("FM [RX] separation latch SET (mode F%u): radial dist=%.1f m > D_engage=%.1f m sustained %lu ms\n",
                    (unsigned)m, dist_m, d_engage, (unsigned long)kFmSepDwellMs);
    }
  } else if (!fm_sep_latched) {
    fm_sep_over_since_ms = 0;
  }

  // ---- min_dist_m stop latch ----
  // The first trustworthy <=min sample during an engaged/proven session removes automatic steering
  // and publishes cap 0 before diagnostics. Distance recovery cannot release it while the same pull
  // continues. Releasing the trigger clears the stop AND the old separation proof, publishes manual
  // cap 255, and therefore makes the next automatic resume prove >D_engage again.
  if (!fm_min_dist_stop_latched && thr_held && latch_position_ok &&
      (was_controlling || fm_sep_latched) && dist_m <= min_dist) {
    fm_min_dist_stop_latched = true;
    fm_rx_active             = false;
    rtm_steer_override       = 127;
    fm_throttle_cap          = 0;
    Serial.printf("FM [RX] MIN-DIST STOP LATCHED: dist=%.1f m <= min_dist_m=%.1f m; cap 0 until trigger release\n",
                  dist_m, min_dist);
  }

  if (fm_min_dist_stop_latched && !thr_held) {
    fm_min_dist_stop_latched = false;
    fm_sep_latched           = false;
    fm_sep_over_since_ms     = 0;
    fm_rx_active             = false;
    rtm_steer_override       = 127;
    fm_throttle_cap          = 255;
    if (active_session) fm_geometry_warning = true;
    Serial.printf("FM [RX] MIN-DIST STOP RELEASED: manual cap 255; fresh radial >D_engage=%.1f m proof required\n",
                  d_engage);
  }

  if (geometry_eval_ok) {
    // ---- A3: DIVERGENCE FAULT — the upper bound FM never had ----
    // V2.5-Evo - 2026-07-25. Condition 8 above is a lower bound only, so a buggy steering the WRONG
    // WAY satisfies it more and more comfortably the further it runs. This adds the missing ceiling:
    // while FM is ACTIVE, being further than the effective absolute limit from the rider AND FAILING
    // TO CLOSE for kFmDivergeMs is not "following badly", it is "not following", and it is a FAULT.
    //
    // V2.5-Evo - 2026-07-25 - F1: this used to be a BARE THRESHOLD (beyond the ceiling for the dwell
    // = fault) and that aborted legitimate engagements. Two reasons, and they compound. First, the
    // engage ramp is kFmEngageRampMs = 3500 ms but the dwell is only kFmDivergeMs = 3000 ms, so the
    // fault could fire BEFORE the buggy had finished being given throttle. Second, while the heading
    // error is still large, cap 4 pins the throttle at kFmAlignCap = 13/255 (~5%) so the buggy pivots
    // in place and the gap GROWS before it starts to shrink — and at the engagement instant dist_m is
    // typically 13-21 m against an 18 m ceiling (2 x a 9 m D_engage). Net effect: FM aborted ~3 s into
    // most real engagements and forced a re-arm mid-session.
    //
    // THE FIX IS IN TWO PARTS, and both are needed:
    //
    //   1. TEST THE DERIVATIVE, NOT THE LEVEL. This is what runPhaseC() actually does — its
    //      convergence check fails on "dist_m >= rtm_prev_dist_m", i.e. on NOT CLOSING, never on being
    //      far. We do the same in FM's own terms: snapshot the distance when the dwell starts
    //      (fm_diverge_start_dist_m) and, at dwell expiry, fault only if the buggy has failed to close
    //      by more than kFmDivergeCloseEpsM. If it HAS closed by more than that, it is following — just
    //      from further back than we would like — so the timer is cleared and no fault fires. The next
    //      tick starts a fresh window, so a buggy that is beyond the ceiling and genuinely closing is
    //      re-tested every 3 s and keeps passing for exactly as long as it keeps closing.
    //      (Why "must be DECREASING" from runPhaseC is not copied verbatim: RTM closes on a stationary
    //      rider so any non-decrease is wrong, whereas FM holds station behind a MOVING rider and the
    //      distance legitimately rises and falls every wave. The epsilon is what carries that across.)
    //
    //   2. NON-AUTONOMOUS WINDOWS. The detector is skipped entirely, and its dwell parked, for
    //      kFmEngageRampMs + kFmDivergeMs (3500 + 3000 = 6500 ms) after every entry into FM_ACTIVE.
    //      The buggy must be allowed to finish ramping AND aligning before its geometry is judged;
    //      judging it mid-ramp measures the ramp, not the steering. Parking the dwell (rather than
    //      letting it run) guarantees the first post-grace window is a full, clean kFmDivergeMs.
    //      The same parking applies while the rider deliberately holds manual steering: distance
    //      then measures the rider's command, not FM convergence. A fresh divergence window starts
    //      only after the steering input is centred and FM is actually steering again.
    //
    // Bookkeeping is otherwise the same shape runPhaseC() uses: one timer plus one baseline, evaluated
    // every tick, both cleared the instant the condition stops holding, so nothing can accumulate
    // across a gap. The dwell is a plain first-exceed timestamp compared against millis() — no delay(),
    // no blocking, no extra loop.
    //
    // Only evaluated while automatic control was live and remains geometrically eligible on this
    // tick. A lifecycle FM_ACTIVE can be paused, in which case distance says nothing about FM's
    // autonomous convergence.
    //
    // SAFETY: this branch only ever sets a flag that REMOVES eligibility. It cannot raise the
    // throttle cap, cannot extend engagement, and does not touch the deadman. Both parts of the fix
    // make the detector STRICTLY LESS likely to fire, never more — a missed divergence still leaves
    // every other fault condition and the deadman in place, and the rider can always let go.
    bool in_engage_grace = (fm_engage_ms != 0) &&
                           ((now - fm_engage_ms) < (kFmEngageRampMs + kFmDivergeMs));

    if (in_engage_grace || manual_steer_requested) {
      // Ramping/aligning or manual steering — FM convergence is not judgeable. Start fresh after.
      fm_diverge_since_ms     = 0;
      fm_diverge_start_dist_m = -1.0f;
    }
    else if (was_controlling && hard_ok && !return_proof_wait &&
             !fm_min_dist_stop_latched &&
             dist_m > diverge_limit_m) {
      if (fm_diverge_since_ms == 0) {
        // First tick beyond the ceiling: start the dwell and record what we are closing FROM.
        fm_diverge_since_ms     = now;
        fm_diverge_start_dist_m = dist_m;
      } else if ((now - fm_diverge_since_ms) >= kFmDivergeMs) {
        if (dist_m >= (fm_diverge_start_dist_m - kFmDivergeCloseEpsM)) {
          // Beyond the ceiling for the full dwell and NOT closing — this is divergence.
          // F7: the numbers are stashed and printed later, after fm_throttle_cap = 0.
          diverge_fault   = true;
          diverge_start_m = fm_diverge_start_dist_m;
        } else {
          // It has closed by more than the epsilon: the buggy IS following, just far. No fault —
          // clear the window so the next tick opens a fresh one from the current distance.
          fm_diverge_since_ms     = 0;
          fm_diverge_start_dist_m = -1.0f;
        }
      }
    } else {
      // Back inside the limit, or FM is not ACTIVE — the dwell restarts from scratch.
      fm_diverge_since_ms     = 0;
      fm_diverge_start_dist_m = -1.0f;
    }
  } else {
    // No trustworthy geometry this tick (GPS stale/rejected or link down). Restart every proof
    // rather than carrying a half-finished result across a data gap.
    fm_sep_over_since_ms = 0;
    // A3: same discipline for the divergence dwell — never judge divergence on data we do not trust.
    // F1: the closure baseline goes with it; a baseline must never outlive the dwell that set it.
    fm_diverge_since_ms     = 0;
    fm_diverge_start_dist_m = -1.0f;
  }

  fm_diag_diverge_fault = diverge_fault;

  // The radial separation proof gates eligibility. Side/front geometry is deliberately absent from
  // this AND chain: it is exported only as a warning. The min-distance stop latch is independent and
  // can be cleared only by releasing the trigger.
  // V2.5-Evo - 2026-07-25 - A3: !diverge_fault joins the same AND chain. It can only ever REMOVE
  // eligibility, so the worst case of a false positive is FM handing control back to the rider.
  // A heading-disagreement latch is diagnostic state, not an eligibility term. getRtmHeading()
  // has already removed the compass and returns true only for valid live/held GPS COG. Therefore
  // hard_ok continues to enforce a real heading without making compass agreement an extra gate.
  bool can_be_active = hard_ok && !return_proof_wait && fm_sep_latched &&
                       !fm_min_dist_stop_latched &&
                       !diverge_fault;
  fm_diag_can_be_active = can_be_active;

  // One stable primary reason accompanies the individual gate columns. Angle/front warnings are
  // deliberately absent: they are observability-only and never block automatic authority.
  if (can_be_active) {
    fm_diag_block_reason = FM_LOG_BLOCK_NONE;
  } else if (!thr_held) {
    // The physical deadman is the first activation gate. Other failed gates remain visible in
    // their named columns, but they must not obscure the simple reason while the trigger is open.
    fm_diag_block_reason = FM_LOG_BLOCK_TRIGGER;
  } else if (!fault_ok) {
    // checkFmFaultConditions() stored its exact first failing gate.
    if (fm_diag_block_reason == FM_LOG_BLOCK_UNKNOWN) {
      fm_diag_block_reason = FM_LOG_BLOCK_NO_HEADING;
    }
  } else if (diverge_fault) {
    fm_diag_block_reason = FM_LOG_BLOCK_DIVERGENCE;
  } else if (fm_min_dist_stop_latched) {
    fm_diag_block_reason = FM_LOG_BLOCK_MIN_DIST_STOP;
  } else if (return_proof_wait) {
    fm_diag_block_reason = FM_LOG_BLOCK_RETURN_CANDIDATE;
  } else if (!latch_position_ok) {
    fm_diag_block_reason = FM_LOG_BLOCK_POSITION;
  } else if (!fm_sep_latched && !(dist_m > d_engage)) {
    fm_diag_block_reason = FM_LOG_BLOCK_BELOW_D_ENGAGE;
  } else if (!fm_sep_latched) {
    fm_diag_block_reason = FM_LOG_BLOCK_SEPARATION_DWELL;
  } else {
    fm_diag_block_reason = FM_LOG_BLOCK_UNKNOWN;
  }

  if (can_be_active) {
    // Manual steering is arbitrated in calcPWM() at 100 Hz. FM intentionally remains ACTIVE here,
    // keeps its separation latch and throttle cap, and continues calculating the automatic command
    // in the background. Centring the input therefore hands steering back without a state edge.

    // ---- FM_ACTIVE ----
    if (!was_controlling) {
      // First engagement from ARMED or resumption inside the same FM_ACTIVE lifecycle.
      fm_engage_ms        = now;    // start the engage ramp - re-engagement is never a jump
      fm_diagonal_engaged = false;  // re-evaluate which side we are on for this engagement

      // Reset the shared P+D derivative continuity. Without this the controller would
      // differentiate a fresh heading error against a stale pre-engagement sample across the
      // gap and command a violent phantom turn on the first tick.
      prev_heading_src_valid  = false;
      prev_heading_error_deg  = 0.0f;
      prev_steering_update_ms = 0;
      fmResetSpeedGovernor();

      bool first_engagement = !active_session;
      fm_state = FM_ACTIVE;
      if (front_mode) {
        Serial.printf("FM [RX] %s F4 In Front: dist=%.1f m along=%.1f m cross=%.1f m angle=%.1f deg rider=%.1f km/h course=%.0f\n",
                      first_engagement ? "ENGAGE" : "RESUME",
                      dist_m, front_along_m, front_cross_m, front_off_axis_deg,
                      fm_rider_speed_kmh, fm_rider_course_deg);
      } else {
        Serial.printf("FM [RX] %s mode %u: dist=%.1f m rider=%.1f km/h course=%.0f\n",
                      first_engagement ? "ENGAGE" : "RESUME",
                      (unsigned)m, dist_m, fm_rider_speed_kmh, fm_rider_course_deg);
      }
    }

    fm_rx_active = true;                                   // gate the steering override on
    computeFmTarget(&fm_target_lat, &fm_target_lng);       // trailing point (F1-3) or front lookahead (F4)
    updateFmSteering();                                    // shared P+D controller
    fm_throttle_cap = (uint8_t)fmComputeThrottleCap(dist_m, front_along_m, m, now);
  }
  else {
    // ---- Not eligible to steer — classify FAULT vs transient ACTIVE inhibit vs never-engaged ARMED ----
    fm_rx_active       = false;
    rtm_steer_override = 127;   // hand steering straight back to the rider
    fm_engage_ms       = 0;     // any re-engagement ramps from zero again

    // A proven compass disagreement is absent here by design: it has already degraded the heading
    // ladder to GPS COG. Only an actual condition-2..7 failure (including no valid COG/hold) or the
    // divergence fault ends an ACTIVE session.
    if ((!fault_ok || diverge_fault) && active_session) {
      // ---- FAULT (conditions 2-7, plus A3 divergence): something actually broke while FM had control ----
      // End autonomy for the run: enter FM_STOPPING, which ramps the throttle cap 0 -> 255 over the
      // next kFmStopRampMs (handled at the top of runFmLoop), then drops to FM_IDLE — re-arm
      // required. Fire the stop notification (sticky fm_flags bit 3, drives St + stop buzz on the
      // TX) only if the trigger was held at this instant — a fault after release is not surprising,
      // and the bar going dark carries it.
      // R4: heading loss is one of these faults now.
      // V2.5-Evo - 2026-07-25 - A3: sustained divergence enters through THIS branch and no other, so
      // it inherits the proven fault semantics unchanged — hard stop to cap 0 now, the same ramp back
      // to manual, the same haptic/St notification, and the same mandatory re-arm. The dwell is
      // parked while the rider deliberately holds manual steering, so this classifier judges FM's
      // own convergence only after automatic steering has resumed.
      if (thr_held) fm_fault_alarm_ms = now;
      fm_stop_ms      = now;
      fm_state        = FM_STOPPING;
      fm_min_dist_stop_latched = false;
      fm_throttle_cap = 0;         // subtract-only hard stop; the ramp begins next tick
      // V2.5-Evo - 2026-07-25 - F7: ALL fault logging happens BELOW this line, never above it. The
      // divergence detail used to print at the point of detection, which is upstream of the cap write
      // — so if the USB CDC TX buffer was full (host not draining) Serial.printf() could block and
      // defer the hard stop for as long as the host took. Motor to 0 first, explain afterwards.
      if (diverge_fault) {
        Serial.printf("FM [RX] DIVERGENCE FAULT: dist=%.1f m (was %.1f m at dwell start, closed <=%.1f m) > absolute limit %.1f m sustained %lu ms — not closing\n",
                      (double)dist_m, (double)diverge_start_m, (double)kFmDivergeCloseEpsM,
                      (double)diverge_limit_m, (unsigned long)kFmDivergeMs);
      }
      Serial.printf("FM [RX] FAULT -> STOPPING (ramp %lu ms) -> IDLE, re-arm required (thr_held=%d)\n",
                    (unsigned long)kFmStopRampMs, (int)thr_held);
    } else if (active_session) {
      // ---- FM_ACTIVE without automatic authority — no lifecycle edge ----
      // Geometry never reaches this branch by itself. A released trigger needs no cap write because
      // its physical throttle value is already zero. The min-distance latch remains cap 0 until that
      // release; a pending RETURN proof also holds cap 0 only while power is actually requested.
      fm_state = FM_ACTIVE;

      if (fm_min_dist_stop_latched || (return_proof_wait && thr_held)) {
        fm_throttle_cap = 0;
      }
    } else {
      // ---- FM_ARMED: never engaged this arm cycle — fully manual buggy ----
      // The throttle chain stays INACTIVE (cap 255) so the rider keeps full manual control while
      // FM waits for the follow geometry. Manual steering is available in both ARMED and ACTIVE.
      fm_state        = FM_ARMED;
      fm_throttle_cap = (return_proof_wait && thr_held) ? 0 : 255;

    }
  }

}
