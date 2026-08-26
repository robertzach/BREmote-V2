#ifndef STEERING_CURVE_H
#define STEERING_CURVE_H

#include <stdint.h>

// Throttle-dependent steering authority shared by manual, RTM and Follow-Me control.
// The steering byte is scaled around its exact neutral (127), so the curve cannot create
// a steering bias. Authority stays at 100% through reductionStartPct, then follows a
// smoothstep curve to fullThrottleAuthorityPct at full throttle. The zero slope at both
// ends avoids a steering step when the throttle crosses the start point or reaches full.
//
// This helper deliberately has no Arduino dependencies so its exact control-law arithmetic
// can be host-tested. Callers are responsible for validating the two percentage settings.
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
  const float minAuthority = fullThrottleAuthorityPct / 100.0f;
  const float authority = 1.0f - smooth * (1.0f - minAuthority);

  const int delta = (int)steering - 127;
  const float scaledDelta = (float)delta * authority;
  const int roundedDelta = (int)(scaledDelta + ((scaledDelta >= 0.0f) ? 0.5f : -0.5f));
  int output = 127 + roundedDelta;
  if (output < 0) output = 0;
  if (output > 255) output = 255;
  return (uint8_t)output;
}

#endif // STEERING_CURVE_H
