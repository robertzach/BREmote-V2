// V2.5-Evo - 2026-08-28 - Differential motor mixing is now throttle-relative and aggregate-power-neutral: steering redistributes the effective throttle as T-delta / T+delta with delta proportional to T, selected steering and steering_influence. At zero throttle steering therefore commands zero on both motors. Before upper saturation the normalized motor-command sum stays exactly 2*T; saturation can only lower it. The existing throttle-dependent steering-authority curve, inversion, per-channel PWM calibration, trim, motor ramp and final hard-neutral clamp remain in their established order. No config/struct/SW_VERSION change.
// V2.5-Evo - 2026-08-27 - Throttle-dependent steering authority: after manual/FM arbitration, scale the selected steering command about neutral 127 using effective throttle and the configurable smoothstep curve (default full authority through 50%, 35% at full throttle). This covers manual riding, FM manual takeover and automatic FM identically.
// V2.5-Evo - 2026-08-26 - FM manual steering takeover: a rider deflection outside kFmManualSteerDeadband immediately wins over the FM steering override without changing FM state, separation latch or throttle cap. Centring the stick hands steering back to FM. RTM behaviour is unchanged.
// V2.5-Evo - 2026-07-19 - P3 FM: calcPWM() applies fm_throttle_cap (subtract-only, lowest cap wins) and lets fm_rx_active gate the steering override alongside rtm_rx_active. Throttle can still only be reduced, never added, and the thr_received>=25 steering gate is unchanged.
// V2.5-Evo - 2026-07-19 - FM triage: calcPWM() records effective_steer into g_effective_steer (diagnostic observer only — no control-path change) so the logger can show the actuation gap
// V2.5-Evo - 2026-04-30 - calcPWM() applies rtm_approach_cap for RTM approach decel zone
// V2.5-Evo - 2026-04-25 - P7: calcPWM() applies RTM emergency stop and steering override via effective_thr/steer
// V2.5-Evo - 2026-04-28 - Security: gate steer override on thr_received>=25 (belt-and-suspenders)
void generatePWM(void *parameter) {
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = pdMS_TO_TICKS(10);

  while (1)
  {
    // V2.5-Evo - 2026-07-31 - RX-WDT-2: gated. initTasks() creates this task BEFORE
    // initWatchdog() subscribes it, so the first few iterations were calling
    // esp_task_wdt_reset() unregistered — which logs "task not found" at ERROR level and put
    // five spurious errors in every boot log. Control flow is untouched: once g_wdt_active is
    // set the feed happens exactly as before, and before that there is no watchdog to feed.
    // The RX has no display and no LED, so the boot log is the only diagnostic surface it has;
    // fake errors in it are not free.
    if (g_wdt_active) esp_task_wdt_reset();
    calcPWM();

    if(PWM_active && millis()-last_packet < usrConf.failsafe_time)
    {

      // V2.5-Evo - 2026-07-22 - <Rex HIGH> WDT self-preservation on i2cMutex.
      // BUG: this WDT-registered top-priority PWM task took i2cMutex with portMAX_DELAY at the
      // two AW9523 enable-swap sites. i2cMutex is shared with compass/ADS1115/AW9523-LED/logger;
      // if the I2C bus wedged, this task blocked unbounded and the 3000ms WDT panic-rebooted the
      // RX mid-ride. FIX: bound the take to 10ms and skip the swap on timeout (never block >10ms).
      // CONSISTENCY: the enable swap sets up the enable line for the NEXT iteration's pulse, so
      // alternatePWMChannel is now advanced ONLY when the swap actually succeeds. On timeout the
      // channel index is left unchanged, so next cycle re-pulses the SAME, still-enabled motor
      // (correct VESC) instead of firing the other channel's pulse onto the current enable state.
      // Worst case under bus contention: one motor gets repeated pulses for a few 10ms cycles while
      // the other misses updates — no wrong-motor pulse, no unbounded block, no WDT trip. On timeout
      // we do NOT hold the mutex, so we do NOT give it.
      if(alternatePWMChannel)
      {
        generate_pulse(PWM0_time);
        vTaskDelay(pdMS_TO_TICKS(2));
        if(xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(10)) == pdTRUE)
        {
          aw.pinMode(AP_EN_PWM0, INPUT);
          aw.pinMode(AP_EN_PWM1, OUTPUT);
          xSemaphoreGive(i2cMutex);
          alternatePWMChannel = 0;  // advance only on a successful enable swap
        }
        // timeout: keep alternatePWMChannel=1 so PWM0 (still enabled) re-pulses next cycle
      }
      else
      {
        generate_pulse(PWM1_time);
        vTaskDelay(pdMS_TO_TICKS(2));
        if(xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(10)) == pdTRUE)
        {
          aw.pinMode(AP_EN_PWM1, INPUT);
          aw.pinMode(AP_EN_PWM0, OUTPUT);
          xSemaphoreGive(i2cMutex);
          alternatePWMChannel = 1;  // advance only on a successful enable swap
        }
        // timeout: keep alternatePWMChannel=0 so PWM1 (still enabled) re-pulses next cycle
      }
    }
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

