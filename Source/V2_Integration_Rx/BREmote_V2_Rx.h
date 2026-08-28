// V2.5-Evo - 2026-08-27 - Deep logs append a 13-byte heading-evidence audit after the existing 83-byte FM audit record: configured heading mode, compass-vs-COG difference, set/clear dwell progress, last-good-COG and compass-snapshot ages, plus twelve raw validity/evidence flags. The BRLG record_size keeps 65-byte and 83-byte Deep files readable with their matching historical CSV headers. Instrumentation only; no control/config/packet/SW_VERSION change.
// V2.5-Evo - 2026-08-27 - Throttle-dependent steering without a config reset: retired foiler_low_speed_kmh float renamed in place to steer_full_throttle_pct and rsvd_u16_1 renamed in place to steer_reduction_start_pct. Defaults 35%/50%; cfgValidateCrossField() migrates legacy values before validation. PWM applies the smoothstep curve after manual/FM arbitration using effective throttle, so manual riding and automatic FM share the same rollover protection. Types, offsets, sizeof(confStruct)==192 and SW_VERSION 35 are unchanged.
// V2.5-Evo - 2026-08-27 - fm_diverge_dist_m claims the banked rsvd_f32_1 slot in place as an absolute-metre setting. The effective limit is clamped to [2 x effective D_engage, 100 m]. Same final float, same offset and sizeof(confStruct)==192, so SW_VERSION remains 35 and stored RX configuration is not reset. Existing boards contain 0 in this formerly unused slot; 0 derives the previous 6 x D_engage limit and applies the new 100 m maximum. New/default configs store 100 m explicitly. No packet or struct-layout change.
// V2.5-Evo - 2026-08-28 - rtm_target_speed_kmh is now the literal 0-50 km/h FM_RETURN PI-governor setpoint: 0 means zero speed, with no default substitution and no 8 km/h hard cap. kFmReturnTargetSpeedMaxKmh is shared by config validation and the defensive control-path clamp. No field/layout/default/version change; SW_VERSION stays 35.
// V2.5-Evo - 2026-08-26 - Added shared compile-time kFmManualSteerDeadband=40 for FM manual-steering arbitration in both PWM.ino and RTMState.ino. No confStruct field/size/version change; SW_VERSION stays 35.
// V2.5-Evo - 2026-08-17 - COMMENT-ONLY size correction (no code, no struct, no SW_VERSION change): the mag_orientation block claimed "sizeof 184 -> 188". The finished struct is 192 — mag_orientation (2) plus the two reserved slots rsvd_u16_1 (2) and rsvd_f32_1 (4) that landed in the same SW34->35 edit, naturally aligned with no tail pad. The static_assert has always said 192; only the prose was wrong. Corrected in three places: the mag_orientation block, the "confStruct is 184 bytes" line in the log_level block (retensed as history), and the static_assert's own trailing history, which never recorded the 184->192 step and now does. Flagged as load-bearing rather than cosmetic because the SW34->35 config-backup migration is pinned to the exact counts 184 (legacy) and 192 (current) and disables itself if either stops matching. Every remaining "184" in this file sits inside a dated change-history entry and is correct AS HISTORY — the static_assert is the SSOT for the current size.
// V2.5-Evo - 2026-08-17 - defaultConf.vesc_timeout_s raised 6 -> 10 s, because a VESC cold restart takes roughly 8-9 seconds and 6 s is shorter than that: every restart blanked the rider's battery % and FET temperature to "N/A" on the TX while the VESC was merely booting. 10 s covers the restart and is still half the original hardcoded 20 s. VALUE-ONLY change to an existing field: no field added, moved, renamed or resized, and the 5-60 s validation range in ConfigService is untouched — so sizeof(confStruct) stays 192, the static_assert is unchanged, SW_VERSION stays 35 and the owner's SPIFFS config is NOT wiped by this flash. Because it is NOT wiped, a board with a stored vesc_timeout_s keeps its old value: it must be set on the device with `?set vesc_timeout_s 10` + `?save`. Second consumer checked — RTM Phase C check 2 uses the same field to decide when to SKIP its VESC-ERPM-vs-GPS-speed comparison; a longer timeout means fewer skips, and since a skipped check and a passed check have the identical outcome (no stop) the check's coverage can only widen, never shrink.
// V2.5-Evo - 2026-07-25 - STAGE 2 (heading-source trust guards): added the four shared compile-time constants the RTM/FM heading ladder needs — kRtmCogFrozenMs (3000), kHeadingDisagreeDeg (45.0), kHeadingDisagreeMs (5000), kHeadingCompareSnapMs (1000). They live HERE, once, for the same reason kFmEngageDistFloorM does: getRtmHeading() in RTMState.ino and its inline duplicate in Logger.ino both read them, and Logger.ino is concatenated BEFORE RTMState.ino, so a constant defined in RTMState.ino would be invisible to the logger mirror. Constants + comments only: no confStruct field added, moved, renamed or resized; sizeof(confStruct) stays 184, static_assert unchanged, SW_VERSION stays 34, SPIFFS config is NOT reset by this flash.
// V2.5-Evo - 2026-07-25 - STAGE 1 (GPS repair): defaultConf.gps_update_hz raised 2 -> 10 Hz, because the GPS module is configured for 5 Hz (BN-220/BN-880) or 10 Hz (M10) and, now that STAGE 1 leaves the UART mux parked on GPS, a 500 ms drain interval would leave ~1250 bytes pending per drain. VALUE-ONLY change to an existing field: no field added, moved, renamed or resized, and the 1-10 validation range in ConfigService is untouched — so sizeof(confStruct) stays 184, the static_assert is unchanged, SW_VERSION stays 34 and the owner's SPIFFS config is NOT wiped by this flash. Because it is NOT wiped, a board with a stored gps_update_hz keeps its old value: it must be set on the device with `?set gps_update_hz 10` + `?save`. No control-path field (throttle, steering, PWM, RTM/FM) is touched.
// V2.5-Evo - 2026-07-25 - STAGE 0 (instrumentation only, ZERO control-behaviour change): (A) the unused RESERVED slot fm_steer_reposition_en is RENAMED IN PLACE to log_level — same offset, same uint16_t, so sizeof(confStruct) stays 184, the static_assert is unchanged, SW_VERSION stays 34 and the owner's SPIFFS config is NOT wiped (same trick as dummy_delete_me -> rtm_steer_response, Bundle 1). 0 = unset (behaves exactly as level 3), 1 = Basic, 2 = VESC, 3 = Developer, 4 = Deep; only 3 and 4 are implemented, 1 and 2 are accepted by the validator and currently log as level 3. (B) added LogFileHeader — every log file now starts with an 8-byte self-describing header (magic/format/level/record size) so a reader can parse a variable record size. (C) added VescLogDataL4 = VescLogData + the 4 level-4 diagnostic fields, and the free-running diagnostic counters the ?diag command and the level-4 record read. Nothing here is read by throttle, steering, PWM, the mux schedule, or any FM/RTM logic.
// V2.5-Evo - 2026-07-25 - F3-b: kFmEngageDistFloorM (the FM engage-distance floor) now lives HERE, once, and is raised 5.0 -> 8.0 m. It used to be defined in RTMState.ino AND duplicated as a bare 5.0f literal in ConfigService.ino, on the false premise that the Arduino concatenation order stopped the two files sharing a constant — this header is included at the top of V2_Integration_Rx.ino, which is compiled first, so both see it. 5.0 m was below the hazard it names: the owner's tow rope is 20 ft = 6.10 m, so a manual fm_engage_dist_m of 5.0-6.1 m was storable and let FM engage with the rider still ON the rope. 8.0 m clears a 6.10 m rope by ~1.31x. One shared constant + comments only: no field added, moved or resized; sizeof(confStruct) stays 184, static_assert unchanged, SW_VERSION stays 34, SPIFFS config is NOT reset by this flash.
// V2.5-Evo - 2026-07-25 - A2: fm_engage_dist_m promoted from RESERVED/unread to LIVE — runFmLoop() now reads it as the FM engage distance in METRES (rope length x ~1.15), 0 = auto (unchanged legacy behaviour). Comment/semantics only: no field added, moved or resized; sizeof(confStruct) stays 184, static_assert unchanged, SW_VERSION stays 34, SPIFFS config is NOT reset by this flash.
// V2.5-Evo - 2026-07-24 - F9: VescLogData +6 bytes (tx_distance_dx10, rssi_dbm, snr_dx10) for owner-requested distance + link-quality CSV columns; sizeof 53->59; old SPIFFS logs misparse after this flash; NO confStruct change, SW_VERSION stays 34
// V2.5-Evo - 2026-07-20 - SW33->34 config bump + defaultConf bake: appended THREE reserved confStruct slots — fm_engage_dist_m (float, 0=auto), auton_runtime_cap_s (uint16_t, 0=disabled), fm_steer_reposition_en (uint16_t, 0=off). All three are default-off storage slots and are NOT read by v1 control law — bundled together so the v2 features that will read them need NO second config wipe. sizeof(confStruct) 176->184 (float+u16+u16, naturally aligned, no tail pad); static_assert updated to 184. defaultConf carries the factory default configuration (compass cal fields made explicit, neutral). Behavior-IDENTICAL control law — config-layer only, no FM/RTM logic change. SPIFFS config IS reset by this flash (struct size changed); this is the one intended config-wipe event.
// V2.5-Evo - 2026-07-20 - FM control brain (Fable v1.4): repurposed the unused reserved_tx_imu telemetry byte (index 16) as fm_flags — the coherent FM engagement sub-state the TX display consumes ([0]armed [1]engaged [2]armed-not-ready [3]fault-stop-sticky). No confStruct change, no telemetry-packet size change (byte was already present) — SW_VERSION stays 33, sizeof(confStruct) stays 176, SPIFFS config is NOT reset by this flash.
// V2.5-Evo - 2026-07-19 - P3 FM: added fm_rx_active + fm_throttle_cap runtime atomics for the Follow-Me state machine. No confStruct change (FM reuses the 8 existing FM params) — SW_VERSION stays 33, sizeof stays 176, SPIFFS config is NOT reset by this flash.
// V2.5-Evo - 2026-07-20 - FM engagement semantics: added fm_mode_last_rx_ms atomic (0xF2 declaration age, drives the 95 s mode-age expiry); R6 comment cleanup on the zone_angle_enter/exit + near_diag_offset block (described a non-existent engagement cone, wrong mode numbers, inverted signs, false "CURRENTLY UNUSED"). No confStruct change — sizeof stays 176, SW_VERSION stays 33, SPIFFS config is NOT reset by this flash.
// V2.5-Evo - 2026-07-19 - FM triage: log the steering byte actually applied by calcPWM() (g_effective_steer global + VescLogData.effective_steer_log); VescLogData sizeof 52→53; old SPIFFS logs misparse after this flash; no confStruct change, SW_VERSION unchanged
// V2.5-Evo - 2026-07-18 - FM mode mapping canonicalized to TX convention (1=Near-Right, 2=Behind, 3=Near-Left). Labels/comment only — no struct/SW_VERSION change. [2026-07-24 F4 correction: the earlier "2→1 preserves Near-Right default" note was stale — that edit never landed; defaultConf.followme_mode is and stays 2 (Behind), the shipped defensive default. All surfaces now agree on 2.]
// V2.5-Evo - 2026-05-22 - SW32: Two-phase RTM throttle: rtm_align_threshold_deg + rtm_target_speed_kmh; sizeof 164→172; SW_VERSION 31→32
// V2.5-Evo - 2026-05-09 - Bundle 9-Final: Added USB CDC On Boot compile-time guard
// V2.5-Evo - 2026-05-11 - E7 Fix: VescLogData +1 byte (error_code_log); sizeof 51→52; old SPIFFS logs misparse after this flash
// V2.5-Evo - 2026-05-08 - Bundle 1: RTM/FM steering preset system (rtm_steer_response 0-4); SW_VERSION 30→31; sizeof unchanged at 164; VescLogData +4 bytes for tuning telemetry
// V2.5-Evo - 2026-05-06 - LOG-EXT-1: VescLogData extended with heading source debug fields (12 fields, +18 bytes)
// V2.5-Evo - 2026-05-06 - D3-Fix: rtm_use_compass + rtm_cog_min_speed_kmh changed uint8_t→uint16_t for ConfigService CFG_U16 compatibility; SW_VERSION 29→30; sizeof 160→164
// V2.5-Evo - 2026-05-06 - D3: Added rtm_use_compass + rtm_cog_min_speed_kmh; sizeof stays 160 (fills tail pad); SW_VERSION 28→29
// V2.5-Evo - 2026-05-01 - Release: DEBUG_RX commented out for production build
// V2.5-Evo - 2026-04-30 - RTM approach decel zone: rtm_approach_zone_m SPIFFS param; rtm_approach_cap atomic global; sizeof 156→160
// V2.5-Evo - 2026-04-30 - Rename: gps_max_jump_kmh → gps_max_teleport_kmh (clarity)
// V2.5-Evo - 2026-04-29 - Bundle B: vesc_timeout_s SPIFFS param replaces hardcoded 20s VESC timeout
// V2.5-Evo - 2026-04-22 - Added gps_chip_type field to confStruct (GPS module selector); sizeof 108→112; updated defaultConf
// V2.5-Evo - 2026-04-22 - Added Phase A GPS anti-spoofing params to confStruct; sizeof 112→128; updated defaultConf
// V2.5-Evo - 2026-04-24 - Added rx_tx_gps_lat/lng/timestamp globals for 0xF3 meta-packet reception
// V2.5-Evo - 2026-04-24 - Added Phase B GPS handshake params to confStruct; sizeof 128→136; updated defaultConf
// V2.5-Evo - 2026-04-25 - P7: Added RTM Phase C + RX safety params; VESC_MORE_VALUES; sizeof 136→152
// V2.5-Evo - 2026-04-27 - P8: TelemetryPacket adds rtm_distance at index 5; link_quality moved to index 6
// V2.5-Evo - 2026-04-25 - P7: Added rtm_rx_active, rtm_rx_emergency_stop, rtm_steer_override, fm_mode_runtime globals
// V2.5-Evo - 2026-04-25 - P7 fix: Changed RTM volatile globals to std::atomic for cross-core safety (core 0 PWM task / core 1 loop task)

// ============================================================
// V2.5-Evo - 2026-05-09 - Bundle 9-Final: USB CDC On Boot guard
//
// ESP32-C3 chip-level hardware default: GPIO 18 = USB D-, GPIO 19 = USB D+.
// RX firmware uses GPIO 18/19 as UART for the BN-880 GPS via Serial1.
// If "USB CDC On Boot" is enabled at compile time, the ESP32-C3 USB
// peripheral claims GPIO 18/19 internally and Serial1.begin() silently
// fails — GPS init never reaches the module, no fix is ever acquired,
// hours of debugging follow.
//
// REQUIRED: Arduino IDE → Tools → USB CDC On Boot → Disabled
//   OR     arduino-cli --fqbn esp32:esp32:esp32c3:CDCOnBoot=default
//
// Debug Serial output goes via UART0 (GPIO 20/21) → CH340 USB-to-UART chip
// → USB connector. Same physical USB cable, same COM port, no debug loss.
// ============================================================
#if defined(ARDUINO_USB_CDC_ON_BOOT) && (ARDUINO_USB_CDC_ON_BOOT != 0)
#error "RX firmware requires USB CDC On Boot = Disabled. ESP32-C3 USB peripheral claims GPIO 18/19 (used by Serial1 for GPS) when CDC On Boot is enabled. Set Tools -> USB CDC On Boot -> Disabled in Arduino IDE, OR pass :CDCOnBoot=default to arduino-cli's --fqbn argument. See file header for full explanation."
#endif

