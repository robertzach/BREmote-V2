#include <assert.h>

#include "../../Source/Common/FollowMeMinDistance.h"

int main()
{
  const float minDistanceM = 10.0f;

  assert(followMeMinDistanceAction(
      false, true, 9.0f, minDistanceM) ==
      FM_MIN_DISTANCE_INACTIVE);

  // Entering or remaining inside the boundary always holds cap 0. Trigger posture and stationary
  // lifecycle completion belong to the caller/FM_RETURN and cannot clear this stop directly.
  assert(followMeMinDistanceAction(
      true, true, 9.0f, minDistanceM) ==
      FM_MIN_DISTANCE_HOLD_STOP);
  assert(followMeMinDistanceAction(
      true, true, 10.0f, minDistanceM) ==
      FM_MIN_DISTANCE_HOLD_STOP);

  // A trustworthy radial recovery retains separation.
  assert(followMeMinDistanceAction(
      true, true, 10.1f, minDistanceM) ==
      FM_MIN_DISTANCE_RECOVER);

  // Untrusted position cannot release a stop through distance.
  assert(followMeMinDistanceAction(
      true, false, 20.0f, minDistanceM) ==
      FM_MIN_DISTANCE_HOLD_STOP);

  return 0;
}
