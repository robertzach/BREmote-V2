#include <assert.h>
#include <math.h>
#include <stdint.h>

#include "../../Source/Common/SteeringCurve.h"

int main()
{
  const float startPct = 50.0f;
  const float fullPct = 35.0f;

  // Neutral must never acquire a steering bias at any throttle.
  for (int throttle = 0; throttle <= 255; ++throttle)
  {
    assert(applyThrottleSteeringAuthority(127, (uint8_t)throttle,
                                          startPct, fullPct) == 127);
  }

  // Full authority below the configured start and the configured endpoint at full throttle.
  assert(applyThrottleSteeringAuthority(255, 127, startPct, fullPct) == 255);
  assert(applyThrottleSteeringAuthority(0,   127, startPct, fullPct) == 0);
  assert(applyThrottleSteeringAuthority(255, 255, startPct, fullPct) == 172);
  assert(applyThrottleSteeringAuthority(0,   255, startPct, fullPct) == 83);

  // Neither direction may gain authority as throttle rises.
  int previousRight = 128;
  int previousLeft = 127;
  for (int throttle = 0; throttle <= 255; ++throttle)
  {
    const int right = (int)applyThrottleSteeringAuthority(
        255, (uint8_t)throttle, startPct, fullPct) - 127;
    const int left = 127 - (int)applyThrottleSteeringAuthority(
        0, (uint8_t)throttle, startPct, fullPct);
    assert(right <= previousRight);
    assert(left <= previousLeft);
    assert(right >= 0);
    assert(left >= 0);
    previousRight = right;
    previousLeft = left;
  }

  // 100% retained authority is the documented feature-off setting.
  for (int steering = 0; steering <= 255; ++steering)
  {
    assert(applyThrottleSteeringAuthority((uint8_t)steering, 255,
                                          startPct, 100.0f) == steering);
  }

  // A normal SW35 legacy config has zero in the old reserved start slot and the former
  // foiler-low-speed default (8) in the reclaimed float. Both must become 50/35.
  uint16_t migratedStart = 0;
  float migratedFull = 8.0f;
  assert(normalizeThrottleSteeringConfig(migratedStart, migratedFull) ==
         STEER_CONFIG_REPAIRED_BOTH);
  assert(migratedStart == 50);
  assert(migratedFull == 35.0f);

  // A valid start paired with an old/invalid float keeps the start and repairs only the endpoint.
  migratedStart = 60;
  migratedFull = 8.0f;
  assert(normalizeThrottleSteeringConfig(migratedStart, migratedFull) ==
         STEER_CONFIG_REPAIRED_FULL_THROTTLE);
  assert(migratedStart == 60);
  assert(migratedFull == 35.0f);

  // NaN cannot bypass the endpoint validator.
  migratedStart = 50;
  migratedFull = NAN;
  assert(normalizeThrottleSteeringConfig(migratedStart, migratedFull) ==
         STEER_CONFIG_REPAIRED_FULL_THROTTLE);
  assert(migratedFull == 35.0f);

  return 0;
}
