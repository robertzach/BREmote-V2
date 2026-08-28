#pragma once

#include <stdint.h>

// Catch-up deliberately removes only the GPS speed-governor cap. The caller still arbitrates this
// result against align, engage, approach, hard-stop, fault and human-trigger limits.
static inline uint16_t followMeSpeedGovernorCap(bool catchup_active,
                                                uint16_t in_band_speed_cap)
{
  return catchup_active ? 255u : in_band_speed_cap;
}