/*
** Includes
*/
#include <Arduino.h>
#include <atomic>
#include "../Common/FollowMeGeometry.h"
#include "../Common/FollowMeMinDistance.h"
#include "../Common/SteeringCurve.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include <RadioLib.h> //V7.1.2
#include <Wire.h>
#include <Adafruit_AW9523.h> //V1.0.5, BusIO 1.17.0
#include "driver/rmt_tx.h"
#define RMT_TX_GPIO_NUM  GPIO_NUM_9
#include <Ticker.h>
#include "esp_task_wdt.h"
// V2.5-Evo - 2026-07-25 - STAGE 0: esp_cpu_get_cycle_count() for loop() timing. It is a single
// CSR read (~5 ns) versus ~1 us for micros(), so measuring the loop does not distort the loop.
#include "esp_cpu.h"
#include "FS.h"
#include "SPIFFS.h"
#include "mbedtls/base64.h"

// Uncomment the line below to enable WiFi AP configuration mode
#define WIFI_ENABLED

#ifdef WIFI_ENABLED
#include <WiFi.h>
#include <WebServer.h>
#endif

#include "vesc_datatypes.h"
#include "vesc_buffer.h"
#include "vesc_crc.h"

#include <TinyGPS++.h> //TinyGPSPlus 1.0.3 Mikal Hart

#define SW_VERSION 35  // V2.5-Evo — 35 = mag_orientation appended (compass mounting rotation); sizeof 184->192 (mag_orientation + 2 reserved slots banked for future no-bump features), config IS reset by this flash. 34 = added fm_engage_dist_m / auton_runtime_cap_s / fm_steer_reposition_en reserved slots + defaultConf carries factory default config (compass cal, near_diag_offset 45); first flash resets all RX SPIFFS config to defaults. NOTE (2026-07-25, STAGE 0 PART A): the third of those slots has since been RENAMED IN PLACE to log_level — same offset, same uint16_t, sizeof(confStruct) still 184 — so this stays 34 and NO further config wipe happens.
const char* CONF_FILE_PATH = "/data.txt";
const char* BC_FILE_PATH = "/batconf.txt";

/*
** Structs
*/
struct confStruct {
    //Version
    uint16_t version;
    
    uint16_t radio_preset; //1: 868MHz (EU), 2: 915MHz (US/AU)
    int16_t rf_power; //Tx power from -9 to 22

    uint16_t steering_type; //0: single motor, 1: diff motor, 2: servo
    uint16_t steering_influence; //How much (percentually) the steering influences the motor speeds
    uint16_t steering_inverted; //If steering is inverted or not
    int16_t trim; //Trim the steering

    //PWM min and max
    uint16_t PWM0_min;
    uint16_t PWM0_max;
    uint16_t PWM1_min;
    uint16_t PWM1_max;

    uint16_t failsafe_time; //Time after last packet until failsafe

    //Foil battery voltage settings
    uint16_t foil_num_cells; //Amount of cells in series e.g. 14 for a "14SxP" pack

    //Sensors
    uint16_t bms_det_active;
    uint16_t wet_det_active;

    uint16_t rtm_steer_response;  // Steering preset index 0-4 for RTM/FM heading controller.
                                  // 0 = Very Soft (big waves, aggressive surfer)
                                  // 1 = Soft (choppy water)
                                  // 2 = Normal (DEFAULT — mixed conditions)
                                  // 3 = Sharp (calm water, RC use)
                                  // 4 = Very Sharp (glass-flat, no waves)
                                  // Drives kSteerPresets[] table in RTMState.ino which
                                  // sets PID gains (Kp, Kd) + bearing filter time constant.

    //UART config
    uint16_t data_src; //0: off, 1:analog, 2: VESC UART

    // GPS features related flags
    uint16_t gps_en;         // GPS runtime enable flag (0=disabled, 1=enabled)
    uint16_t followme_mode;  // 0=off, 1=rear-right, 2=behind, 3=rear-left, 4=front-left, 5=front, 6=front-right
    uint16_t kalman_en;      // Kalman filter runtime enable flag (0=disabled, 1=enabled)

    //Follow-me
    float boogie_vmax_in_followme_kmh; // F1-F6 catch-up target/in-band ceiling; 0 opens catch-up speed cap
    float min_dist_m; // minimum allowed distance to the foiler
    float followme_smoothing_band_m; // smoothing band above min distance
    // V2.5-Evo - 2026-08-27 - RENAMED IN PLACE from the retired foiler_low_speed_kmh slot.
    // Same float and offset: no layout change and no SW_VERSION bump. Existing configurations are
    // recognised by steer_reduction_start_pct still containing its old reserved value and are
    // promoted to the 35% default by cfgValidateCrossField() before normal range validation.
    float steer_full_throttle_pct; // 20-100%. Steering authority retained at full effective throttle.
    // V2.5-Evo - 2026-07-20 - R6: comment block corrected. It previously described an
    // "engagement cone" gate that does not exist, marked all three params "CURRENTLY UNUSED"
    // (the FM geometry consumes all three), and gave the wrong mode numbers with the wrong
    // signs for the diagonal offset. The values and ranges themselves are unchanged.

    // FOLLOW-GEOMETRY SCHMITT — ENTER half-angle (degrees).
    // F1/F3: decides whether the buggy is lined up closely enough BEHIND the rider to apply the
    // diagonal side offset, or whether it should just sit directly behind. Measured from the
    // directly-behind axis; it is not an F1/F3 engagement gate.
    // F4-F6: reused as the inner target-axis warning threshold. It does not gate control.
    // Range: 0-180°. Default 35°.
    float zone_angle_enter_deg;

    // FOLLOW-GEOMETRY SCHMITT — EXIT half-angle (degrees). F1/F3 drop the diagonal beyond it;
    // F4-F6 raise their front-position warning beyond it but keep steering/cap/state/latch.
    // MUST be > zone_angle_enter_deg by 5-15° so noise cannot flap either decision.
    // Range: 0-180°. Default 45°.
    float zone_angle_exit_deg;

    // NEAR-MODE DIAGONAL OFFSET (degrees from "directly behind the rider").
    // Applied as target_bearing = rider_course + 180 + offset, in this board's one bearing
    // convention (degrees CLOCKWISE from North), where "Near-Right"/"Near-Left" mean the side
    // the buggy ends up on RELATIVE TO THE RIDER as the rider faces along their course:
    //   followme_mode=1 (Near Right): offset = -near_diag_offset_deg  (behind-and-right)
    //   followme_mode=2 (Behind)    : offset = 0
    //   followme_mode=3 (Near Left) : offset = +near_diag_offset_deg  (behind-and-left)
    // 0° = directly behind, 90° = beside the rider. Diagonal placement keeps the buggy out of
    // the rider's wake/spray path. Authoritative derivation: the OFFSET SIGN CONVENTION block
    // above computeFmTarget() in RTMState.ino.
    // Range: 0-180°. Default 45°.
    float near_diag_offset_deg;
    
    //System parameters
    float ubat_cal; //ADC to volt cal for bat meas
    float ubat_offset; //Offset to add to analog/vesc measurement

    uint16_t tx_gps_stale_timeout_ms; // TX GPS data stale timeout (ms)

    //Logger
    uint16_t logger_en; // BREmote Logger runtime enable flag (0=disabled, 1=enabled)

    //Comms
    uint16_t paired;
    uint8_t own_address[3];
    uint8_t dest_address[3];
    char wifi_password[8];  // WPA2 AP password, exactly 8 chars (no null terminator)

    // ---> NEW COMPASS CALIBRATION VARIABLES <---
    int16_t mag_offset_x;
    int16_t mag_offset_y;
    float mag_scale_x;
    float mag_scale_y;

    // ============================================================
    // V2.5-Evo - 2026-04-22 - GPS CHIP TYPE SELECTOR
    //
    // !!! IMPORTANT: Adding this field changed sizeof(confStruct)  !!!
    // !!! from 108 bytes to 112 bytes.                             !!!
    // !!! On first V2.5-Evo boot after this change, SPIFFS config  !!!
    // !!! failed the size check — ALL RX SETTINGS RESET TO        !!!
    // !!! DEFAULTS. One-time migration; complete on existing units. !!!
    // ============================================================
    uint16_t gps_chip_type;  // 0=BN-220, 1=BN-880+compass (default), 2=M10 no compass, 3=M10+compass; range 0-3

    // ============================================================
    // V2.5-Evo - 2026-04-22 - PHASE A GPS ANTI-SPOOFING PARAMETERS
    //
    // These four parameters control the always-on Phase A anti-
    // spoofing filter in GPS.ino. A reading is rejected if ANY
    // check fails. After gps_suspect_threshold consecutive
    // rejections, gps_rejected is set and RTM arming is blocked.
    //
    // !!! Adding these fields changed sizeof(confStruct) 112→128.  !!!
    // !!! On first V2.5-Evo boot after this change, SPIFFS reset  !!!
    // !!! ALL settings to defaults. One-time migration; complete.  !!!
    // ============================================================
    float    gps_max_hdop;            // Max HDOP for a valid fix; range 0.5-5.0; default 2.0; dimensionless
    float    gps_max_accel_g;         // Max implied acceleration between readings; range 1.0-10.0G; default 3.0G
    float    gps_max_teleport_kmh;        // Max position-implied speed for teleport check; range 50-500 km/h; default 80
    uint16_t gps_suspect_threshold;   // Consecutive failures before GPS marked rejected; range 1-10; default 3

    // ============================================================
    // V2.5-Evo - 2026-04-24 - PHASE B GPS HANDSHAKE ANTI-SPOOFING PARAMETERS
    //
    // These two parameters control Phase B, which runs every time a
    // 0xF3 GPS meta-packet is received from TX (at most every 30s).
    //
    // Distance check: TX-RX Haversine distance must be <
    //   gps_max_pair_dist_m or RTM arming is blocked.
    // Speed consistency check: TX implied speed (from consecutive
    //   meta-packet positions) must be within gps_max_speed_diff_kmh
    //   of RX GPS speed or arming is blocked.
    //
    // !!! Adding these fields changes sizeof(confStruct) 128→136. !!!
    // !!! On first flash after this change, SPIFFS resets ALL      !!!
    // !!! settings to defaults. After flashing:                    !!!
    // !!!   1) Re-pair TX and RX                                   !!!
    // !!!   2) Re-configure all settings via web UI                !!!
    // !!!   3) Re-calibrate compass (runcal)                       !!!
    // !!!   4) Verify Phase B defaults (500 m, 50 km/h)            !!!
    // ============================================================
    float gps_max_pair_dist_m;      // Max plausible TX-RX distance at handshake; range 50-2000 m; default 500 m
    float gps_max_speed_diff_kmh;   // Max TX-RX speed difference for handshake; range 10-200 km/h; default 50 km/h

    // ============================================================
    // V2.5-Evo - 2026-04-25 - PRIORITY 7: RTM PHASE C + RX SAFETY PARAMETERS
    //
    // sizeof grows 136->152. Layout:
    //   float rtm_vesc_speed_diff_kmh  (4)
    //   float vesc_erpm_per_kmh        (4)
    //   uint16_t rtm_rx_enabled        (2)
    //   uint16_t rtm_rx_override_steer (2)
    //   uint16_t rtm_compass_required  (2)
    //   uint16_t rtm_stop_distance_m   (2) — fills former 2-byte tail padding; sizeof stays 152
    //
    // First flash of P7 firmware resets all RX settings to defaults.
    // After flashing: re-pair TX/RX, re-enter all settings, re-run runcal.
    // ============================================================
    float    rtm_vesc_speed_diff_kmh;    // retired standalone-RTM ABI slot; not read at runtime
    float    vesc_erpm_per_kmh;          // ERPM per km/h (vehicle-specific); default 0.0 (0=skip VESC check)
    uint16_t rtm_rx_enabled;             // FM/FM_RETURN master enable (historical key name)
    uint16_t rtm_rx_override_steering;   // FM automatic steering enable (historical key name)
    uint16_t rtm_compass_required;       // FM_RETURN heading-required gate (historical key name)
    uint16_t rtm_stop_distance_m;        // retired standalone-RTM ABI slot; not read at runtime

    // V2.5-Evo - 2026-04-29 - BUNDLE B: VESC UART TIMEOUT
    // Set to 6s (down from the original hardcoded 20s) to minimise stale VESC data.
    // At 20s the TX display would show a valid battery % and FET temp for up to 20s after
    // the VESC UART connection dropped — misleading the rider. 6s matches the typical
    // VESC polling cadence (data_src=2 polls every ~1s) with room for 5 missed packets.
    // Minimum 5s: going lower causes false N/A during normal VESC dropout transients
    // (e.g. heavy regen braking briefly interrupts UART). Maximum 60s for diagnostic use.
    //
    // V2.5-Evo - 2026-08-17 - FACTORY DEFAULT RAISED 6 -> 10 s. The 5-60 s validation range is
    // NOT changed; only the value baked into defaultConf below.
    // WHY: a VESC cold restart takes roughly 8-9 seconds, which is LONGER than the 6 s window.
    // So every time the VESC rebooted, the rider's battery % and FET temperature blanked to
    // "N/A" on the TX display even though nothing was actually wrong — the VESC was simply
    // booting. 10 s covers that restart with about a second to spare, and it is still half the
    // original hardcoded 20 s, so Bundle B's intent (do not show a stale reading long after a
    // real disconnect) is preserved. The price is that a GENUINE VESC disconnect now holds the
    // last reading for up to 10 s instead of 6 s before it blanks.
    // SECOND CONSUMER, CHECKED: this same field also decides when RTM Phase C check 2 SKIPS its
    // VESC-ERPM-vs-GPS-speed comparison (runPhaseC() in RTMState.ino). A longer timeout means
    // FEWER skips — the check now runs on VESC data up to 10 s old instead of 6 s. That cannot
    // weaken the check, because a skipped check and a passed check have the identical outcome
    // (no stop); it only widens the window in which slightly staler ERPM is compared against a
    // live GPS speed. And it is moot on a stock board: the check only runs when
    // vesc_erpm_per_kmh > 0, whose factory default is 0.0 (disabled).
    uint16_t vesc_timeout_s;  // 5-60 s; default 10; how long without a VESC UART packet before bat/temp shown as N/A

