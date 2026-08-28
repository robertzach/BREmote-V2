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

  // A stopped ACTIVE lifecycle always takes the common RETURN cleanup, at any valid distance.
  assert(followMeReturnCandidate(
      false, true, true, 20.0f, engageDistanceM, true, 1.9f, 2.0f));
  assert(followMeReturnCandidate(
      false, true, true, 12.0f, engageDistanceM, true, 1.9f, 2.0f));
  assert(followMeReturnCandidate(
      false, true, true, 5.0f, engageDistanceM, true, 1.9f, 2.0f));

  // ARMED still starts a real retrieval only outside D_engage, avoiding a near-range state loop.
  assert(followMeReturnCandidate(
      true, false, true, 12.1f, engageDistanceM, true, 1.9f, 2.0f));
  assert(!followMeReturnCandidate(
      true, false, true, 12.0f, engageDistanceM, true, 1.9f, 2.0f));
  assert(!followMeReturnCandidate(
      true, false, true, 5.0f, engageDistanceM, true, 1.9f, 2.0f));

  // The proof still needs trustworthy inputs and a continuously stationary rider.
  assert(!followMeReturnCandidate(
      false, true, false, 5.0f, engageDistanceM, true, 1.9f, 2.0f));
  assert(!followMeReturnCandidate(
      false, true, true, 5.0f, engageDistanceM, false, 1.9f, 2.0f));
  assert(!followMeReturnCandidate(
      false, true, true, 5.0f, engageDistanceM, true, 2.0f, 2.0f));
  assert(!followMeReturnCandidate(
      false, false, true, 20.0f, engageDistanceM, true, 1.9f, 2.0f));

  // Arrival is the exact complement of the ARMED outside rule, including equality.
  assert(!followMeReturnArrived(true, 12.1f, engageDistanceM));
  assert(followMeReturnArrived(true, 12.0f, engageDistanceM));
  assert(followMeReturnArrived(true, 5.0f, engageDistanceM));
  assert(!followMeReturnArrived(false, 5.0f, engageDistanceM));

  return 0;
}
