#include <assert.h>
#include <math.h>
#include <stdint.h>

#include "../../Source/Common/FollowMeSpeedControl.h"

int main()
{
  // Catch-up always requests speed cap 3 open, regardless of the inherited in-band result.
  assert(followMeSpeedGovernorCap(true, 0) == 255u);
  assert(followMeSpeedGovernorCap(true, 60) == 255u);
  assert(followMeSpeedGovernorCap(true, 254) == 255u);

  // Once catch-up ends, the PI governor's in-band result passes through unchanged.
  assert(followMeSpeedGovernorCap(false, 0) == 0u);
  assert(followMeSpeedGovernorCap(false, 60) == 60u);
  assert(followMeSpeedGovernorCap(false, 255) == 255u);

  FollowMeSpeedCapExposure exposure = {255.0f, 0};

  // Safety reductions are immediate, including before the first usable GPS timestamp.
  assert(followMeExposeSpeedGovernorCap(exposure, 0, 0, 35.0f) == 0u);
  assert(exposure.cap == 0.0f);

  // A hidden desired cap cannot return without a fresh speed sample.
  assert(followMeExposeSpeedGovernorCap(exposure, 201, 0, 35.0f) == 0u);
  assert(followMeExposeSpeedGovernorCap(exposure, 201, 1000, 35.0f) == 3u);
  assert(fabsf(exposure.cap - 3.5f) < 0.001f);
  assert(followMeExposeSpeedGovernorCap(exposure, 201, 1000, 35.0f) == 3u);
  assert(fabsf(exposure.cap - 3.5f) < 0.001f);

  // The measured 335 ms recovery interval may restore only 11.725 counts, not jump to 201.
  assert(followMeExposeSpeedGovernorCap(exposure, 201, 1335, 35.0f) == 15u);
  assert(fabsf(exposure.cap - 15.225f) < 0.001f);

  // A new tighter request always wins immediately, even without another GPS sample.
  assert(followMeExposeSpeedGovernorCap(exposure, 7, 1335, 35.0f) == 7u);

  // Catch-up's desired 255 follows the same rise limit instead of reopening in one tick.
  assert(followMeExposeSpeedGovernorCap(exposure, 255, 1435, 35.0f) == 10u);
  assert(fabsf(exposure.cap - 10.5f) < 0.001f);

  // Implausibly long GPS intervals use the controller's defensive 100 ms timestep.
  assert(followMeExposeSpeedGovernorCap(exposure, 255, 3435, 35.0f) == 14u);
  assert(fabsf(exposure.cap - 14.0f) < 0.001f);

  // Unsigned timestamp subtraction preserves the same limit across millis() wrap.
  exposure.cap = 0.0f;
  exposure.last_gps_ms = 0xFFFFFFF0u;
  assert(followMeExposeSpeedGovernorCap(exposure, 255, 84u, 35.0f) == 3u);
  assert(fabsf(exposure.cap - 3.5f) < 0.001f);

  // Corrupt/non-finite exposure state fails closed instead of restoring full authority.
  exposure.cap = NAN;
  assert(followMeExposeSpeedGovernorCap(exposure, 255, 184u, 35.0f) == 3u);
  assert(fabsf(exposure.cap - 3.5f) < 0.001f);

  followMeResetSpeedCapExposure(exposure);
  assert(exposure.cap == 255.0f);
  assert(exposure.last_gps_ms == 0u);

  return 0;
}
