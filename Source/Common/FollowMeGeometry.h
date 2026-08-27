#pragma once

#include <math.h>
#include <stdint.h>

struct FollowMeFrontStation {
  float offset_deg;
  float along_m;
  float cross_m;
};

static inline bool followMeIsFrontMode(uint8_t mode)
{
  return mode >= 4 && mode <= 6;
}

// Bearings increase clockwise: rider-left is negative, rider-right positive. The stored diagonal
// field remains backwards-compatible through 180 degrees for the rear modes, but front use is
// clamped below 90 degrees so a mode named Front cannot put its station behind the rider.
static inline float followMeFrontOffsetDeg(uint8_t mode, float configured_offset_deg)
{
  float offset = fabsf(configured_offset_deg);
  if (!(offset >= 0.0f)) offset = 0.0f;  // NaN fails closed to straight ahead
  if (offset > 89.0f) offset = 89.0f;
  if (mode == 4) return -offset;         // Front-Left
  if (mode == 6) return +offset;         // Front-Right
  return 0.0f;                           // F5 Front (and defensive fallback)
}

static inline FollowMeFrontStation followMeFrontStation(float radius_m, uint8_t mode,
                                                         float configured_offset_deg)
{
  FollowMeFrontStation station = {};
  station.offset_deg = followMeFrontOffsetDeg(mode, configured_offset_deg);
  float offset_rad = station.offset_deg * 0.01745329251994329577f;
  station.along_m = radius_m * cosf(offset_rad);
  station.cross_m = radius_m * sinf(offset_rad);
  return station;
}