void calcPWM()
{
  uint8_t effective_thr = thr_received;

  // V2.5-Evo - 2026-07-19 - P3 FM: apply the Follow-Me throttle cap.
  // Subtract-only: the cap can only
  // ever reduce the rider's throttle, never raise it. fm_throttle_cap is 255 (no cap) whenever FM
  // is idle or merely armed; runFmLoop() drives it to 0 on a fault, during a powered RETURN proof,
  // or after the min_dist stop latch trips, and uses the cap-chain result while FM is actively
  // following. Geometry/front warnings never alter this value. Creator safety philosophy enforced: the
  // human trigger remains the sole throttle source.
  if ((uint8_t)fm_throttle_cap < effective_thr)
  {
    effective_thr = fm_throttle_cap;
  }

  // Also gate on thr_received>=25 so the 100 Hz PWM task cannot apply a stale bearing during
  // the up-to-100 ms window before the navigation loop next runs.
  // V2.5-Evo - 2026-08-26 - FM manual steering takeover. A deliberate rider input wins immediately
  // in this 100 Hz task, while FM remains ACTIVE in the 10 Hz state machine. This preserves the FM
  // mode, separation latch and subtract-only throttle cap; centring the input seamlessly restores
  // the latest automatic steering command.
  int fm_manual_steer_dev = (int)steering_received - 127;
  if (fm_manual_steer_dev < 0) fm_manual_steer_dev = -fm_manual_steer_dev;
  bool fm_manual_steer = fm_rx_active &&
                         (fm_manual_steer_dev >= (int)kFmManualSteerDeadband);
  bool automatic_steer = fm_rx_active && !fm_manual_steer;

  // Autonomous steering never reaches the motors on a released trigger. When FM manual takeover
  // is selected, steering_received is used below even though FM remains active in the background.
  uint8_t effective_steer = (automatic_steer && usrConf.rtm_rx_override_steering &&
                             thr_received >= 25)
                            ? (uint8_t)rtm_steer_override
                            : steering_received;

  // Apply one rollover-protection curve AFTER manual-vs-automatic arbitration, so manual riding,
  // deliberate manual takeover during FM and automatic FM all receive the same limit. Use the
  // throttle actually permitted by the FM safety-cap chain: reduced motor power restores the
  // corresponding low-throttle steering authority. Scaling only the delta from 127 cannot create
  // a left/right bias. A 100% full-throttle setting disables the reduction.
  effective_steer = applyThrottleSteeringAuthority(
      effective_steer,
      effective_thr,
      (float)usrConf.steer_reduction_start_pct,
      usrConf.steer_full_throttle_pct);

  // V2.5-Evo - 2026-07-19 - FM triage: record the steering byte actually applied this loop for
  // the logger. Diagnostic observer only — this write does not alter any PWM/motor control path.
  g_effective_steer = effective_steer;

  if(usrConf.steering_type == 0)
  {
    //Efoil mode
    PWM0_time = constrain(map(effective_thr, 0, 255, usrConf.PWM0_min, usrConf.PWM0_max) + usrConf.trim, usrConf.PWM0_min, usrConf.PWM0_max);
    PWM1_time = constrain(map(effective_thr, 0, 255, usrConf.PWM1_min, usrConf.PWM1_max) - usrConf.trim, usrConf.PWM1_min, usrConf.PWM1_max);
  }
  else if(usrConf.steering_type == 1)
  {
    //Diff
    // Steering redistributes the permitted throttle instead of adding a gas-independent offset from
    // the full PWM span. This is calculated in normalized command space first, so unequal channel
    // calibration ranges do not distort the requested motor-power ratio.
    DifferentialMotorMix motor_mix = mixThrottleRelativeDifferential(
        effective_thr, effective_steer, usrConf.steering_influence,
        usrConf.steering_inverted);
    int motor_0_pwm = map(motor_mix.motor0, 0, 255, usrConf.PWM0_min, usrConf.PWM0_max);
    int motor_1_pwm = map(motor_mix.motor1, 0, 255, usrConf.PWM1_min, usrConf.PWM1_max);

    // Trim remains a symmetric post-calibration correction. The final effective_thr==0 clamp below
    // still owns the absolute stopped state and cannot be bypassed by trim or ramp residue.
    PWM0_time = constrain(motor_0_pwm + usrConf.trim, usrConf.PWM0_min, usrConf.PWM0_max);
    PWM1_time = constrain(motor_1_pwm - usrConf.trim, usrConf.PWM1_min, usrConf.PWM1_max);
  }
  else if(usrConf.steering_type == 2)
  {
    //Servo
    PWM0_time = map(effective_thr, 0, 255, usrConf.PWM0_min, usrConf.PWM0_max);
    if(usrConf.steering_inverted)
    {
      PWM1_time = constrain(map(effective_steer, 0, 255, usrConf.PWM1_min, usrConf.PWM1_max)+usrConf.trim, usrConf.PWM1_min, usrConf.PWM1_max);
    }
    else
    {
      PWM1_time = constrain(map(effective_steer, 255, 0, usrConf.PWM1_min, usrConf.PWM1_max)+usrConf.trim, usrConf.PWM1_min, usrConf.PWM1_max);
    }
  }
  else
  {
    PWM_active = 0;
  }

  // ── SAFETY: MOTOR RAMPING (usrConf.motor_ramp_s, seconds) ──────────────────────
  // Rise-limit BOTH motor outputs so 0->full takes motor_ramp_s seconds. Prevents a violent throttle
  // yank AND a single motor taking off (throttle- or steering-driven). FALL is instant so release /
  // failsafe / RTM e-stop / straightening drop the motor immediately. By design this also ramps the
  // differential-steering response (a sharp turn builds over this time). 0 = instant/off.
  if (usrConf.motor_ramp_s > 0.001f)
  {
    static uint16_t pwm0_ramp = 0, pwm1_ramp = 0;
    static bool     ramp_init = false;
    if (!ramp_init) { pwm0_ramp = usrConf.PWM0_min; pwm1_ramp = usrConf.PWM1_min; ramp_init = true; }
    uint16_t step0 = (uint16_t)max(1.0f, (float)(usrConf.PWM0_max - usrConf.PWM0_min) / (usrConf.motor_ramp_s * 100.0f));
    uint16_t step1 = (uint16_t)max(1.0f, (float)(usrConf.PWM1_max - usrConf.PWM1_min) / (usrConf.motor_ramp_s * 100.0f));
    if (PWM0_time > pwm0_ramp + step0) pwm0_ramp += step0; else pwm0_ramp = PWM0_time;
    if (PWM1_time > pwm1_ramp + step1) pwm1_ramp += step1; else pwm1_ramp = PWM1_time;
    PWM0_time = pwm0_ramp;
    PWM1_time = pwm1_ramp;
  }

  // ============================================================
  // V2.5-Evo - 2026-07-27 - SAFETY-NEUTRAL-1: RELEASED THROTTLE == ABSOLUTE MINIMUM.
  //
  // THIS RUNS LAST, AFTER EVERY OTHER CALCULATION, ON PURPOSE. It is a structural
  // guarantee, not a correction of any one contributor: with the trigger released, both
  // outputs are the configured minimum and NOTHING is permitted to lift them.
  //
  // WHAT WAS WRONG IN THE FORMER FULL-SPAN MIXER — confirmed by arithmetic, not inference:
  //   throttle_0 = map(0, 0,255, PWM0_min, PWM0_max)  ->  PWM0_min
  //   PWM0_time  = constrain(throttle_0 + usrConf.trim - steering_offset_0, min, max)
  // so at ZERO throttle the output was PWM0_min + trim. Any non-zero trim parked a motor
  // above its minimum forever — trigger released, radio idle, buggy on the dock. The
  // asymmetry matches the field report exactly: POSITIVE trim lifts motor 0, NEGATIVE trim
  // lifts motor 1, so exactly ONE motor creeps. Owner observed one motor starting on its
  // own at the dock, 2026-07-27, and had been compensating by widening the VESC's own
  // deadband — i.e. masking an RX bug inside the ESC.
  //
  // The later 2026-08-28 throttle-relative mixer separately removes that former steering path:
  // its turn term is zero whenever effective_thr is zero, regardless of steering-centre drift.
  // This final clamp deliberately remains as defence in depth for trim, ramp residue and future
  // contributors; it is the last writer and therefore the stronger stopped-state invariant.
  //
  // WHY IT IS WRITTEN THIS WAY: patching individual contributors alone would leave ramp residue
  // and whatever is added next able to put throttle on a motor the rider is not asking for.
  // Enforcing the invariant ONCE, at
  // the end, makes it independent of every upstream term. The creator safety philosophy
  // already written throughout this file ("the human trigger remains the sole throttle
  // source") is now enforced instead of merely assumed.
  //
  // The test is effective_thr, not thr_received: RTM e-stop and the FM/RTM caps all drive
  // effective_thr to 0, so an autonomous stop lands on true minimum too rather than on
  // minimum-plus-trim.
  //
  // NOTE: this is deliberately == 0 and NOT a deadband. If the TX ever sends a non-zero
  // throttle with the trigger released, that is a TX fault that must be found and fixed at
  // source, and swallowing it in a deadband here would hide it.
  // ============================================================
  if (effective_thr == 0)
  {
    PWM0_time = usrConf.PWM0_min;
    PWM1_time = usrConf.PWM1_min;
  }
}