    // V2.5-Evo - 2026-04-30 - BUNDLE E: GPS POLLING RATE
    // V2.5-Evo - 2026-07-25 - STAGE 1: factory default raised 2 -> 10 Hz (see defaultConf below).
    uint16_t gps_update_hz;   // 1-10 Hz; default 10; how often per second to drain the GPS UART (10=100ms, 5=200ms, 2=500ms)

    // V2.5-Evo - 2026-04-30 - RTM APPROACH DECEL ZONE
    // Distance from TX at which the approach throttle ramp begins during active RTM.
    // Throttle cap = thr × (dist − rtm_stop_distance_m) / (rtm_approach_zone_m − rtm_stop_distance_m)
    // Result: full throttle at the outer edge; cap reaches 0 at rtm_stop_distance_m; Gate 9 hard stop still applies.
    // Set to 0 to disable the decel zone and use Gate 9 hard stop only.
    uint16_t rtm_approach_zone_m;  // FM_RETURN approach-band width; values below 2 use 2 m; default 12

    // ============================================================
    // V2.5-Evo - 2026-05-06 - D3: RTM HEADING SOURCE SELECTION
    //
    // These two parameters control which heading source RTM steering uses.
    // Bench-test data (?magtest) confirmed compass-only steering is unsafe
    // on this hardware: motor current biases compass heading by 100° or more
    // even at 20% throttle. GPS course-over-ground (COG) is unaffected by
    // motor EMI and is the preferred heading source whenever the buggy is
    // moving fast enough for COG to be reliable (~3 km/h default).
    //
    // Modes:
    //   0 = GPS COG only — compass disabled for steering (safest if compass biased)
    //   1 = Hybrid (DEFAULT) — GPS COG primary, compass snapshot at low speed
    //   2 = Compass only — DIAGNOSTIC USE, DO NOT USE ON WATER. Bench tests confirm
    //                      motor current biases compass by 100° or more during
    //                      operation. Setting this on water risks RTM steering
    //                      the buggy in the wrong direction. For non-EMI builds
    //                      that have proven clean compass behavior under load only.
    //
    // !!! Adding these fields changes sizeof(confStruct) 160→164,         !!!
    // !!! and bumps SW_VERSION 29→30. SPIFFS resets ALL settings to       !!!
    // !!! defaults again (the second time, because D3-Fix changes layout). !!!
    // !!! After flashing:                                                  !!!
    // !!!   1) Re-pair TX and RX                                           !!!
    // !!!   2) Re-configure all settings via web UI                        !!!
    // !!!   3) Re-calibrate compass (runcal)                               !!!
    // !!!   4) Verify rtm_use_compass = 1 (hybrid default)                 !!!
    // !!!   5) Verify rtm_cog_min_speed_kmh = 3                            !!!
    // ============================================================
    uint16_t rtm_use_compass;        // 0=GPS COG only; 1=Hybrid (default); 2=Compass only DIAGNOSTIC ONLY DO NOT USE ON WATER
    uint16_t rtm_cog_min_speed_kmh;  // Min GPS speed for COG to be primary heading source; 1-15 km/h; default 3

    // ============================================================
    // V2.5-Evo - 2026-05-22 - SW32: TWO-PHASE RTM THROTTLE CONTROL
    //
    // Phase 1 (Align): when |heading_error| > rtm_align_threshold_deg, throttle is
    //   suppressed to ~5% so the buggy pivots toward the target without driving away.
    //   At near-zero throttle, motor current is minimal — compass bias is also reduced,
    //   so hybrid heading mode has cleaner compass snapshot data during alignment.
    //
    // Phase 2 (Run): once aligned, FM_RETURN uses the shared stateful GPS-speed PI governor.
    //   rtm_target_speed_kmh is its literal 0-50 km/h target; zero commands cap 0.
    //
    // sizeof grows 164 → 172. SW_VERSION 31 → 32. First flash resets SPIFFS config.
    // ============================================================
    float    rtm_target_speed_kmh;      // FM_RETURN PI target; literal 0-50 km/h; default 4
    uint16_t rtm_align_threshold_deg;   // FM_RETURN align threshold; 10-90°; default 45

    // V2.5-Evo - 2026-06-05 - SW33: MOTOR RAMPING (seconds). Time for a motor output to rise
    // 0->full. Applied to BOTH motor channels — smooths the throttle AND prevents a single motor
    // from taking off (throttle- or steering-driven). Fall is instant (release/failsafe/e-stop/
    // straightening drop immediately). NOTE: this also ramps differential steering — a sharp turn
    // builds over this time. 0 = instant/off. sizeof grows 172->176; SW_VERSION 32->33; SPIFFS resets.
    float    motor_ramp_s;              // 0=off/instant, 0-4 s; default 0.75

    // V2.5-Evo - 2026-07-20 - SW34 reserved slots (added together so only ONE config wipe is needed).
    // They were all storage slots at SW34 so v2 features could be code-only, with no re-wipe.
    // V2.5-Evo - 2026-07-25 - A2 status update: fm_engage_dist_m is now LIVE (read by runFmLoop()).
    // V2.5-Evo - 2026-07-25 - STAGE 0 PART A status update: the third slot, fm_steer_reposition_en,
    // has been RENAMED IN PLACE to log_level and is now LIVE (read by the logger). auton_runtime_cap_s
    // is the only one of the three still RESERVED and still read by nothing.
    // fm_engage_dist_m: LIVE since 2026-07-25 (A2) — no longer RESERVED/unread. Fixed engage-distance
    //   override for the FM separation latch, read in RTMState.ino runFmLoop().
    //   0   = auto: d_engage = kFmEngageFactor (1.5) * (min_dist_m + followme_smoothing_band_m), the
    //         original behaviour, reproduced exactly.
    //   >0  = the engage distance itself, in METRES. This is NOT the rope length — set it to rope
    //         length x ~1.3 so the buggy clears the rope with margin (a 20 ft / 6.10 m rope -> 8.0).
    //         V2.5-Evo - 2026-07-25 - F3-b: legal non-zero values start at kFmEngageDistFloorM
    //         (8.0 m, defined below this struct); (0, 8) m is rejected by cfgValidateCrossField().
    //   HOW THE RIDER PICKS THIS VALUE: measure your own tow rope and set this to AT LEAST one metre
    //   more than the rope length, so Follow-Me only engages once you have genuinely let go and
    //   separated. Example: a 20 ft (6.1 m) rope -> set 8 m or more. Setting it at or below your rope
    //   length lets FM engage while you are still on the rope. 8.0 m is the enforced minimum, not a
    //   recommendation — a longer rope needs a bigger number.
    float    fm_engage_dist_m;         // 0 = auto; >0 = fixed engage distance in metres; 0, or 8-50 m
    // V2.5-Evo - 2026-08-16 - RENAMED IN PLACE: auton_runtime_cap_s -> gps_dyn_model.
    // Same offset, same uint16_t, so sizeof(confStruct) stays 184, the static_assert is
    // unchanged, SW_VERSION stays 34 and NOBODY'S CONFIG IS WIPED. Same trick as
    // fm_steer_reposition_en -> log_level (2026-07-25) and dummy_delete_me ->
    // rtm_steer_response (Bundle 1). The old field was RESERVED and never read by v1, so
    // no behaviour is displaced. Every board in the field currently holds 0 here — which is
    // why 0 MUST mean "the previous hard-coded behaviour", i.e. Sea.
    //
    // u-blox NAV5 dynamic platform model, applied by configureGPS().
    //   0 = default -> Sea (5). What every existing board already does.
    //   4 = Automotive — REQUIRED above ~500 m; Sea's altitude ceiling is 500 m.
    //   5 = Sea (explicit) — best below 500 m: constrains the filter to ~25 m/s and pins
    //       altitude, which sharpens course-over-ground, and COG is what Follow-Me steers on.
    // Deliberately NOT offering dynModel 0 (Portable): it permits 310 m/s and is what produced
    // the bogus 254 km/h / 4800 m HIGH-CONFIDENCE fixes. No reason to expose it.
    uint16_t gps_dyn_model;            // 0 = default (Sea) | 4 = Automotive | 5 = Sea
    // ============================================================
    // V2.5-Evo - 2026-07-25 - STAGE 0 PART A: log_level (IN-PLACE RENAME, NO SPIFFS WIPE)
    //
    // This slot used to be fm_steer_reposition_en — a RESERVED Option-C storage slot added at
    // SW34 that no code ever read and that has therefore been sitting in every stored config as
    // a plain 0. It is RENAMED IN PLACE here: same position in the struct, same uint16_t type,
    // so sizeof(confStruct) stays 184, the static_assert below is unchanged, SW_VERSION stays 34
    // and the owner's saved settings are NOT reset by this flash. There is precedent for exactly
    // this move: dummy_delete_me -> rtm_steer_response (Bundle 1, 2026-05-08).
    //
    // WHY THE RENAME RATHER THAN A NEW FIELD: confStruct was 184 bytes with no tail padding, so
    // appending anything grows it, which fails the SPIFFS size check on the next boot and wipes
    // the rider's entire configuration (pairing, calibration, all tuning). Reusing a dead slot
    // that is already 0 everywhere costs nothing and wipes nothing.
    // (V2.5-Evo - 2026-08-17 - tense fix: the 184 above is the SW34-era size this rename was
    // written against. The struct is 192 bytes today — SW35 appended mag_orientation and two
    // reserved slots and spent the one config wipe. The static_assert below is the SSOT for the
    // current size; every "184" in this block is history, not a current fact.)
    //
    // WHAT THE FM v2 FEATURE DOES NOW: the Option-C "steer reposition" idea has NOT been
    // implemented and has NOT been abandoned — when FM v2 lands it must claim a FRESH field of
    // its own (which will be a deliberate, announced config-wipe event), not this slot.
    //
    // WHAT THE VALUES MEAN — how much detail each recorded log line carries:
    //   0 = UNSET. Behaves EXACTLY as level 3. This is what every existing config already
    //       stores, so nothing changes for anyone who never touches this setting.
    //   1 = Basic   — RESERVED for a future storage optimisation; CURRENTLY LOGS AS LEVEL 3.
    //   2 = VESC    — RESERVED for a future storage optimisation; CURRENTLY LOGS AS LEVEL 3.
    //   3 = Developer — the full 59-byte record this firmware has always written.
    //   4 = Deep      — Developer plus GPS/loop, Follow-Me engage and heading-evidence audits
    //       (mode/state/block reason, gates, COG/snapshot validity and disagreement) = 96 bytes.
    // Levels 1 and 2 are ACCEPTED by the validator (so a rider can set them and a future
    // firmware will honour them) but are deliberately NOT silently ignored: they are documented
    // everywhere as falling back to level 3 until the smaller records are implemented.
    //
    // The level is latched once per log FILE (createNewLogFile() in Logger.ino) and written into
    // that file's header, so changing this setting mid-session never corrupts an open file.
    // ============================================================
    uint16_t log_level;                // 0 = unset (= level 3); 1 = Basic*, 2 = VESC*, 3 = Developer, 4 = Deep. (*accepted, currently logs as level 3.) Range 0-4.

    // V2.5-Evo - 2026-08-16 - SW34->35: mag_orientation. NEW FIELD, appended at the END of
    // confStruct so every existing offset is unchanged, and SW_VERSION 34 -> 35, which DOES reset
    // config on first boot. This is the one intended wipe for this bump.
    // V2.5-Evo - 2026-08-17 - SIZE CORRECTION: this comment said "sizeof 184 -> 188 (the uint16 plus
    // 2 bytes of 4-byte alignment padding)". THE FINISHED STRUCT IS 192 BYTES, not 188 — see the
    // static_assert below, which is the SSOT. 188 was the size mag_orientation alone would have
    // produced, but the two banked RESERVED slots (rsvd_u16_1 + rsvd_f32_1, documented under this
    // field) landed in the SAME edit and account for the other 4 bytes: 184 + 2 (mag_orientation)
    // + 2 (rsvd_u16_1) = 188, + 4 (rsvd_f32_1) = 192, naturally aligned with no tail pad.
    // THIS IS NOT COSMETIC: the SW34 -> SW35 config-backup migration is pinned to the exact byte
    // counts 184 (legacy) and 192 (current) and disables itself if either stops matching, so a
    // comment claiming 188 would have someone reasoning from the wrong number about code that
    // writes bytes into the live config.
    //
    // Mounting rotation of the compass module about the vertical axis, in degrees, applied to the
    // computed heading. Set by ?compasscal (which now starts and ends pointing north) or by
    // ?magalign. Snapped to the four cardinal values: a 3.2 deg idle noise floor cannot justify
    // finer resolution, and nobody glues a module at 37 degrees.
    //   0 / 90 / 180 / 270
    // MIRRORING is NOT stored here - a mirrored sensor frame is stored as a NEGATIVE mag_scale_y,
    // because negating cal_y is exactly the mirror fix and that field already exists.
    uint16_t mag_orientation;          // 0 | 90 | 180 | 270 degrees


    // V2.5-Evo - 2026-08-27 - RENAMED IN PLACE from rsvd_u16_1. Same uint16_t and offset,
    // so sizeof(confStruct) remains 192 and SW_VERSION remains 35. Stored zero is the legacy
    // marker used by cfgValidateCrossField() to initialise both steering settings to 50% / 35%.
    uint16_t steer_reduction_start_pct; // 30-80%. Full authority at and below this effective throttle.

    // V2.5-Evo - 2026-08-27 - RENAMED IN PLACE: rsvd_f32_1 -> fm_diverge_dist_m.
    // Absolute distance ceiling for the sustained non-closing divergence detector. Runtime clamps
    // it to at least 2 x effective D_engage and at most 100 m. 0 is the compatibility encoding for
    // existing SW35 configs: derive the old 6 x D_engage value, then cap it at 100 m.
    float    fm_diverge_dist_m;         // 0=legacy auto; explicit 1-100 m, effective min 2 x D_engage


};
static_assert(sizeof(confStruct) == 192, "confStruct size mismatch — expected 192 bytes. Update this assert if you change the struct.");  // 176->184: +fm_engage_dist_m(float 4) +auton_runtime_cap_s(u16 2) +fm_steer_reposition_en(u16 2), all naturally aligned, no tail pad (2026-07-20 SW34)  // 172->176 motor_ramp_s float (2026-06-05 SW33)  // 112->128 Phase A; 128->136 Phase B; 136->152 P7 RTM; 152->156 Bundle B; 156 unchanged BundleE; 156->160 rtm_approach_zone_m (uint16_t + 2-byte tail pad) (2026-04-30); D3 rtm_use_compass + rtm_cog_min_speed_kmh (2x uint8_t) fill the 2-byte tail pad — sizeof stays 160 (2026-05-06); D3-Fix: uint8_t→uint16_t for ConfigService compatibility, sizeof unchanged at 164 (2026-05-06); Bundle 1: dummy_delete_me renamed to rtm_steer_response in-place, sizeof unchanged at 164 (2026-05-08); STAGE 0 PART A: fm_steer_reposition_en renamed to log_level in-place — same offset, same uint16_t, sizeof STILL 184 and SW_VERSION STILL 34, so this flash does NOT reset SPIFFS config (2026-07-25); auton_runtime_cap_s renamed to gps_dyn_model in-place, sizeof STILL 184, SW_VERSION STILL 34 (2026-08-16); 184->192 SW34->35: +mag_orientation(u16 2) +rsvd_u16_1(u16 2) +rsvd_f32_1(float 4), appended at the tail, naturally aligned, no tail pad — the one intended config wipe for this bump (2026-08-16). THIS NUMBER IS THE SSOT: the SW34->35 config-backup migration is pinned to 184 (legacy) and 192 (current) and disables itself if either stops matching, so any prose elsewhere that disagrees with the 192 above is stale and must be corrected rather than trusted.
confStruct usrConf;
  //The orginal confs were:  ##// confStruct defaultConf = {SW_VERSION, 1, 0, 0, 50, 0, 0, 1500, 2000, 1500, 2000, 1000, 10, 0, 1, 0, 0, 0, 0, 0, 25.0f, 10.0f, 10.0f, 5.0f, 35.0f, 45.0f, 45.0f, 0.0095554f, 0.0, 1000, 1, 0, {0, 0, 0}, {0, 0, 0}, {'1','2','3','4','5','6','7','8'}};
  // Factory default configuration.
