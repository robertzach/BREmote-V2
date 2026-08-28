#include <assert.h>
#include <stdint.h>

#include "../../Source/Common/FollowMeSpeedControl.h"

int main()
{
  // Catch-up always opens speed cap 3, regardless of the configured/inherited in-band result.
  assert(followMeSpeedGovernorCap(true, 0) == 255u);
  assert(followMeSpeedGovernorCap(true, 60) == 255u);
  assert(followMeSpeedGovernorCap(true, 254) == 255u);

  // Once catch-up ends, the PI governor's in-band result passes through unchanged.
  assert(followMeSpeedGovernorCap(false, 0) == 0u);
  assert(followMeSpeedGovernorCap(false, 60) == 60u);
  assert(followMeSpeedGovernorCap(false, 255) == 255u);

  return 0;
}
