#pragma once

#include <stdint.h>

struct FollowMeSpeedCapExposure {
  float cap;
  uint32_t last_gps_ms;
};

static inline void followMeResetSpeedCapExposure(FollowMeSpeedCapExposure &state)
{
  state.cap = 255.0f;
  state.last_gps_ms = 0;
}

// Publish the speed governor's requested cap without allowing a hidden PI/catch-up value to jump
// back into the motor path after an overspeed reduction. Any reduction is safety-relevant and lands
// immediately. Authority is restored only on a fresh GPS-speed sample and no faster than rise_per_s.
// Keeping this state outside the PI also makes catch-up (desired cap 255) obey the same recovery law.
static inline uint16_t followMeExposeSpeedGovernorCap(FollowMeSpeedCapExposure &state,
                                                      uint16_t desired_cap,
                                                      uint32_t gps_ms,
                                                      float rise_per_s)
{
  if (desired_cap > 255u) desired_cap = 255u;
  if (!(state.cap >= 0.0f && state.cap <= 255.0f)) state.cap = 0.0f;
  if (!(rise_per_s >= 0.0f)) rise_per_s = 0.0f;

  bool fresh_sample = gps_ms != 0 && gps_ms != state.last_gps_ms;
  float dt_s = 0.0f;
  if (fresh_sample) {
    if (state.last_gps_ms == 0) {
      dt_s = 0.1f;
    } else {
      uint32_t dt_ms = gps_ms - state.last_gps_ms;  // unsigned subtraction is millis()-wrap safe
      dt_s = (float)dt_ms / 1000.0f;
      if (dt_s < 0.05f || dt_s > 1.5f) dt_s = 0.1f;
    }
    state.last_gps_ms = gps_ms;
  }

  if ((float)desired_cap < state.cap) {
    state.cap = (float)desired_cap;
  } else if (fresh_sample && (float)desired_cap > state.cap) {
    float restored = state.cap + (rise_per_s * dt_s);
    state.cap = (restored < (float)desired_cap) ? restored : (float)desired_cap;
  }

  if (state.cap > 255.0f) state.cap = 255.0f;
  if (state.cap <   0.0f) state.cap =   0.0f;
  return (uint16_t)state.cap;
}

// Catch-up deliberately requests only the GPS speed-governor cap open. The caller still applies the
// bounded exposure above and arbitrates against align, engage, approach, hard-stop, fault and trigger.
static inline uint16_t followMeSpeedGovernorCap(bool catchup_active,
                                                uint16_t in_band_speed_cap)
{
  return catchup_active ? 255u : in_band_speed_cap;
}