confStruct defaultConf = {SW_VERSION, 2, 22, 1, 50 /*steering_influence: conventional default (0-100)*/, 0 /*steering_inverted: 0 = conventional default; a fresh build MUST verify steering direction wheels-up (FM steers toward rider) before trusting FM.*/, 0, 1000, 2000, 1000, 2000, 1000, 10, 0, 1, 2, 2, 1, 2, 1, 25.0f, 10.0f, 10.0f, kSteerFullThrottleDefaultPct /*steer_full_throttle_pct*/, 35.0f, 45.0f, 45.0f, 0.0095554f, 0.0f, 3000, 0, 0, {0, 0, 0}, {0, 0, 0}, {'1','2','3','4','5','6','7','8'}, // wifi_password below: documented DEFAULT AP password "12345678" — change before use
  // V2.5-Evo - 2026-04-22 - Compass calibration fields (previously implicit zeros).
  // Made explicit here so gps_chip_type can follow. Safe neutral values:
  // offsets=0 (no bias), scales=1.0f (unity gain = no correction applied).
  0, 0,   // mag_offset_x, mag_offset_y (neutral zero bias; re-derived via 'runcal')
  1.0f, 1.0f, // mag_scale_x, mag_scale_y (unity gain = no correction until calibrated)
  // V2.5-Evo - 2026-04-22 - GPS chip type: 1 = BN-880 (GPS+compass). RX default.
  1,          // gps_chip_type (1 = BN-880 + compass; run 'runcal' after first boot)
  // V2.5-Evo - 2026-04-22 - Phase A GPS anti-spoofing defaults
  2.0f,       // gps_max_hdop:           max HDOP for valid reading (range 0.5-5.0)
  3.0f,       // gps_max_accel_g:        max implied acceleration (range 1.0-10.0 G)
  80.0f,      // gps_max_teleport_kmh:       max teleport-implied speed (range 50-500 km/h; default lowered 200→80 2026-04-30)
  3,          // gps_suspect_threshold:  consecutive failures before GPS rejected (range 1-10)
  // V2.5-Evo - 2026-04-24 - Phase B GPS handshake anti-spoofing defaults
  500.0f,     // gps_max_pair_dist_m:    max TX-RX pairing distance (range 50-2000 m)
  50.0f,      // gps_max_speed_diff_kmh: max TX-RX speed difference (range 10-200 km/h)
  // V2.5-Evo - 2026-04-25 - Priority 7 RTM Phase C + RX safety defaults
  20.0f,      // rtm_vesc_speed_diff_kmh: max GPS vs VESC speed diff (5-50 km/h)
  0.0f,       // vesc_erpm_per_kmh: 0 = skip Phase C VESC check until calibrated
  1,          // rtm_rx_enabled: 1 = RTM enabled on RX side
  1,          // rtm_rx_override_steering: 1 = RTM may override steering
  1,          // rtm_compass_required: 1 = compass required for RTM arming
  // V2.5-Evo - 2026-04-26 - CRITICAL FIX: rtm_stop_distance_m was missing from defaultConf; zero-init
  // would have set it to 0, making Gate 9 check (dist_m < 0.0f) never fire — permanently
  // disabling the hard stop that prevents the buggy from hitting the user.
  10,  // rtm_stop_distance_m: safe default 10 m (>= 8 m GPS floor); RTM hard-stop radius
  // V2.5-Evo - 2026-04-29 - Bundle B: vesc_timeout_s replaces hardcoded 20s VESC connection timeout
  // V2.5-Evo - 2026-08-17 - raised 6 -> 10 s. A VESC cold restart takes roughly 8-9 s, so at 6 s
  // the rider's battery % and FET temperature blanked to N/A on the TX across every restart even
  // though the VESC was only booting. 10 s covers it and is still half the original hardcoded 20 s.
  // VALUE-ONLY change: no field added, moved or resized, and the 5-60 range already in kCfgFields is
  // unchanged, so sizeof(confStruct) stays 192, SW_VERSION stays 35 and SPIFFS config is NOT reset.
  // NOTE: defaultConf only applies to units whose config is reset. A board with a STORED
  // vesc_timeout_s keeps its old value (6) after this flash — set it explicitly on the device with
  // `?set vesc_timeout_s 10` then `?save`.
  10,         // vesc_timeout_s: seconds without VESC UART packet before bat/temp shown as N/A (range 5-60s; default 10s)
  // V2.5-Evo - 2026-04-30 - Bundle E: gps_update_hz replaces hardcoded 1Hz GPS poll cadence
  // V2.5-Evo - 2026-07-25 - STAGE 1: default raised 2 -> 10 Hz. The GPS module is configured for
  // 5 Hz (BN-220/BN-880) or 10 Hz (M10), so draining twice a second left ~1250 bytes pending per
  // drain now that STAGE 1 lets the module stream continuously into the ring. 10 Hz drains every
  // ~100 ms, keeping the ring occupancy far below its 2048-byte capacity at all times.
  // VALUE-ONLY change: no field added, moved or resized; the 1-10 range already in kCfgFields is
  // unchanged, sizeof(confStruct) stays 184, SW_VERSION stays 34, SPIFFS config is NOT reset.
  // NOTE: defaultConf only applies to units whose config is reset. A board with a STORED
  // gps_update_hz keeps its old value (2) after this flash — set it explicitly on the device.
  10,         // gps_update_hz: GPS NMEA polling rate in Hz (range 1-10 Hz; default 10 Hz = 100ms interval)
  // V2.5-Evo - 2026-04-30 - RTM approach decel zone default
  12,         // rtm_approach_zone_m: outer edge of RTM throttle decel zone (0=disabled, 5-100 m)
  // V2.5-Evo - 2026-05-06 - D3: RTM heading source selection defaults
  1,          // rtm_use_compass: 1 = Hybrid (GPS COG primary, compass snapshot at low speed). 0=COG only, 2=compass only DIAGNOSTIC.
  3,          // rtm_cog_min_speed_kmh: GPS speed threshold below which compass snapshot is used; 1-15 km/h; default 3
  // V2.5-Evo - 2026-05-22 - SW32: Two-phase RTM throttle defaults
  4.0f,       // rtm_target_speed_kmh: FM_RETURN PI target; literal 0-50 km/h; default 4
  45,         // rtm_align_threshold_deg: heading error threshold for Phase 1→2 transition; 45° default
  // V2.5-Evo - 2026-06-05 - SW33: motor ramping (secs) default
  0.75f,      // motor_ramp_s: motors ramp 0->full over 0.75s (0=instant/off, 0-4s); also ramps steering
  // V2.5-Evo - 2026-07-20 - SW34 slots. 2026-07-25 A2: fm_engage_dist_m is now live; the other two stay reserved/unread.
  0.0f,       // fm_engage_dist_m: 0 = auto (RTMState computes d_engage from min_dist + band); >0 = fixed engage distance in metres
  0,          // gps_dyn_model: 0 = default -> Sea (was auton_runtime_cap_s, renamed in place 2026-08-16)

  // V2.5-Evo - 2026-07-25 - STAGE 0 PART A: this slot was fm_steer_reposition_en, renamed in place
  // to log_level. The default stays 0 on purpose — 0 means "unset" and behaves exactly as level 3
  // (Developer), which is the behaviour every unit already has, so nothing changes on flash.
  0,          // log_level: 0 = unset -> logs as level 3 (Developer). 1/2 accepted but currently log as 3; 4 = Deep.
  0,          // mag_orientation: 0 deg. Set by ?compasscal (north-to-north) or ?magalign.
  kSteerReductionStartDefaultPct, // steer_reduction_start_pct: full authority through 50% effective throttle

  100.0f        // fm_diverge_dist_m: absolute divergence ceiling; runtime min 2 x D_engage



};

// ============================================================
// V2.5-Evo - 2026-07-25 - F3-b: FM ENGAGE-DISTANCE FLOOR (one shared definition)
//
// Hard floor for a MANUAL fm_engage_dist_m override, in metres. 0 (auto) bypasses it entirely,
// because auto derives the engage distance from min_dist_m + followme_smoothing_band_m instead.
//
// WHAT THIS NUMBER GUARDS: since A2, fm_engage_dist_m IS the Follow-Me engage distance in metres —
// how far the rider must be from the buggy before FM may engage for the first time. The whole point
// of the separation latch is that the rider must be OFF the tow rope before FM engages.
//
// WHAT THE BUG WAS: the kCfgFields row for the field only range-checks 0-50 m, so a small value such
// as 3 m was accepted and stored. An engage distance shorter than the rope does not tune the
// interlock, it DEFEATS it — FM engages with the rider still on the rope, i.e. autonomous steering
// mid-tow. The first fix (F3) set this floor to 5.0 m, which was still BELOW the hazard it named:
// the owner's tow rope is 20 ft = 6.10 m, so 5.0-6.1 m was still storable and still on-rope.
// 8.0 m clears a 6.10 m rope by ~1.31x, and it also sits above the owner's own follow geometry
// (min_dist 4 m + smoothing band 2 m = 6 m), so the interlock is a real gate rather than a no-op.
//
// WHERE IT IS USED — exactly two places, both reading THIS definition; there is no second copy:
//   1. cfgValidateCrossField() in ConfigService.ino — refuses to STORE anything in (0, 8) m.
//   2. runFmLoop() in RTMState.ino — defensive clamp UP to this floor for a value already sitting in
//      SPIFFS from before the rule existed (a stored config is never re-validated when it is loaded).
// Both are .ino files, and this header is included at the top of V2_Integration_Rx.ino, which the
// Arduino build concatenates first — so the constant is visible to both. (An earlier comment claimed
// concatenation order prevented sharing and used that to justify a duplicated literal. It was wrong.)
// ============================================================
static const float kFmEngageFactor       = 1.5f;   // auto D_engage multiplier on follow distance
static const float kFmEngageDistFloorM   = 8.0f;   // smallest effective D_engage, metres

// One shared D_engage calculation for ConfigService validation and the FM state machine.
static inline float fmEffectiveEngageDistanceFromConfig(const confStruct &conf)
{
  float d_follow = conf.min_dist_m + conf.followme_smoothing_band_m;
  if (d_follow < 0.5f) d_follow = 0.5f;

  float d_engage = (conf.fm_engage_dist_m > 0.1f)
      ? conf.fm_engage_dist_m
      : (kFmEngageFactor * d_follow);
  if (d_engage < kFmEngageDistFloorM) d_engage = kFmEngageDistFloorM;
  return d_engage;
}

// Absolute FM divergence-distance rules. Zero remains a compatibility encoding because every SW35
// board stored zero while this slot was unused; it reconstructs the old 6 x D_engage behavior, then
// applies the new 100 m maximum. Explicit values are metres and can never be effective below
// 2 x D_engage. If that minimum itself exceeds 100 m, the requested absolute maximum wins.
static const float kFmDivergeLegacyFactor   = 6.0f;
static const float kFmDivergeMinEngageRatio = 2.0f;
static const float kFmDivergeMaxDistM        = 100.0f;

static inline float fmMinimumDivergeDistanceFromConfig(const confStruct &conf)
{
  float minimum = kFmDivergeMinEngageRatio * fmEffectiveEngageDistanceFromConfig(conf);
  return (minimum < kFmDivergeMaxDistM) ? minimum : kFmDivergeMaxDistM;
}

// Follow-Me manual-steering takeover threshold, in raw steering counts either side of centre.
// A deliberate deflection at or beyond this threshold wins over FM's automatic steering while
// the stick is held there; centring the stick hands steering back to FM. The FM state, separation
// latch and throttle cap stay active throughout. This lives in the header because PWM.ino is
// compiled before RTMState.ino and both control paths must use the same threshold.
static const uint8_t kFmManualSteerDeadband = 40;   // counts from steering centre 127

// FM_RETURN target-speed range. The setting itself already exists as a float in confStruct, so
// widening its accepted values changes neither layout nor SW_VERSION. Zero is a real zero-speed
// target; a non-zero boogie_vmax_in_followme_kmh may still clamp it further at runtime.
static const float kFmReturnTargetSpeedMaxKmh = 50.0f;

// ============================================================
// V2.5-Evo - 2026-07-25 - STAGE 2: HEADING-SOURCE TRUST CONSTANTS (RTM + FM)
//
// WHY THESE EXIST — MEASURED ON THE BENCH, NOT THEORISED
//   ?diag on the repaired board reported:
//       COG : 7.4 timestamp-updates/s vs 0.0 value-changes/s   [value frozen 533 s]
//   The GPS module pushed a course update about seven times a second for nine minutes, and every
//   single one carried THE SAME NUMBER. getRtmHeading()'s freshness test is
//   (millis() - gps_last_course_ms) < 1500 ms, and that timestamp was being refreshed by every one
//   of those repeats — so the heading ladder saw a perfectly fresh, HIGH-confidence COG the whole
//   time. It was not fresh. It was one heading, repeated. That is the failure that made Follow-Me
//   steer the wrong way on the water: COG looked alive, was dead, and when it finally dropped out
//   the ladder handed steering to a compass this hardware is known to bias by 100 deg+ under motor
//   current. Two independent guards, both consuming these constants, close that hole.
//
// WHY THEY LIVE IN THIS HEADER AND NOT IN RTMState.ino
//   Exactly the kFmEngageDistFloorM story (see the block above). getRtmHeading() in RTMState.ino
//   and its inline duplicate in Logger.ino (convertToLogData, the block marked CRITICAL
//   MAINTENANCE) must apply the SAME rule or the log will report a heading source the controller
//   did not actually use. The Arduino build concatenates .ino files alphabetically after the main
//   sketch, so Logger.ino is compiled BEFORE RTMState.ino: a constant defined in RTMState.ino is
//   invisible to the logger mirror. This header is included at the top of V2_Integration_Rx.ino,
//   so one definition here is visible to both.
//
// NONE of these four is a confStruct field. They are deliberately compile-time: no SPIFFS slot,
// no web-UI row, no SW_VERSION bump, no config wipe.
// ============================================================

