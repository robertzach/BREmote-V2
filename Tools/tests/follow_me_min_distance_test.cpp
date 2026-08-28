#include <assert.h>

#include "../../Source/Common/FollowMeMinDistance.h"

int main()
{
  const float minDistanceM = 10.0f;
  const float stationarySpeedKmh = 2.0f;

  assert(followMeMinDistanceAction(
      false, true, 9.0f, minDistanceM, true, true) ==
      FM_MIN_DISTANCE_INACTIVE);

  // Entering or remaining inside the boundary always holds cap 0. Releasing while still moving
  // must not clear either latch or hand the lifecycle back to FM_ARMED.
  assert(followMeMinDistanceAction(
      true, true, 9.0f, minDistanceM, true, false) ==
      FM_MIN_DISTANCE_HOLD_STOP);
  assert(followMeMinDistanceAction(
      true, true, 9.0f, minDistanceM, false, false) ==
      FM_MIN_DISTANCE_HOLD_STOP);

  // A trustworthy radial recovery wins with either trigger posture and retains separation.
  assert(followMeMinDistanceAction(
      true, true, 10.1f, minDistanceM, true, false) ==
      FM_MIN_DISTANCE_RECOVER);
  assert(followMeMinDistanceAction(
      true, true, 10.1f, minDistanceM, false, true) ==
      FM_MIN_DISTANCE_RECOVER);

  // Stationarity alone does not expose manual cap 255; the physical release is the state edge.
  assert(followMeMinDistanceAction(
      true, true, 9.0f, minDistanceM, true, true) ==
      FM_MIN_DISTANCE_HOLD_STOP);
  assert(followMeMinDistanceAction(
      true, true, 9.0f, minDistanceM, false, true) ==
      FM_MIN_DISTANCE_HANDOFF_ARMED);

  // Untrusted position cannot build the stationary proof or release a stop through distance.
  assert(!followMeMinDistanceStationarySample(
      true, false, 9.0f, minDistanceM, true, 0.0f, stationarySpeedKmh));
  assert(followMeMinDistanceAction(
      true, false, 20.0f, minDistanceM, false, false) ==
      FM_MIN_DISTANCE_HOLD_STOP);

  assert(followMeMinDistanceStationarySample(
      true, true, 9.0f, minDistanceM, true, 1.9f, stationarySpeedKmh));
  assert(!followMeMinDistanceStationarySample(
      true, true, 9.0f, minDistanceM, true, 2.0f, stationarySpeedKmh));
  assert(!followMeMinDistanceStationarySample(
      true, true, 9.0f, minDistanceM, false, 0.0f, stationarySpeedKmh));

  assert(!followMeMinDistanceStationaryConfirmed(1000, 2999, 2000));
  assert(followMeMinDistanceStationaryConfirmed(1000, 3000, 2000));
  assert(!followMeMinDistanceStationaryConfirmed(0, 5000, 2000));

  // The usual unsigned millis rollover must not break a dwell already in progress.
  assert(followMeMinDistanceStationaryConfirmed(0xFFFFFF00u, 0x00000700u, 2000));

  return 0;
}
