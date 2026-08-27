#include <assert.h>
#include <math.h>
#include <stdint.h>

#include "../../Source/Common/FollowMeGeometry.h"

static bool closeTo(float actual, float expected, float tolerance = 0.001f)
{
  return fabsf(actual - expected) <= tolerance;
}

int main()
{
  assert(!followMeIsFrontMode(3));
  assert(followMeIsFrontMode(4));
  assert(followMeIsFrontMode(5));
  assert(followMeIsFrontMode(6));
  assert(!followMeIsFrontMode(7));

  assert(followMeStepActiveMode(1, -1) == 6);
  assert(followMeStepActiveMode(2, -1) == 1);
  assert(followMeStepActiveMode(5, +1) == 6);
  assert(followMeStepActiveMode(6, +1) == 1);
  assert(followMeStepActiveMode(4, 0) == 4);
  assert(followMeStepActiveMode(0, +1) == 1);

  uint8_t repeated = 1;
  for (int i = 0; i < 12; ++i) {
    repeated = followMeStepActiveMode(repeated, +1);
    assert(followMeIsActiveMode(repeated));
  }
  assert(repeated == 1);  // two complete F1-F6 loops, never F0

  const float radius = 20.0f;
  const float diagonal = 45.0f;
  const float component = 14.142136f;

  FollowMeFrontStation left = followMeFrontStation(radius, 4, diagonal);
  FollowMeFrontStation front = followMeFrontStation(radius, 5, diagonal);
  FollowMeFrontStation right = followMeFrontStation(radius, 6, diagonal);

  assert(closeTo(left.offset_deg, -45.0f));
  assert(closeTo(left.along_m, component));
  assert(closeTo(left.cross_m, -component));

  assert(closeTo(front.offset_deg, 0.0f));
  assert(closeTo(front.along_m, radius));
  assert(closeTo(front.cross_m, 0.0f));

  assert(closeTo(right.offset_deg, 45.0f));
  assert(closeTo(right.along_m, component));
  assert(closeTo(right.cross_m, component));

  // Every station remains on the configured radial circle.
  assert(closeTo(hypotf(left.along_m, left.cross_m), radius));
  assert(closeTo(hypotf(front.along_m, front.cross_m), radius));
  assert(closeTo(hypotf(right.along_m, right.cross_m), radius));

  // Invalid or rear-pointing front offsets cannot put a front mode behind the rider.
  FollowMeFrontStation clamped = followMeFrontStation(radius, 6, 120.0f);
  assert(closeTo(clamped.offset_deg, 89.0f));
  assert(clamped.along_m > 0.0f);

  FollowMeFrontStation nanOffset = followMeFrontStation(radius, 4, NAN);
  assert(closeTo(nanOffset.offset_deg, 0.0f));
  assert(closeTo(nanOffset.along_m, radius));
  assert(closeTo(nanOffset.cross_m, 0.0f));

  return 0;
}