void initRMT()
{
  // Initialize RMT TX channel
  rmt_tx_channel_config_t tx_chan_config = {
    .gpio_num = RMT_TX_GPIO_NUM,
    .clk_src = RMT_CLK_SRC_DEFAULT,  // Select APB clock (80MHz)
    .resolution_hz = 1000000,         // 1MHz, 1 tick = 1μs
    .mem_block_symbols = 64,
    .trans_queue_depth = 4,
  };

  tx_chan_config.flags.io_od_mode = 0; //open-drain
  // Create RMT TX channel
  ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &tx_channel));
  // Create RMT encoder
  rmt_copy_encoder_config_t copy_encoder_config = {};
  ESP_ERROR_CHECK(rmt_new_copy_encoder(&copy_encoder_config, &copy_encoder));
  // Enable RMT TX channel
  ESP_ERROR_CHECK(rmt_enable(tx_channel));
}

void generate_pulse(uint16_t pulse_width_us) 
{
  pulse_symbol.level0 = 1;
  pulse_symbol.duration0 = pulse_width_us;  // High time in microseconds
  pulse_symbol.level1 = 0;
  pulse_symbol.duration1 = 1;  // Low time in microseconds
  // Create a transmission that loops the same pattern (creates a continuous signal)
  rmt_transmit_config_t tx_config = {
    .loop_count = 1,  // Infinite loop
  };
  tx_config.flags.eot_level = 0; // End-of-transmission level (LOW)
  // Send the pulse pattern
  ESP_ERROR_CHECK(rmt_transmit(tx_channel, copy_encoder, &pulse_symbol, 
                              sizeof(pulse_symbol), &tx_config));
}
