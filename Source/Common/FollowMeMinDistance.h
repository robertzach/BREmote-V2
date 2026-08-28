#ifndef BREMOTE_FOLLOW_ME_MIN_DISTANCE_H
#define BREMOTE_FOLLOW_ME_MIN_DISTANCE_H

#include <stdint.h>

enum FollowMeMinDistanceAction : uint8_t {
  FM_MIN_DISTANCE_INACTIVE = 0,
  FM_MIN_DISTANCE_HOLD_STOP,
  FM_MIN_DISTANCE_RECOVER,
  FM_MIN_DISTANCE_HANDOFF_ARMED
};

static inline bool followMeMinDistanceStationarySample(
    bool stop_latched,
    bool position_ok,
    float distance_m,
    float min_distance_m,
    bool rider_speed_finite,
    float rider_speed_kmh,
    float stationary_speed_kmh)
{
  return stop_latched && position_ok &&
      distance_m <= min_distance_m &&
      rider_speed_finite && rider_speed_kmh < stationary_speed_kmh;
}

static inline bool followMeMinDistanceStationaryConfirmed(
    uint32_t stationary_since_ms,
    uint32_t now_ms,
    uint32_t dwell_ms)
{
  return stationary_since_ms != 0 &&
      (uint32_t)(now_ms - stationary_since_ms) >= dwell_ms;
}

// Radial recovery has priority over the manual handoff. A GPS fix outside the stop boundary is no
// longer a min-distance situation, even if the previous stationary dwell had just matured.
static inline FollowMeMinDistanceAction followMeMinDistanceAction(
    bool stop_latched,
    bool position_ok,
    float distance_m,
    float min_distance_m,
    bool trigger_held,
    bool stationary_confirmed)
{
  if (!stop_latched) return FM_MIN_DISTANCE_INACTIVE;
  if (position_ok && distance_m > min_distance_m) return FM_MIN_DISTANCE_RECOVER;
  if (!trigger_held && stationary_confirmed) return FM_MIN_DISTANCE_HANDOFF_ARMED;
  return FM_MIN_DISTANCE_HOLD_STOP;
}

#endif