// GUARD 1 — how long the COG VALUE may sit still, WHILE THE BUGGY IS MOVING, before COG stops
// counting as a trustworthy heading source.
// WHY 3000 ms. The module emits course several times a second, and real course-over-ground is
// noisy: at the default rtm_cog_min_speed_kmh of 3 km/h, GPS course scatter is whole degrees, and
// the value-change tracker in GPS.ino counts any movement above kDiagCogChangeDeg = 0.05 deg. A
// genuinely live COG therefore re-stamps g_diag_cog_change_ms many times per second, and three
// full seconds of a bit-identical course while moving is not a quiet trajectory, it is a frozen
// register. The measured fault held one value for 533 s, so 3 s catches it ~178x over while
// leaving a wide margin against a false positive. It is also short enough to matter: at the FM
// speed governor's ceiling the buggy covers only a few metres in 3 s.
static const uint32_t kRtmCogFrozenMs       = 3000;   // ms of unchanged COG value (while moving) = COG not trusted

// GUARD 2 — how far apart the two independent heading estimates may be before BOTH are distrusted.
// WHY 45 deg. Below this, disagreement is explainable by things that are not faults: compass
// mounting offset, magnetic declination, the yaw the buggy accumulates between the compass
// snapshot and the live COG sample, and ordinary COG scatter at low speed. Beyond 45 deg the two
// sensors are no longer describing the same vehicle attitude, and at 45 deg of heading error the
// steering controller is already commanding half of full authority in a direction one of the two
// sources says is wrong. It is a "these cannot both be right" threshold, not a tuning knob.
static const float    kHeadingDisagreeDeg   = 45.0f;  // degrees, shortest angular distance

// GUARD 2 — how long that disagreement must persist before it is treated as a genuine FAULT rather
// than a transient. WHY 5000 ms. A single bad compass sample, one COG glitch or one hard carve can
// open a >45 deg gap for a moment; a broken sensor holds it. At the 10 Hz ladder cadence 5 s is ~50
// consecutive confirmations, the same "a spike cannot sustain it" argument kFmSepDwellMs and
// kFmDivergeMs are built on. It is deliberately LONGER than kRtmCogFrozenMs: guard 1 has hard
// evidence (a register that stopped moving) and may act fast; guard 2 only knows that one of two
// sources is lying, so it waits for proof before ending the run.
static const uint32_t kHeadingDisagreeMs    = 5000;   // ms of sustained disagreement = FAULT

// GUARD 2 — the oldest compass snapshot that may still be compared against a live COG.
// WHY THIS IS NEEDED AND WHY IT IS 1000 ms. compass_snapshot_heading is a HELD value: Compass.ino
// only re-captures it while the motor is idle (thr_received < 25), so during an FM/RTM run, with
// the trigger held, it freezes at the instant of the squeeze and simply ages. Comparing a live COG
// against a heading captured seconds ago measures how much the buggy has TURNED since, not whether
// the sensors agree — at a modest 10 deg/s yaw a 5 s-old snapshot is 50 deg out all by itself and
// would fake a disagreement every corner. 1000 ms is the same window getRtmHeading() already
// requires before it will call a snapshot MEDIUM confidence, i.e. the only compass data this
// firmware already treats as simultaneous with now. Outside it, guard 2 simply does not run —
// no comparison is better than a comparison of two different moments in time.
// V2.5-Evo - 2026-08-16 - How long a COG stays usable after cog_valid goes false. Bridges the
// noise-driven flicker between GPS course and compass that RTM's 4.0 km/h target creates
// against a 3 km/h COG floor. 3 s is ~3.3 m of travel at that speed - short enough that the
// held course is still true, long enough to cover the dips that caused the flapping.
static const uint32_t kCogHoldMs           = 3000;   // ms; hold last-good COG across a dropout
static const uint32_t kHeadingCompareSnapMs = 1000;   // ms; max compass-snapshot age for a valid comparison

#include "../Common/ConfigServiceEngine.h"

// Web config globals
#ifdef WIFI_ENABLED
volatile bool web_cfg_service_enabled = false;
volatile bool web_cfg_pending_save = false;
volatile bool web_cfg_radio_reinit_required = false;
volatile uint32_t web_cfg_req_total = 0;
volatile uint32_t web_cfg_req_ok = 0;
volatile uint32_t web_cfg_req_err = 0;
volatile uint8_t web_cfg_debug_mode = 1; // 0=off, 1=some, 2=full
volatile uint32_t web_cfg_ap_startup_timeout_ms = 60000; // 0 disables timeout
String web_cfg_last_err = "";
#endif
volatile bool config_version_error = false;

// ============================================================
// V2.5-Evo - 2026-04-24 - TX GPS COORDINATES (received via 0xF3 meta-packet)
//
// Written by processMetaGpsPacket() in Radio.ino at 2Hz whenever TX sends
// a GPS meta-packet and RX successfully validates it.
// Read by Phase B anti-spoofing (Priority 6) to check TX-RX proximity.
//
// rx_tx_gps_timestamp == 0 means no meta-packet has ever been received.
// Use (millis() - rx_tx_gps_timestamp) > usrConf.tx_gps_stale_timeout_ms
// to detect a stale TX GPS reading before trusting lat/lng.
// ============================================================
double        rx_tx_gps_lat       = 0.0;  // TX latitude (degrees, WGS84)
double        rx_tx_gps_lng       = 0.0;  // TX longitude (degrees, WGS84)
unsigned long rx_tx_gps_timestamp = 0;    // millis() when last meta-packet received; 0 = never

// FM runtime state. Historical rtm_* names remain on shared controller/diagnostic storage so the
// packet/log/config ABI does not change; standalone RTM has no activation or PWM path.
// rtm_steer_override: shared FM bearing-derived steering value (0-255, 127=straight ahead).
// fm_mode_runtime: TX-side FM mode declaration (0-6); 0xFF = no declaration this session.
// V2.5-Evo - 2026-04-25 - P7 fix: use std::atomic for safe access across FreeRTOS task
// preemption. generatePWM (task) and RTMState.ino loop() both run on the single-core
// ESP32-C3; std::atomic gives an indivisible read/write + compiler barrier so a higher-
// priority task can't observe a torn value. (seq_cst, matching the rfInterrupt pattern.)
std::atomic<bool>    rtm_rx_active         {false};
std::atomic<bool>    rtm_rx_emergency_stop {false};
std::atomic<uint8_t> rtm_steer_override    {127};
std::atomic<uint8_t> fm_mode_runtime       {0xFF};

// V2.5-Evo - 2026-07-20 - R2: millis() when the last 0xF2 FM-mode declaration arrived from the
// TX; 0 = none has ever arrived this session. The TX refreshes its declaration every 30 s while
// armed, so RTMState.ino expires the mode after kFmModeAgeMs (95 s, ~3 missed keepalives) and
// returns FM to IDLE. Without this the RX kept a declared mode forever, which meant a lost
// disarm burst left the RX armed for the rest of the session with no way to discover it.
// Written by Radio.ino's meta-packet handler (triggeredReceive task), read by RTMState.ino's
// runFmLoop() (loop) — std::atomic for the same single-core preemption reason as the flags above.
std::atomic<unsigned long> fm_mode_last_rx_ms {0};
std::atomic<uint8_t> rtm_approach_cap      {255};  // V2.5-Evo - 2026-04-30 - approach decel cap (0-255); 255=no cap; computed by RTMState.ino during active RTM; applied by calcPWM()

// V2.5-Evo - 2026-07-19 - P3 Follow-Me (FM) autonomous-following runtime flags.
// fm_rx_active : true while FM or FM_RETURN is actively steering. Gates the override in calcPWM().
// fm_throttle_cap : FM's own subtract-only throttle cap (0-255; 255 = no cap). Applied in calcPWM()
//                as the only autonomous throttle cap. seq_cst makes it indivisible to the 100 Hz
//                generatePWM task.
std::atomic<bool>    fm_rx_active     {false};
std::atomic<uint8_t> fm_throttle_cap  {255};

#include "../Common/SPIFFSEngine.h"

// ============================================================
// V2.5-Evo - 2026-07-25 - STAGE 0 DIAGNOSTIC COUNTERS (instrumentation only)
//
// WHAT THIS IS
//   A set of plain counters that the rest of the firmware bumps at the exact point where the
//   event happens. They exist so a full on-water session log can explain itself instead of
//   needing a 60-second bench capture to reproduce a fault that only happens on the water.
//
// WHAT READS THEM — exactly two consumers, both read-only:
//   1. the level-4 log record (fillLevel4Diag() in Logger.ino), and
//   2. the ?diag / ?diagz serial commands (System.ino).
//   NOTHING in the control path reads any of these. No throttle, steering, PWM, mux schedule,
//   FM or RTM decision depends on a single value below. Deleting this whole block would change
//   how the buggy drives by precisely nothing.
//
// WHY PLAIN volatile uint32_t AND NO MUTEX
//   A 32-bit aligned load or store is a single instruction on this RISC-V core, so a reader can
//   never observe a half-written value. The increment itself is a read-modify-write, so if two
//   execution contexts bump the SAME counter and one preempts the other mid-increment, one count
//   can be lost. That is accepted deliberately: these are diagnostics, one lost count in a
//   million is invisible in a rate, and taking a mutex inside the GPS byte loop or inside
//   setUartMux() would perturb the very timing we are trying to measure.
//
// COST
//   One load, one add, one store per event. The only counter that sits in a tight loop is
//   g_diag_gps_bytes (once per GPS UART byte) and it is a bare increment — no branch, no call,
//   no formatting. There is no printf anywhere in this instrumentation except inside ?diag,
//   which is a one-shot operator command.
// ============================================================

// --- GPS feed health ---
volatile uint32_t g_diag_gps_bytes      = 0;  // bytes pulled off Serial1 by getGPSLoop() since boot / ?diagz
volatile uint32_t g_diag_gps_sentences  = 0;  // COMPLETE, checksum-valid NMEA sentences parsed (gps.encode() returned true)
volatile uint8_t  g_diag_gps_sent_per_s = 0;  // sentences parsed during the last completed 1-second window; recomputed in getGPSLoop(). Saturates at 255.

// --- GPS course-over-ground (COG) ---
// THE POINT OF THESE TWO COUNTERS: gps_last_course_ms is refreshed on EVERY valid course
// sentence, even when the module keeps repeating the same heading. The existing cog_age_ms_div10
// log column is derived from that timestamp, so it read "fresh" straight through the failure that
// actually cost control — a COG VALUE frozen on one heading while its timestamp kept ticking.
// g_diag_cog_ts_updates counts the timestamp refreshes; g_diag_cog_val_changes counts only the
// times the NUMBER genuinely moved. When the first is high and the second is ~0, the GPS is
// repeating itself and any heading derived from COG is stale even though it looks current.
volatile uint32_t g_diag_cog_ts_updates  = 0;  // times gps_last_course_ms was refreshed
volatile uint32_t g_diag_cog_val_changes = 0;  // times the COG VALUE moved by more than kDiagCogChangeDeg
volatile uint32_t g_diag_cog_change_ms   = 0;  // millis() when the COG VALUE last moved. SENTINEL: 0 = no COG value has EVER been captured this session (never a valid timestamp — the writer substitutes 1 for a literal millis()==0).
static const float kDiagCogChangeDeg     = 0.05f;  // degrees; smallest COG movement counted as a real change. Below this is GPS quantisation/noise, not motion.

// --- RX GPS fix age (sampled once per GPS poll, so it tracks staleness even when updates stop) ---
volatile uint32_t g_diag_fix_age_sum_ms = 0;  // running sum of sampled fix ages, for the mean
volatile uint32_t g_diag_fix_age_samples = 0; // number of samples in that sum
volatile uint32_t g_diag_fix_age_max_ms = 0;  // worst fix age seen; ?diag reads then resets it

// --- AW9523 UART mux (the suspected EMI failure) ---
volatile uint32_t g_diag_mux_switches = 0;    // setUartMux() calls that actually drove the select pins
volatile uint32_t g_diag_mux_errors   = 0;    // read-back mismatches inside setUartMux() (a write that did not stick)

// --- VESC polling ---
volatile uint32_t g_diag_vesc_polls = 0;      // getVescLoop() query attempts
volatile uint32_t g_diag_vesc_ok    = 0;      // of those, replies that parsed and validated

// --- loop() timing (microseconds, derived from the CPU cycle counter) ---
volatile uint32_t g_diag_loop_count      = 0;          // completed loop() bodies
volatile uint32_t g_diag_loop_us_sum     = 0;          // running sum of loop-body durations, for the mean
volatile uint32_t g_diag_loop_min_us     = 0xFFFFFFFF; // shortest loop body; 0xFFFFFFFF = no sample yet
volatile uint32_t g_diag_loop_max_us     = 0;          // longest loop body — owned by ?diag, which reads then resets it
volatile uint32_t g_diag_loop_max_us_log = 0;          // longest loop body — owned by the LOGGER, which reads then resets it once per level-4 record (kept separate so ?diag and the log never steal each other's peak)

// ============================================================
// diagCpuMhz - CPU clock in MHz, cached after the first call
//
// Inputs:  none. Outputs: MHz (160 on a stock ESP32-C3).
// Side effects: none after the first call. Used only to turn a cycle delta into microseconds.
// ============================================================
static inline uint32_t diagCpuMhz()
{
  static uint32_t mhz = 0;
  if (mhz == 0)
  {
    mhz = getCpuFrequencyMhz();
    if (mhz == 0) mhz = 160;   // defensive: never divide by zero if the call ever returns 0
  }
  return mhz;
}

// ============================================================
// diagLoopBegin / diagLoopEnd - measure one pass through the loop() body
//
// diagLoopBegin() is called on the FIRST line of loop() and returns a raw cycle stamp.
// diagLoopEnd(stamp) is called on the LAST line of the loop() body, BEFORE its own
// vTaskDelay(10) — so what is measured is the WORK the loop did, not the 10 ms it then
// deliberately sleeps. A "loop_max_ms" of 40 therefore means the loop genuinely spent 40 ms
// doing something, which is what starves GPS drains and RTM/FM ticks.
//
// esp_cpu_get_cycle_count() is a single CSR read (~5 ns). micros() is ~1 us, which is the same
// order as the shortest thing being measured, so it is deliberately NOT used here.
// The 32-bit cycle counter wraps every ~26.8 s at 160 MHz; the unsigned subtraction below is
// correct across that wrap for any interval shorter than 26.8 s, and the 3 s watchdog
// guarantees a loop body is never anywhere near that long.
//
// Side effects: updates the loop-timing counters above. Nothing else. No allocation, no I/O.
// ============================================================
static inline uint32_t diagLoopBegin()
{
  return (uint32_t)esp_cpu_get_cycle_count();
}

