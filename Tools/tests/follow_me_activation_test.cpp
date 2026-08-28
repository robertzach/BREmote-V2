#include <assert.h>

#include "../../Source/Common/FollowMeActivation.h"

int main()
{
  const float engageDistanceM = 12.0f;

  // Separation is a sensor/geometry proof. There is intentionally no trigger input to this API.
  assert(followMeSeparationSample(
      true, true, false, 12.1f, engageDistanceM));
  assert(!followMeSeparationSample(
      false, true, false, 12.1f, engageDistanceM));
  assert(!followMeSeparationSample(
      true, false, false, 12.1f, engageDistanceM));
  assert(!followMeSeparationSample(
      true, true, true, 12.1f, engageDistanceM));
  assert(!followMeSeparationSample(
      true, true, false, 12.0f, engageDistanceM));
  assert(!followMeSeparationConfirmed(1000, 2999, 2000));
  assert(followMeSeparationConfirmed(1000, 3000, 2000));
  assert(!followMeSeparationConfirmed(0, 5000, 2000));
  assert(followMeSeparationConfirmed(0xFFFFFF00u, 0x00000700u, 2000));

  // A proven lifecycle may enter FM_ACTIVE regardless of trigger posture.
  assert(followMeLifecycleReady(true, false, true, false, false));
  assert(!followMeLifecycleReady(false, false, true, false, false));
  assert(!followMeLifecycleReady(true, true, true, false, false));
  assert(!followMeLifecycleReady(true, false, false, false, false));
  assert(!followMeLifecycleReady(true, false, true, true, false));
  assert(!followMeLifecycleReady(true, false, true, false, true));

  // Actual automatic motor/steering authority remains a physical-deadman decision.
  assert(!followMeAutomaticAuthority(true, false));
  assert(followMeAutomaticAuthority(true, true));
  assert(!followMeAutomaticAuthority(false, true));

  // With authority withheld, readiness still causes the requested FM_ARMED -> FM_ACTIVE edge.
  assert(followMeActiveLifecycle(false, true));
  assert(followMeActiveLifecycle(true, false));
  assert(!followMeActiveLifecycle(false, false));

  return 0;
}
