#include <assert.h>

#include "../../Source/Common/FollowMeActivation.h"

int main()
{
  const float engageDistanceM = 12.0f;

  // Link loss is detected at exactly the same boundary as PWM.ino's pulse-output failsafe.
  assert(followMeLinkHealthy(1000, 3999, 3000));
  assert(!followMeLinkHealthy(1000, 4000, 3000));
  assert(!followMeLinkHealthy(0, 100, 3000));
  assert(followMeLinkHealthy(0xFFFFFF00u, 0x00000010u, 3000));

  // A control-packet recovery still waits for one post-outage TX GPS sample.
  assert(!followMeTxGpsRefreshedAfterLinkHold(2500, 2500));
  assert(!followMeTxGpsRefreshedAfterLinkHold(2500, 0));
  assert(followMeTxGpsRefreshedAfterLinkHold(2500, 4100));
  assert(followMeLinkRecoveryShouldHold(false, 4000, 9999, 6000));
  assert(!followMeLinkRecoveryShouldHold(false, 4000, 10000, 6000));
  assert(!followMeLinkRecoveryShouldHold(true, 4000, 4001, 6000));
  assert(followMeLinkRecoveryShouldHold(false, 0xFFFFFF00u, 0x00000010u, 6000));

  // Timeout removes outputs but preserves every live lifecycle state exactly.
  const uint8_t linkHeldStates[] = { 1, 2, 5 }; // ARMED, ACTIVE, RETURN
  for (uint8_t state : linkHeldStates) {
    FollowMeLinkHoldPolicy hold = followMeLinkHold(state);
    assert(hold.lifecycle_state == state);
    assert(!hold.automatic_authority);
    assert(hold.throttle_cap == 0);
  }

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

  // Actual automatic motor/steering authority needs both the physical deadman and a live link.
  assert(!followMeAutomaticAuthority(true, false, true));
  assert(followMeAutomaticAuthority(true, true, true));
  assert(!followMeAutomaticAuthority(false, true, true));
  assert(!followMeAutomaticAuthority(true, true, false));
  assert(followMeAutomaticAuthority(true, true, true)); // recovery may re-enter via the ramp

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
