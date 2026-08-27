#ifndef STEERING_CURVE_H
#define STEERING_CURVE_H

#include <stdint.h>

static constexpr uint16_t kSteerReductionStartMinPct = 30;
static constexpr uint16_t kSteerReductionStartMaxPct = 80;
static constexpr uint16_t kSteerReductionStartDefaultPct = 50;
static constexpr float kSteerFullThrottleMinPct = 20.0f;
static constexpr float kSteerFullThrottleMaxPct = 100.0f;
static constexpr float kSteerFullThrottleDefaultPct = 35.0f;

enum ThrottleSteeringConfigRepair : uint8_t
{
  STEER_CONFIG_UNCHANGED = 0,
  STEER_CONFIG_REPAIRED_BOTH,
  STEER_CONFIG_REPAIRED_FULL_THROTTLE
};

// Same-version migration/repair for the two reused fields. An invalid start value is the legacy
// marker, so BOTH values are reset: the full-throttle float previously had unrelated low-speed
// semantics and must not be reinterpreted. The ordered comparisons deliberately catch NaN too.
static inline ThrottleSteeringConfigRepair normalizeThrottleSteeringConfig(
    uint16_t &reductionStartPct,
    float &fullThrottleAuthorityPct)
{
  if (reductionStartPct < kSteerReductionStartMinPct ||
      reductionStartPct > kSteerReductionStartMaxPct)
  {
    reductionStartPct = kSteerReductionStartDefaultPct;
    fullThrottleAuthorityPct = kSteerFullThrottleDefaultPct;
    return STEER_CONFIG_REPAIRED_BOTH;
  }

  if (!(fullThrottleAuthorityPct >= kSteerFullThrottleMinPct &&
        fullThrottleAuthorityPct <= kSteerFullThrottleMaxPct))
  {
    fullThrottleAuthorityPct = kSteerFullThrottleDefaultPct;
    return STEER_CONFIG_REPAIRED_FULL_THROTTLE;
  }

  return STEER_CONFIG_UNCHANGED;
}

// Scale steering around its exact neutral (127) as effective throttle rises.
// Authority stays at 100% through reductionStartPct, then follows a smoothstep
// curve to fullThrottleAuthorityPct at full throttle. Zero slope at both ends
// avoids a steering step when crossing the start point or reaching full power.
//
// Deliberately Arduino-independent so the exact control-law arithmetic can be
// host-tested. Callers validate the two configuration percentages.
static inline uint8_t applyThrottleSteeringAuthority(uint8_t steering,
                                                     uint8_t throttle,
                                                     float reductionStartPct,
                                                     float fullThrottleAuthorityPct)
{
  if (reductionStartPct < 0.0f) reductionStartPct = 0.0f;
  if (reductionStartPct >= 100.0f || fullThrottleAuthorityPct >= 100.0f)
  {
    return steering;
  }
  if (fullThrottleAuthorityPct < 0.0f) fullThrottleAuthorityPct = 0.0f;

  const float throttlePct = ((float)throttle * 100.0f) / 255.0f;
  if (throttlePct <= reductionStartPct)
  {
    return steering;
  }

  float t = (throttlePct - reductionStartPct) / (100.0f - reductionStartPct);
  if (t < 0.0f) t = 0.0f;
  if (t > 1.0f) t = 1.0f;

  const float smooth = t * t * (3.0f - 2.0f * t);
  const float minimumAuthority = fullThrottleAuthorityPct / 100.0f;
  const float authority = 1.0f - smooth * (1.0f - minimumAuthority);

  const int delta = (int)steering - 127;
  const float scaledDelta = (float)delta * authority;
  const int roundedDelta = (int)(scaledDelta + ((scaledDelta >= 0.0f) ? 0.5f : -0.5f));
  int output = 127 + roundedDelta;
  if (output < 0) output = 0;
  if (output > 255) output = 255;
  return (uint8_t)output;
}

#endif // STEERING_CURVE_H
