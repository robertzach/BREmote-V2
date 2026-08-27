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

// Step only through active modes. F0 is an explicit disarm command and must never be reached by
// the armed hold-to-repeat selector. LEFT/negative walks backwards; RIGHT/positive walks forwards.
static inline uint8_t followMeStepActiveMode(uint8_t current_mode, int direction)
{
  if (!followMeIsActiveMode(current_mode)) return 1;
  if (direction < 0) return (current_mode > 1) ? current_mode - 1 : 6;
  if (direction > 0) return (current_mode < 6) ? current_mode + 1 : 1;
  return current_mode;
}