static inline void diagLoopEnd(uint32_t start_cycles)
{
  uint32_t us = (uint32_t)((uint32_t)esp_cpu_get_cycle_count() - start_cycles) / diagCpuMhz();
  g_diag_loop_count++;
  g_diag_loop_us_sum += us;
  if (us > g_diag_loop_max_us)     g_diag_loop_max_us     = us;
  if (us > g_diag_loop_max_us_log) g_diag_loop_max_us_log = us;
  if (us < g_diag_loop_min_us)     g_diag_loop_min_us     = us;
}

// --- Global VESC Logger Struct ---
struct vesc_struct {
  int16_t fetTemp = 0;
  int32_t motCur = 0;
  int32_t batCur = 0;
  int16_t duty = 0;
  int32_t erpm = 0;
  int16_t batVolt = 0;
  int32_t wh_raw = 0;          // session Wh×10 from VESC float32_auto; 0 = unavailable
  uint8_t fault_code = 0;
  unsigned long last_packet = 0;
};
extern vesc_struct vesc;

struct __attribute__((packed)) VescLogData {
    uint32_t timestamp;           // Local Timestamp in ms
    int16_t current_motor;       // Motor Current in 0.01A
    int16_t current_battery;     // Battery Current in 0.01A
    int8_t duty_cycle;           // Duty cycle in %
    uint16_t voltage;             // Voltage in 0.1V
    int16_t ERPM;                // ERPM / 10
    int8_t temp_mos;             // MOSFET temperature in °C
    uint8_t fault_code;           // Error code
    uint16_t speed;               // Speed in 0.1 km/h
    float latitude;               // Latitude in degrees
    float longitude;              // Longitude in degrees
    uint32_t datetime;            // UTC datetime as unix timestamp
    // V2.5-Evo - 2026-05-06 - LOG-EXT-1: heading source debug fields.
    // Populated by convertToLogData() in Logger.ino (LOG-EXT-2).
    // All ×10 fields use 0xFFFF as the "invalid/no data" sentinel.
    // rtm_heading_chosen_dx10 uses int16, -1 sentinel for "no source".
    uint8_t  thr_received_log;          // TX throttle last received (0-255)
    uint8_t  rtm_source;                // 0=NONE, 1=GPS_COG, 2=COMPASS_SNAPSHOT, 3=COMPASS_LIVE (legacy mode)
    uint8_t  rtm_confidence;            // 0=NONE, 1=LOW, 2=MEDIUM, 3=HIGH
    uint8_t  rtm_rx_active_log;         // RTM engagement state (0/1)
    uint8_t  gps_phase_b_ok_log;        // Phase B anti-spoofing handshake state (0/1)
    uint8_t  rtm_steer_override_log;    // Current steering command 0-255 (127 = straight ahead)
    int16_t  rtm_heading_chosen_dx10;   // getRtmHeading() output × 10 deg; -1 if no valid source
    uint16_t compass_live_dx10;         // Live compass heading × 10 deg (0xFFFF = invalid/uncalibrated)
    uint16_t compass_snap_dx10;         // Clean compass snapshot × 10 deg (0xFFFF = no snapshot yet)
    uint16_t snap_age_s;                // Snapshot age in seconds (0xFFFF = no snapshot yet)
    uint16_t gps_course_dx10;           // GPS course-over-ground × 10 deg (0xFFFF = no fix or invalid)
    uint16_t cog_age_ms_div10;          // GPS course age in 10ms units (0xFFFF = no fix yet)
    // V2.5-Evo - 2026-05-08 - Bundle 1: heading controller tuning telemetry fields.
    // 0x7FFF (32767) is the "no data" sentinel — NOT 0x0000.
    int16_t heading_error_dx10;   // Heading error in 0.1° units (signed; -1800..+1800).
                                  // 0x7FFF = no valid heading source. Positive = need turn right.
    int16_t d_error_dx10;         // Rate-of-change of heading error in 0.1°/s units.
                                  // 0x7FFF = no prior sample (first cycle). For tuning Kd.
    // V2.5-Evo - 2026-05-11 - E7 Fix: BREmote remote_error code for cross-correlation with VESC/motor data.
    // 0 = no error, 71 = E71 water ingress (see checkWetness() in System.ino).
    uint8_t error_code_log;       // telemetry.error_code at log time. 0 = no BREmote error.
    // V2.5-Evo - 2026-07-19 - FM triage: steering byte actually applied to the motor mix by
    // calcPWM() (g_effective_steer), NOT just the commanded rtm_steer_override. Reveals the
    // actuation gap — a valid heading error can log a non-127 rtm_steer_override_log while this
    // stays 127 because the throttle-release gate suppressed it. 127 = straight ahead.
    uint8_t effective_steer_log;
    // V2.5-Evo - 2026-07-24 - F9: owner-requested range telemetry — RX→TX distance + LoRa link quality.
    // Appended at the tail so existing CSV column order is preserved (old parsers ignore trailing columns).
    uint16_t tx_distance_dx10;    // RX→TX distance × 10 m (0.1 m resolution, capped ~164 m); 0xFFFF = N/A (no valid GPS pair)
    int16_t  rssi_dbm;            // last control-packet RSSI in dBm (rounded); 0x7FFF = N/A (failsafe — no recent packet)
    int16_t  snr_dx10;            // last control-packet SNR × 10 dB; 0x7FFF = N/A (failsafe — no recent packet)
};
static_assert(sizeof(VescLogData) == 59, "VescLogData size mismatch — check binary log compat.");  // 29 base; +18 LOG-EXT-1 (2026-05-06); +4 Bundle 1 tuning fields (2026-05-08); +1 error_code_log E7 fix (2026-05-11); +1 effective_steer_log FM triage (2026-07-19); +6 F9 distance+RSSI+SNR (2026-07-24)

// ============================================================
// V2.5-Evo - 2026-07-25 - STAGE 0 PART C: LEVEL-4 ("Deep") LOG RECORD
//
// Tiers are ADDITIVE: level N is level N-1 plus a block. VescLogDataL4 starts with a complete,
// byte-identical VescLogData, so the first 59 bytes of a level-4 record decode with exactly the
// same code that decodes a level-3 record. That is what lets one CSV formatter serve both.
//
// The first four fields answer the four open theories about the on-water failure:
//   gps_sent_per_s — is the GPS still delivering sentences at all, or has the feed died?
//   cog_frozen_s   — how long has the COG VALUE been stuck? (The existing cog_age_ms_div10
//                    column tracks the TIMESTAMP, which kept refreshing while the value was
//                    frozen, so it was blind to exactly this failure.)
//   mux_err_cnt    — how many AW9523 mux writes failed read-back? (motor EMI corrupting I2C)
//   loop_max_ms    — did the main loop stall long enough to starve the GPS drain / RTM tick?
// ============================================================
struct __attribute__((packed)) VescLogDataL4 {
    VescLogData base;              // the complete level-3 record, unchanged and first — do not reorder
    uint8_t  gps_sent_per_s;       // complete NMEA sentences parsed in the last full second (saturates at 255)
    uint8_t  cog_frozen_s;         // seconds since the COG VALUE last changed. SENTINEL 255 = no COG value has ever been seen this session. 254 = 254 s or longer.
    uint16_t mux_err_cnt;          // running session total of setUartMux() I2C read-back mismatches (saturates at 0xFFFE)
    uint16_t loop_max_ms;          // worst loop() body duration since the PREVIOUS record, in ms, rounded to nearest; reset to 0 after every record. 0 = no loop completed since the last record, or every loop was under 0.5 ms.

    // Follow-Me engage audit block (2026-08-27). The first 65 bytes above are deliberately
    // byte-identical to the original level-4 record. Readers use the record_size stored in each
    // file header, so old 65-byte level-4 files remain readable while new files append this block.
    uint32_t fm_gate_flags;        // FM_LOG_GATE_* bits below; expanded into named CSV columns
    uint16_t fm_distance_dx10;     // current RX-to-TX radial distance x10 m; 0xFFFF = untrusted/N/A
    uint16_t fm_d_engage_dx10;     // effective D_engage x10 m (configured/auto value after 8 m floor)
    uint16_t fm_rider_speed_dx10;  // filtered rider speed x10 km/h; 0xFFFF = unavailable
    uint16_t fm_sep_dwell_ms;      // current radial separation proof progress; 0..2000 ms
    int16_t  fm_front_angle_dx10;  // F4-F6 selected-axis error x10 deg; 0x7FFF = not front/not measurable
    uint8_t  fm_mode;              // live RX declaration: 1..4; 0/0xFF = disabled/not declared
    uint8_t  fm_state;             // 0=IDLE, 1=ARMED, 2=ACTIVE, 4=STOPPING, 5=RETURN
    uint8_t  fm_block_reason;      // FM_LOG_BLOCK_*; a STOPPING row retains its initiating fault
    uint8_t  fm_throttle_cap;      // actual FM cap published to PWM, 0..255

    // Heading evidence audit block (2026-08-27). Appended after the original 83-byte FM audit
    // record so both 65-byte legacy Deep logs and 83-byte FM-audit logs retain their exact layout.
    uint16_t heading_diag_flags;         // HEADING_LOG_* raw conditions below
    int16_t  compass_cog_diff_dx10;      // signed shortest COG-compass gap x10 deg; 0x7FFF = not comparable
    uint16_t heading_disagree_dwell_ms;  // current set-proof progress, capped at kHeadingDisagreeMs
    uint16_t heading_agree_dwell_ms;     // current clear-proof progress, capped at kHeadingDisagreeMs
    uint16_t cog_last_good_age_ms;       // age of controller's last accepted live COG; 0xFFFF = none
    uint16_t compass_snap_age_ms;        // clean compass-snapshot age; 0xFFFF = none
    uint8_t  heading_mode;               // configured rtm_use_compass: 0=COG, 1=hybrid, 2=compass
};

// The original level-4 record ended after loop_max_ms. Keep the exact size as an explicit
// compatibility boundary: logCsvHeaderForRecord() and logFormatCsvRow() use it to decode old files.
#define LOG_RECORD_SIZE_L4_LEGACY 65u
#define LOG_RECORD_SIZE_L4_FM_AUDIT 83u
static_assert(offsetof(VescLogDataL4, fm_gate_flags) == LOG_RECORD_SIZE_L4_LEGACY,
              "The FM audit block must remain appended after the legacy 65-byte level-4 record.");
static_assert(offsetof(VescLogDataL4, heading_diag_flags) == LOG_RECORD_SIZE_L4_FM_AUDIT,
              "The heading audit block must remain appended after the 83-byte FM audit record.");
static_assert(sizeof(VescLogDataL4) == 96,
              "VescLogDataL4 size mismatch — expected 65 + 18-byte FM + 13-byte heading blocks.");

// fm_gate_flags bit map. These are snapshots of the exact decisions made by runFmLoop(), not a
// second implementation in the logger. A zero gate can mean either "failed" or "not evaluated"
// after an earlier short-circuit; fm_block_reason names the first effective blocker.
#define FM_LOG_GATE_MODE_OK             (1UL << 0)
#define FM_LOG_GATE_TRIGGER_HELD        (1UL << 1)
#define FM_LOG_GATE_GPS_NOT_REJECTED    (1UL << 2)
#define FM_LOG_GATE_PHASE_B_OK          (1UL << 3)
#define FM_LOG_GATE_TX_GPS_FRESH        (1UL << 4)
#define FM_LOG_GATE_RX_GPS_FRESH        (1UL << 5)
#define FM_LOG_GATE_HEADING_OK          (1UL << 6)
#define FM_LOG_GATE_LINK_OK             (1UL << 7)
#define FM_LOG_GATE_POSITION_OK         (1UL << 8)
#define FM_LOG_GATE_DIST_OVER_ENGAGE    (1UL << 9)
#define FM_LOG_GATE_SEP_LATCHED         (1UL << 10)
#define FM_LOG_GATE_RETURN_CANDIDATE    (1UL << 11)
#define FM_LOG_GATE_MIN_DIST_STOP       (1UL << 12)
#define FM_LOG_GATE_HEADING_DISAGREE    (1UL << 13)
#define FM_LOG_GATE_GEOMETRY_WARNING    (1UL << 14)
#define FM_LOG_GATE_FRONT_WARNING       (1UL << 15)
#define FM_LOG_GATE_CAN_BE_ACTIVE       (1UL << 16)
#define FM_LOG_GATE_RETURN_EXIT_HOLD    (1UL << 17)
#define FM_LOG_GATE_MANUAL_STEER        (1UL << 18)
#define FM_LOG_GATE_DIVERGENCE_FAULT    (1UL << 19)

// heading_diag_flags: raw inputs and intermediate verdicts behind heading condition 6. Unlike
// fm_heading_ok these bits do not short-circuit after an earlier FM gate, so every Deep row shows
// why the heading ladder did or did not have a GPS-derived source.
#define HEADING_LOG_COG_CAPTURED         (1u << 0)
#define HEADING_LOG_COG_TIMESTAMP_FRESH (1u << 1)
#define HEADING_LOG_COG_SPEED_OK         (1u << 2)
#define HEADING_LOG_COG_FROZEN_MOVING    (1u << 3)
#define HEADING_LOG_COG_LIVE_VALID       (1u << 4)
#define HEADING_LOG_COG_HOLD_VALID       (1u << 5)
#define HEADING_LOG_COMPASS_SNAP_FRESH   (1u << 6)
#define HEADING_LOG_COMPASS_SNAP_USABLE  (1u << 7)
#define HEADING_LOG_COMPARE_POSSIBLE     (1u << 8)
#define HEADING_LOG_DISAGREE_NOW         (1u << 9)
#define HEADING_LOG_DISAGREE_LATCHED     (1u << 10)
#define HEADING_LOG_GPS_FIX_FRESH        (1u << 11)

enum FmLogBlockReason : uint8_t {
  FM_LOG_BLOCK_NONE = 0,
  FM_LOG_BLOCK_NO_DECLARATION,
  FM_LOG_BLOCK_CONFIG_DISABLED,
  FM_LOG_BLOCK_MODE_EXPIRED,
  FM_LOG_BLOCK_STOPPING,
  FM_LOG_BLOCK_RETURN_EXIT_HOLD,
  FM_LOG_BLOCK_GPS_REJECTED,
  FM_LOG_BLOCK_PHASE_B,
  FM_LOG_BLOCK_TX_GPS_STALE,
  FM_LOG_BLOCK_RX_GPS_STALE,
  FM_LOG_BLOCK_NO_HEADING,
  FM_LOG_BLOCK_LINK,
  FM_LOG_BLOCK_POSITION,
  FM_LOG_BLOCK_TRIGGER,
  FM_LOG_BLOCK_BELOW_D_ENGAGE,
  FM_LOG_BLOCK_SEPARATION_DWELL,
  FM_LOG_BLOCK_RETURN_CANDIDATE,
  FM_LOG_BLOCK_RETURN_ACTIVE,
  FM_LOG_BLOCK_MIN_DIST_STOP,
  FM_LOG_BLOCK_HEADING_DISAGREE,  // legacy CSV decoder only; current FM reports NO_HEADING + warning bit
  FM_LOG_BLOCK_DIVERGENCE,
  FM_LOG_BLOCK_UNKNOWN,
  // Appended values preserve the numeric meaning of every reason already stored in Deep logs.
  FM_LOG_BLOCK_RETURN_RUNTIME,
  FM_LOG_BLOCK_RETURN_NOT_CLOSING
};

