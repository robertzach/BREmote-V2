#include <assert.h>
#include <stdint.h>

#include "../../Source/Common/SteeringCurve.h"

int main()
{
  const float startPct = 50.0f;
  const float fullPct = 35.0f;

  // Neutral can never acquire a steering bias.
  for (int throttle = 0; throttle <= 255; throttle++)
  {
    assert(applyThrottleSteeringAuthority(127, (uint8_t)throttle,
                                          startPct, fullPct) == 127);
  }

  // Authority is untouched below the configured start and lands exactly on the configured
  // full-throttle endpoint (subject only to one-byte rounding around neutral).
  assert(applyThrottleSteeringAuthority(255, 127, startPct, fullPct) == 255);
  assert(applyThrottleSteeringAuthority(0,   127, startPct, fullPct) == 0);
  assert(applyThrottleSteeringAuthority(255, 255, startPct, fullPct) == 172);
  assert(applyThrottleSteeringAuthority(0,   255, startPct, fullPct) == 83);

  // As throttle rises, neither steering direction may gain authority.
  int previousRight = 128;
  int previousLeft = 127;
  for (int throttle = 0; throttle <= 255; throttle++)
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

  // A 100% endpoint is the documented feature-off setting.
  for (int steering = 0; steering <= 255; steering++)
  {
    assert(applyThrottleSteeringAuthority((uint8_t)steering, 255,
                                          startPct, 100.0f) == steering);
  }

  return 0;
}
