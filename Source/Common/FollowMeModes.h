#pragma once

#include <stdint.h>

static inline bool followMeIsActiveMode(uint8_t mode)
{
  return mode >= 1 && mode <= 6;
}

static inline bool followMeIsFrontMode(uint8_t mode)
{
  return mode >= 4 && mode <= 6;
}

// The standalone RTM gesture formerly occupied the reverse direction. Only the live FM arm/disarm
// combination may take ownership away from the armed LEFT/RIGHT selector.
static inline bool followMeArmComboPending(int last_tap_direction,
                                           int hold_direction,
                                           uint32_t elapsed_ms,
                                           uint32_t combo_window_ms)
{
  return last_tap_direction == -1 &&
         hold_direction > 0 &&
         elapsed_ms < combo_window_ms;
}

// Step only through active modes. F0 is an explicit disarm command and must never be reached by
// the armed hold-to-repeat selector. LEFT/negative walks backwards; RIGHT/positive walks forwards.
static inline uint8_t followMeStepActiveMode(uint8_t current_mode, int direction)
{
  if (!followMeIsActiveMode(current_mode)) return 1;
  if (direction < 0) return (current_mode > 1) ? current_mode - 1 : 6;
  if (direction > 0) return (current_mode < 6) ? current_mode + 1 : 1;
  return current_mode;
}