// Runtime copy passed from the loop-task FM controller to the logger task. This is deliberately
// separate from the packed on-flash struct; RTMState.ino publishes it under a tiny critical section.
struct FmLogDiagSnapshot {
  uint32_t gate_flags;
  uint16_t distance_dx10;
  uint16_t d_engage_dx10;
  uint16_t rider_speed_dx10;
  uint16_t sep_dwell_ms;
  int16_t  front_angle_dx10;
  uint8_t  mode;
  uint8_t  state;
  uint8_t  block_reason;
  uint8_t  throttle_cap;
  uint16_t heading_diag_flags;
  int16_t  compass_cog_diff_dx10;
  uint16_t heading_disagree_dwell_ms;
  uint16_t heading_agree_dwell_ms;
  uint16_t cog_last_good_age_ms;
  uint16_t compass_snap_age_ms;
  uint8_t  heading_mode;
};

// ============================================================
// V2.5-Evo - 2026-07-25 - STAGE 0 PART B: SELF-DESCRIBING LOG FILE HEADER
//
// WHY THIS EXISTS: different log levels write different record sizes, so a log file that does
// not say what it is cannot be parsed — a reader that assumes sizeof(VescLogData) would walk
// straight off the record boundary and emit convincing garbage. Every log file now opens with
// this 8-byte header, and BOTH readers (the serial ?download path in Logger.ino and the WiFi
// /api/logs/download path in Common/WebConfigEngine.h) read it and parse accordingly.
//
// 8 bytes exactly, naturally aligned, no padding: uint32 + uint8 + uint8 + uint16. The first
// record therefore starts at file offset 8, which is still 4-byte aligned.
//
// OLD LOGS: files written before this change have no header. Their first 4 bytes are a millis()
// timestamp, which will not equal the magic, so both readers detect the missing magic and say so
// in plain language instead of emitting garbage. Those files were already undecodable after the
// 53 -> 59 byte record change (F9, 2026-07-24) — this just makes the failure honest.
// ============================================================
#define LOG_FILE_MAGIC       0x474C5242UL  // little-endian bytes on disk read "BRLG" (BREmote Log)
#define LOG_FILE_FORMAT_VER  1             // bump ONLY if the header layout itself changes

struct __attribute__((packed)) LogFileHeader {
    uint32_t magic;        // LOG_FILE_MAGIC — absent/mismatched means "not a BREmote log of this era"
    uint8_t  format_ver;   // LOG_FILE_FORMAT_VER — layout of THIS header
    uint8_t  log_level;    // the level the file was actually recorded at (3 or 4 today)
    uint16_t record_size;  // bytes per record in this file — the ONLY thing a reader may step by
};
static_assert(sizeof(LogFileHeader) == 8, "LogFileHeader must stay 8 bytes — readers step past it by sizeof().");

// ============================================================
// logResolveLevel - turn the stored config value into the level actually used
//
// Inputs:  usrConf.log_level. Outputs: 3 or 4. Side effects: none.
//
// 0 (unset), 1 (Basic), 2 (VESC), 3 (Developer) and ANY out-of-range value all resolve to 3.
// Levels 1 and 2 are reserved for a future storage optimisation (smaller records); they are
// accepted by the config validator so a rider can select them and a later firmware will honour
// them, but until those records exist they are documented — here, in the field comment, and in
// all three config UIs — as logging at level 3 rather than being silently dropped.
// ============================================================
static inline uint8_t logResolveLevel()
{
  return (usrConf.log_level == 4) ? 4 : 3;
}

// ============================================================
// logRecordSizeForLevel - bytes per record for a given level
// Inputs: level (3 or 4). Outputs: record size in bytes. Side effects: none.
// ============================================================
static inline uint16_t logRecordSizeForLevel(uint8_t level)
{
  return (level >= 4) ? (uint16_t)sizeof(VescLogDataL4) : (uint16_t)sizeof(VescLogData);
}

// ============================================================
// V2.5-Evo - 2026-07-25 - STAGE 0 PART B: ONE CSV DEFINITION, TWO READERS
//
// The serial ?download path and the WiFi /api/logs/download path must emit the same columns,
// the same values, the same scaling and the same N/A sentinels. That parity was only just
// repaired (F-WEBCSV, 2026-07-25) after the WiFi path silently dropped 5 columns for months.
// Rather than keep two hand-synchronised copies and a comment asking future editors to be
// careful, the column list, the row format and the row FORMATTER now exist exactly once and
// both readers call them. Divergence is now structurally impossible, not merely discouraged.
//
// If you add a column: extend LOG_CSV_HEADER_L3 (or _L4), extend the matching format string,
// and add the argument in logFormatCsvRow(). Both readers pick it up with no further edits.
// ============================================================
#define LOG_CSV_HEADER_L3 "timestamp_ms,motor_current_A,battery_current_A,duty_cycle_%,voltage_V,ERPM,temp_mos_C,fault_code,speed_kmh,latitude,longitude,datetime_unix,thr_received,rtm_source,rtm_confidence,rtm_rx_active,gps_phase_b_ok,rtm_steer_override,rtm_heading_chosen_dx10,compass_live_dx10,compass_snap_dx10,snap_age_s,gps_course_dx10,cog_age_ms_div10,heading_error_dx10,d_error_dx10,remote_error,effective_steer,tx_distance_m,rssi_dbm,snr_db"
#define LOG_CSV_HEADER_L4_LEGACY LOG_CSV_HEADER_L3 ",gps_sent_per_s,cog_frozen_s,mux_err_cnt,loop_max_ms"
#define LOG_CSV_HEADER_L4_FM_AUDIT LOG_CSV_HEADER_L4_LEGACY ",fm_mode,fm_state,fm_block_reason,fm_throttle_cap,fm_distance_m,fm_d_engage_m,fm_rider_speed_kmh,fm_sep_dwell_ms,fm_front_angle_deg,fm_mode_ok,fm_trigger_held,fm_gps_not_rejected,fm_phase_b_ok,fm_tx_gps_fresh,fm_rx_gps_fresh,fm_heading_ok,fm_link_ok,fm_position_ok,fm_dist_over_d_engage,fm_sep_latched,fm_return_candidate,fm_min_dist_stop,fm_heading_disagree,fm_geometry_warning,fm_front_warning,fm_can_be_active,fm_return_exit_hold,fm_manual_steer,fm_divergence_fault"
#define LOG_CSV_HEADER_L4 LOG_CSV_HEADER_L4_FM_AUDIT ",heading_mode,compass_cog_diff_deg,heading_disagree_dwell_ms,heading_agree_dwell_ms,cog_last_good_age_ms,compass_snap_age_ms,cog_captured,cog_timestamp_fresh,cog_speed_ok,cog_frozen_moving,cog_live_valid,cog_hold_valid,compass_snap_fresh,compass_snap_usable,heading_compare_possible,heading_disagree_now,heading_disagree_latched,heading_gps_fix_fresh"

#define LOG_CSV_ROW_FMT_L3 "%u,%.2f,%.2f,%d,%.1f,%d,%u,%u,%.1f,%.6f,%.6f,%u,%u,%u,%u,%u,%u,%u,%d,%u,%u,%u,%u,%u,%d,%d,%u,%u,%.1f,%d,%.1f"
#define LOG_CSV_ROW_EXT_L4 ",%u,%u,%u,%u"
#define LOG_CSV_ROW_EXT_FM ",%u,%s,%s,%u,%.1f,%.1f,%.1f,%u,%.1f,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u"
#define LOG_CSV_ROW_EXT_HEADING ",%u,%.1f,%u,%u,%d,%d,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u"

// Row buffer size. The 31 level-3 columns need ~210 bytes normally and ~282 in the pathological
// corrupt-coordinate case. The Deep fields plus FM and heading audit fields remain comfortably
// below 768 bytes even with the longest state/block labels. It is a stack local in the Arduino loop
// task (8 KB stack), which is where both readers run.
#define LOG_CSV_ROW_BUF 768

static inline const char* fmLogStateText(uint8_t state)
{
  switch (state) {
    case 0: return "IDLE";
    case 1: return "ARMED";
    case 2: return "ACTIVE";
    case 4: return "STOPPING";
    case 5: return "RETURN";
    default: return "UNKNOWN";
  }
}

static inline const char* fmLogBlockReasonText(uint8_t reason)
{
  switch ((FmLogBlockReason)reason) {
    case FM_LOG_BLOCK_NONE:              return "none";
    case FM_LOG_BLOCK_NO_DECLARATION:    return "no_declaration";
    case FM_LOG_BLOCK_CONFIG_DISABLED:   return "config_disabled";
    case FM_LOG_BLOCK_MODE_EXPIRED:      return "mode_expired";
    case FM_LOG_BLOCK_STOPPING:          return "stopping";
    case FM_LOG_BLOCK_RETURN_EXIT_HOLD:  return "return_exit_hold";
    case FM_LOG_BLOCK_GPS_REJECTED:      return "gps_rejected";
    case FM_LOG_BLOCK_PHASE_B:           return "phase_b";
    case FM_LOG_BLOCK_TX_GPS_STALE:      return "tx_gps_stale";
    case FM_LOG_BLOCK_RX_GPS_STALE:      return "rx_gps_stale";
    case FM_LOG_BLOCK_NO_HEADING:        return "no_heading";
    case FM_LOG_BLOCK_LINK:              return "link";
    case FM_LOG_BLOCK_POSITION:          return "position";
    case FM_LOG_BLOCK_TRIGGER:           return "trigger";
    case FM_LOG_BLOCK_BELOW_D_ENGAGE:    return "below_d_engage";
    case FM_LOG_BLOCK_SEPARATION_DWELL:  return "separation_dwell";
    case FM_LOG_BLOCK_RETURN_CANDIDATE:  return "return_candidate";
    case FM_LOG_BLOCK_RETURN_ACTIVE:     return "return_active";
    case FM_LOG_BLOCK_MIN_DIST_STOP:     return "min_dist_stop";
    case FM_LOG_BLOCK_HEADING_DISAGREE:  return "heading_disagree";
    case FM_LOG_BLOCK_DIVERGENCE:        return "divergence";
    case FM_LOG_BLOCK_RETURN_RUNTIME:    return "return_runtime";
    case FM_LOG_BLOCK_RETURN_NOT_CLOSING: return "return_not_closing";
    default:                             return "unknown";
  }
}

static inline const char* logCsvHeaderForRecord(uint8_t level, uint16_t record_size)
{
  if (level < 4 || record_size < (uint16_t)LOG_RECORD_SIZE_L4_LEGACY) {
    return LOG_CSV_HEADER_L3;
  }
  if (record_size >= (uint16_t)sizeof(VescLogDataL4)) return LOG_CSV_HEADER_L4;
  if (record_size >= (uint16_t)LOG_RECORD_SIZE_L4_FM_AUDIT) return LOG_CSV_HEADER_L4_FM_AUDIT;
  return LOG_CSV_HEADER_L4_LEGACY;
}

