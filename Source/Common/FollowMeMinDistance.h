#ifndef BREMOTE_FOLLOW_ME_MIN_DISTANCE_H
#define BREMOTE_FOLLOW_ME_MIN_DISTANCE_H

#include <stdint.h>

enum FollowMeMinDistanceAction : uint8_t {
  FM_MIN_DISTANCE_INACTIVE = 0,
  FM_MIN_DISTANCE_HOLD_STOP,
  FM_MIN_DISTANCE_RECOVER
};

// Stationary lifecycle completion is owned exclusively by FM_RETURN. The min-distance state now has
// one responsibility: hold cap 0 inside the boundary and recover without clearing separation when a
// trustworthy radial sample moves back outside it.
static inline FollowMeMinDistanceAction followMeMinDistanceAction(
    bool stop_latched,
    bool position_ok,
    float distance_m,
    float min_distance_m)
{
  if (!stop_latched) return FM_MIN_DISTANCE_INACTIVE;
  if (position_ok && distance_m > min_distance_m) return FM_MIN_DISTANCE_RECOVER;
  return FM_MIN_DISTANCE_HOLD_STOP;
}

#endif
