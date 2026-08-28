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
  const float frontRadius = 40.0f;
  const float component = 28.284271f;

  assert(closeTo(followMeFrontStationRadius(radius), frontRadius));
  assert(closeTo(followMeFrontLookaheadM(radius), 40.0f));

  FollowMeFrontStation left = followMeFrontStation(frontRadius, 4, diagonal);
  FollowMeFrontStation front = followMeFrontStation(frontRadius, 5, diagonal);
  FollowMeFrontStation right = followMeFrontStation(frontRadius, 6, diagonal);

  assert(closeTo(left.offset_deg, -45.0f));
  assert(closeTo(left.along_m, component));
  assert(closeTo(left.cross_m, -component));

  assert(closeTo(front.offset_deg, 0.0f));
  assert(closeTo(front.along_m, frontRadius));
  assert(closeTo(front.cross_m, 0.0f));

  assert(closeTo(right.offset_deg, 45.0f));
  assert(closeTo(right.along_m, component));
  assert(closeTo(right.cross_m, component));

  // With the 20 m base defaults, nominal steering points are 80 m ahead for F5 and 68.3 m
  // ahead for F4/F6: doubled station plus another doubled-base course lookahead.
  const float lookahead = followMeFrontLookaheadM(radius);
  assert(closeTo(front.along_m + lookahead, 80.0f));
  assert(closeTo(left.along_m + lookahead, 68.284271f));
  assert(closeTo(right.along_m + lookahead, 68.284271f));

  // Every station remains on the configured radial circle.
  assert(closeTo(hypotf(left.along_m, left.cross_m), frontRadius));
  assert(closeTo(hypotf(front.along_m, front.cross_m), frontRadius));
  assert(closeTo(hypotf(right.along_m, right.cross_m), frontRadius));

  // Invalid or rear-pointing front offsets cannot put a front mode behind the rider.
  FollowMeFrontStation clamped = followMeFrontStation(frontRadius, 6, 120.0f);
  assert(closeTo(clamped.offset_deg, 89.0f));
  assert(clamped.along_m > 0.0f);

  FollowMeFrontStation nanOffset = followMeFrontStation(frontRadius, 4, NAN);
  assert(closeTo(nanOffset.offset_deg, 0.0f));
  assert(closeTo(nanOffset.along_m, frontRadius));
  assert(closeTo(nanOffset.cross_m, 0.0f));

  return 0;
}