// ============================================================
// logFormatCsvRow - format ONE binary log record as one CSV line
//
// What it does:
//   Decodes rec_bytes (raw bytes straight off SPIFFS) into the level-3 fields, formats them,
//   and — when the file is level 4 and the record is big enough to contain it — appends the
//   level-4 diagnostics present in that record version. Always terminates the line with a single
//   '\n' and a NUL.
//
// Inputs:
//   out       - destination buffer (use LOG_CSV_ROW_BUF bytes)
//   out_len   - size of that buffer
//   rec_bytes - one raw record as read from the file, at least sizeof(VescLogData) bytes
//   rec_size  - bytes actually read for this record, taken from the FILE HEADER, never sizeof()
//   level     - log level from the file header (3 or 4)
//
// Outputs: number of characters written (excluding the NUL); 0 on a bad argument.
// Side effects: none — reads nothing global, writes only into out.
//
// The record is copied with memcpy rather than cast in place: the caller's buffer is a byte
// array with no guaranteed alignment, and these structs are packed.
// ============================================================
static int logFormatCsvRow(char* out, size_t out_len, const uint8_t* rec_bytes, uint16_t rec_size, uint8_t level)
{
  if (out == NULL || out_len < 2) return 0;
  if (rec_bytes == NULL || rec_size < (uint16_t)sizeof(VescLogData)) { out[0] = '\0'; return 0; }

  VescLogData d;
  memcpy(&d, rec_bytes, sizeof(VescLogData));

  int n = snprintf(out, out_len, LOG_CSV_ROW_FMT_L3,
                   d.timestamp,
                   d.current_motor / 100.0f,
                   d.current_battery / 100.0f,
                   (int16_t)d.duty_cycle,
                   d.voltage / 10.0f,
                   (int32_t)d.ERPM * 10,
                   d.temp_mos,
                   d.fault_code,
                   d.speed / 10.0f,
                   d.latitude,
                   d.longitude,
                   d.datetime,
                   (unsigned)d.thr_received_log,
                   (unsigned)d.rtm_source,
                   (unsigned)d.rtm_confidence,
                   (unsigned)d.rtm_rx_active_log,
                   (unsigned)d.gps_phase_b_ok_log,
                   (unsigned)d.rtm_steer_override_log,
                   (int)d.rtm_heading_chosen_dx10,
                   (unsigned)d.compass_live_dx10,
                   (unsigned)d.compass_snap_dx10,
                   (unsigned)d.snap_age_s,
                   (unsigned)d.gps_course_dx10,
                   (unsigned)d.cog_age_ms_div10,
                   // Bundle 1: heading controller tuning columns (0x7FFF = no data sentinel)
                   (int)d.heading_error_dx10,
                   (int)d.d_error_dx10,
                   // E7 Fix: BREmote remote_error code (0 = none, 71 = E71 water ingress)
                   (unsigned)d.error_code_log,
                   // FM triage: steering byte actually applied by calcPWM() (vs commanded rtm_steer_override)
                   (unsigned)d.effective_steer_log,
                   // F9: distance (m) + link quality. N/A → distance -1.0, rssi -999, snr -99.0
                   (d.tx_distance_dx10 == 0xFFFF) ? -1.0f : (d.tx_distance_dx10 / 10.0f),
                   (d.rssi_dbm == 0x7FFF) ? -999 : (int)d.rssi_dbm,
                   (d.snr_dx10 == 0x7FFF) ? -99.0f : (d.snr_dx10 / 10.0f));

  if (n < 0) { out[0] = '\0'; return 0; }
  if ((size_t)n >= out_len) n = (int)out_len - 1;   // snprintf truncated — keep the index inside the buffer

  // Level-4 blocks. The original four fields occupy bytes 59..64 and are decoded from every
  // legacy-or-new L4 record. The FM extension begins at byte 65 and is appended only when the file
  // actually contains it. This preserves exact header/row parity for old 65-byte files.
  if (level >= 4 && rec_size >= (uint16_t)LOG_RECORD_SIZE_L4_LEGACY && (size_t)n < (out_len - 1))
  {
    VescLogDataL4 d4 = {};
    uint16_t copy_size = (rec_size < (uint16_t)sizeof(VescLogDataL4))
        ? rec_size : (uint16_t)sizeof(VescLogDataL4);
    memcpy(&d4, rec_bytes, copy_size);
    int m = snprintf(out + n, out_len - (size_t)n, LOG_CSV_ROW_EXT_L4,
                     (unsigned)d4.gps_sent_per_s,
                     (unsigned)d4.cog_frozen_s,
                     (unsigned)d4.mux_err_cnt,
                     (unsigned)d4.loop_max_ms);
    if (m > 0)
    {
      n += m;
      if ((size_t)n >= out_len) n = (int)out_len - 1;
    }

    if (rec_size >= (uint16_t)LOG_RECORD_SIZE_L4_FM_AUDIT && (size_t)n < (out_len - 1))
    {
      uint32_t g = d4.fm_gate_flags;
      int m2 = snprintf(out + n, out_len - (size_t)n, LOG_CSV_ROW_EXT_FM,
                        (unsigned)d4.fm_mode,
                        fmLogStateText(d4.fm_state),
                        fmLogBlockReasonText(d4.fm_block_reason),
                        (unsigned)d4.fm_throttle_cap,
                        (d4.fm_distance_dx10 == 0xFFFF) ? -1.0f : (d4.fm_distance_dx10 / 10.0f),
                        (d4.fm_d_engage_dx10 == 0xFFFF) ? -1.0f : (d4.fm_d_engage_dx10 / 10.0f),
                        (d4.fm_rider_speed_dx10 == 0xFFFF) ? -1.0f : (d4.fm_rider_speed_dx10 / 10.0f),
                        (unsigned)d4.fm_sep_dwell_ms,
                        (d4.fm_front_angle_dx10 == 0x7FFF) ? -1.0f : (d4.fm_front_angle_dx10 / 10.0f),
                        (unsigned)((g & FM_LOG_GATE_MODE_OK) != 0),
                        (unsigned)((g & FM_LOG_GATE_TRIGGER_HELD) != 0),
                        (unsigned)((g & FM_LOG_GATE_GPS_NOT_REJECTED) != 0),
                        (unsigned)((g & FM_LOG_GATE_PHASE_B_OK) != 0),
                        (unsigned)((g & FM_LOG_GATE_TX_GPS_FRESH) != 0),
                        (unsigned)((g & FM_LOG_GATE_RX_GPS_FRESH) != 0),
                        (unsigned)((g & FM_LOG_GATE_HEADING_OK) != 0),
                        (unsigned)((g & FM_LOG_GATE_LINK_OK) != 0),
                        (unsigned)((g & FM_LOG_GATE_POSITION_OK) != 0),
                        (unsigned)((g & FM_LOG_GATE_DIST_OVER_ENGAGE) != 0),
                        (unsigned)((g & FM_LOG_GATE_SEP_LATCHED) != 0),
                        (unsigned)((g & FM_LOG_GATE_RETURN_CANDIDATE) != 0),
                        (unsigned)((g & FM_LOG_GATE_MIN_DIST_STOP) != 0),
                        (unsigned)((g & FM_LOG_GATE_HEADING_DISAGREE) != 0),
                        (unsigned)((g & FM_LOG_GATE_GEOMETRY_WARNING) != 0),
                        (unsigned)((g & FM_LOG_GATE_FRONT_WARNING) != 0),
                        (unsigned)((g & FM_LOG_GATE_CAN_BE_ACTIVE) != 0),
                        (unsigned)((g & FM_LOG_GATE_RETURN_EXIT_HOLD) != 0),
                        (unsigned)((g & FM_LOG_GATE_MANUAL_STEER) != 0),
                        (unsigned)((g & FM_LOG_GATE_DIVERGENCE_FAULT) != 0));
      if (m2 > 0)
      {
        n += m2;
        if ((size_t)n >= out_len) n = (int)out_len - 1;
      }
    }

    if (rec_size >= (uint16_t)sizeof(VescLogDataL4) && (size_t)n < (out_len - 1))
    {
      uint16_t h = d4.heading_diag_flags;
      int m3 = snprintf(out + n, out_len - (size_t)n, LOG_CSV_ROW_EXT_HEADING,
                        (unsigned)d4.heading_mode,
                        (d4.compass_cog_diff_dx10 == 0x7FFF)
                            ? -999.0f : (d4.compass_cog_diff_dx10 / 10.0f),
                        (unsigned)d4.heading_disagree_dwell_ms,
                        (unsigned)d4.heading_agree_dwell_ms,
                        (d4.cog_last_good_age_ms == 0xFFFF)
                            ? -1 : (int)d4.cog_last_good_age_ms,
                        (d4.compass_snap_age_ms == 0xFFFF)
                            ? -1 : (int)d4.compass_snap_age_ms,
                        (unsigned)((h & HEADING_LOG_COG_CAPTURED) != 0),
                        (unsigned)((h & HEADING_LOG_COG_TIMESTAMP_FRESH) != 0),
                        (unsigned)((h & HEADING_LOG_COG_SPEED_OK) != 0),
                        (unsigned)((h & HEADING_LOG_COG_FROZEN_MOVING) != 0),
                        (unsigned)((h & HEADING_LOG_COG_LIVE_VALID) != 0),
                        (unsigned)((h & HEADING_LOG_COG_HOLD_VALID) != 0),
                        (unsigned)((h & HEADING_LOG_COMPASS_SNAP_FRESH) != 0),
                        (unsigned)((h & HEADING_LOG_COMPASS_SNAP_USABLE) != 0),
                        (unsigned)((h & HEADING_LOG_COMPARE_POSSIBLE) != 0),
                        (unsigned)((h & HEADING_LOG_DISAGREE_NOW) != 0),
                        (unsigned)((h & HEADING_LOG_DISAGREE_LATCHED) != 0),
                        (unsigned)((h & HEADING_LOG_GPS_FIX_FRESH) != 0));
      if (m3 > 0)
      {
        n += m3;
        if ((size_t)n >= out_len) n = (int)out_len - 1;
      }
    }
  }

  if ((size_t)n < (out_len - 1)) out[n++] = '\n';
  out[n] = '\0';
  return n;
}

#define ENABLE_WEB_LOG_DOWNLOAD // Enable log download endpoints

#ifdef WIFI_ENABLED
#include "../Common/WebConfigEngine.h"
#endif

#ifdef WIFI_ENABLED
void webCfgNotifyRxConnected();
#else
inline void webCfgNotifyRxConnected() {}  // No-op stub when WiFi disabled
#endif

// V2.5-Evo - 2026-05-16 - feat(telemetry): expand LoRa packet 8→19 bytes + 0xF4 aux meta-packet
//Telemetry to send, MUST BE 8-bit!!
// V2.5-Evo - 2026-04-27 - P8: rtm_distance at index 5; encoding: 0-99=tenths of m, 100-254=(value-90) whole m, 255=N/A.
#define FM_FLAG_ARMED        0x01
#define FM_FLAG_ENGAGED      0x02
#define FM_FLAG_NOTREADY     0x04
#define FM_FLAG_FAULT        0x08
#define FM_FLAG_RETURN       0x10
#define FM_FLAG_DONE         0x20  // reserved: legacy RETURN->IDLE completion bit; current RX does not emit
#define FM_FLAG_GEOMETRY     0x40  // radial/separation geometry warning only; no control effect
#define FM_FLAG_FRONT_LOST   0x80  // F4-F6 front-position warning only; no control effect
struct __attribute__((packed)) TelemetryPacket {
    uint8_t foil_bat = 0xFF;          // index 0 — battery % 0-100
    uint8_t foil_temp = 0xFF;         // index 1 — FET temp degC
    uint8_t foil_speed = 0xFF;        // index 2 — speed km/h
    uint8_t error_code = 0;           // index 3 — fault flags
    uint8_t foil_power = 0xFF;        // index 4 — power (watts/50); 0xFF = N/A
    uint8_t rtm_distance = 0xFF;      // index 5 — RX→TX distance; see encoding above; 0xFF = N/A
    uint8_t foil_motor_amps = 0xFF;   // index 6 — motor current whole amps; 0xFF = N/A
    uint8_t foil_voltage = 0xFF;      // index 7 — battery voltage V×2 (0.5V res); 0xFF = N/A
    uint8_t foil_duty = 0xFF;         // index 8 — duty cycle 0-100%; 0xFF = N/A
    uint8_t foil_erpm_lo = 0xFF;      // index 9 — |ERPM|÷100 low byte; 0xFFFF when both=0xFF means N/A
    uint8_t foil_erpm_hi = 0xFF;      // index 10 — |ERPM|÷100 high byte
    uint8_t foil_wh_lo = 0xFF;        // index 11 — session Wh×10 low byte; 0xFFFF when both=0xFF means N/A
    uint8_t foil_wh_hi = 0xFF;        // index 12 — session Wh×10 high byte
    uint8_t rx_heading = 0xFF;        // index 13 — GPS COG÷2 (0-179→0-358°); 0xFF = N/A
    uint8_t fm_heading_err = 127;     // index 14 — bearing error+127; 127 = no data
    uint8_t fm_status = 0;            // index 15 — [7]=aux2_on [6]=aux1_on [5]=vesc_online [4]=rx_wetness [3:2]=heading_conf [1]=rtm_active [0]=fm_active
    uint8_t fm_flags = 0;             // index 16 — FM state: [7]=F4-F6 front warning [6]=geometry warning
                                      // [5]=legacy done/reserved [4]=return [3]=fault [2]=not-ready [1]=driving [0]=armed
    uint8_t rx_bearing_to_tx = 0xFF;  // index 17 — bearing from buggy toward rider÷2; 0xFF = N/A
    uint8_t link_quality = 0;         // index 18 (must be last)
} telemetry;

/*
** FreeROTS/Task handles
*/
const int maxTasks = 10;
TaskStatus_t taskStats[maxTasks];

// Task handles
TaskHandle_t generatePWMHandle = NULL;
TaskHandle_t triggeredReceiveHandle = NULL;
TaskHandle_t checkConnStatusHandle = NULL;
extern TaskHandle_t loopTaskHandle;

// Semaphore for triggered task
SemaphoreHandle_t triggerReceiveSemaphore;

// Mutex protecting Wire/AW9523 — accessed by multiple FreeRTOS tasks that preempt each
// other on the single ESP32-C3 core: generatePWM, checkConnStatus, checkWetness,
// checkButtons, setUartMux, blinkErr, blinkBind, readCompassRaw, loggerLoop, triggerBlink.
// Created in initHardware() before Wire.begin() so startupAW() can safely use it.
SemaphoreHandle_t i2cMutex;

/*
** Variables
*/
std::atomic<bool> rfInterrupt{false};
volatile bool rxIsrState = 0;
volatile int unpairedBlink = 0;
volatile unsigned long last_packet = 0;
volatile uint8_t telemetry_index = 0;

volatile uint8_t payload_buffer[10];
volatile uint8_t payload_received = 0;

const unsigned long PAIRING_TIMEOUT = 10000;
const uint8_t MAX_ADDRESS_CONFLICTS = 5;

rmt_channel_handle_t tx_channel = NULL;
rmt_encoder_handle_t copy_encoder = NULL;
rmt_symbol_word_t pulse_symbol;

volatile int alternatePWMChannel = 0;
volatile bool PWM_active = 0;
volatile uint16_t PWM0_time = 0;
volatile uint16_t PWM1_time = 0;

volatile uint8_t thr_received = 0;
volatile uint8_t steering_received = 127;

// V2.5-Evo - 2026-07-19 - FM triage: the steering byte calcPWM() actually applied to the motor
// mix this loop (rtm_steer_override while RTM active + override enabled + thr>=25, otherwise the
// user's steering_received). Written by calcPWM() (generatePWM task, 100Hz) and read by the logger
// (loggerTask). Single-byte volatile — atomic on ESP32-C3, same pattern as thr_received. Logged so
// the actuation gap is visible: rtm_steer_override can command a turn while this stays neutral
// because the throttle-release gate suppressed it. 127 = straight ahead.
volatile uint8_t g_effective_steer = 127;

volatile unsigned long get_vesc_timer = 0;
volatile unsigned long last_uart_packet = 0;

volatile uint8_t bind_pin_state = 0;
volatile uint8_t rx_aux_flags = 0;   // set by 0xF4 meta-packet: bit0=strobe, bit3=find-me

float fbatVolt = 0.0;
float noload_offset = 0.0;
uint8_t bc_arr[101];
uint8_t percent_last_val = 0xFF;
uint8_t percent_last_thr = 1;
unsigned long percent_last_thr_change = 0;

// V2.5-Evo: ERPM added to VESC selective-get mask; payload length is 23 bytes.
// P7: ERPM is also read by Phase C RTM anti-spoofing (RTMState.ino) to verify
// VESC speed matches GPS speed during active RTM. gps_en + vesc_erpm_per_kmh>0 required.
#define VESC_MORE_VALUES
#ifdef VESC_MORE_VALUES
  #define VESC_PACK_LEN 27  // +4 bytes for watt_hours (float32_auto)
  uint8_t vescRelayBuffer[34];
#else
  #define VESC_PACK_LEN 9
  uint8_t vescRelayBuffer[15];
#endif

//SPI Pins
#define P_SPI_MISO 6
#define P_SPI_MOSI 7
#define P_SPI_SCK 10
//LORA Pins
#define P_LORA_DIO 3
#define P_LORA_BUSY 4
#define P_LORA_RST 5
#define P_LORA_NSS 8
//Misc Pins
#define P_PWM_OUT 9
#define P_U1_TX 18
#define P_U1_RX 19
#define P_UBAT_MEAS 0
#define P_I2C_SCL 1
#define P_I2C_SDA 2

//AW9523 Pins
#define AP_U1_MUX_0 8
#define AP_U1_MUX_1 9
#define AP_S_BIND 0
#define AP_S_AUX 10
#define AP_L_BIND 1
#define AP_L_AUX 11
#define AP_EN_BMS_MEAS 4
#define AP_BMS_MEAS 7
#define AP_EN_PWM0 13
#define AP_EN_PWM1 12
#define AP_EN_WET_MEAS 14
#define AP_WET_MEAS 15

//Debug options — comment out for release builds
//#define DEBUG_RX
//#define DEBUG_VESC

#if defined DEBUG_RX
   #define rxprint(x)    Serial.print(x)
   #define rxprintln(x)  Serial.println(x)
#else
   #define rxprint(x)
   #define rxprintln(x)
#endif

#ifdef DEBUG_VESC
#define VESC_DEBUG_PRINT(x) Serial.print(x)
#define VESC_DEBUG_PRINTLN(x) Serial.println(x)
#else
#define VESC_DEBUG_PRINT(x)
#define VESC_DEBUG_PRINTLN(x)
#endif

#include "../Common/RadioCommon.h"
#include "../Common/SystemCommon.h"
