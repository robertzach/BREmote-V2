#ifndef BREMOTE_FOLLOW_ME_FAULT_TELEMETRY_H
#define BREMOTE_FOLLOW_ME_FAULT_TELEMETRY_H

#include <stdint.h>

// Stable fm_flags packet bits. During FM_STOPPING, ARMED means the RX will preserve the live F1-F6
// declaration and finish in FM_ARMED; FAULT without ARMED means the stop is terminal. Keeping this
// decoding in a native-testable helper protects the TX from accidentally treating a recoverable
// stop as F0, or a terminal stop as an automatic recovery.
static const uint8_t kFollowMeTelemetryArmed = 0x01u;
static const uint8_t kFollowMeTelemetryFault = 0x08u;

static inline bool followMeRecoverableFaultEdge(uint8_t current_flags,
                                                uint8_t previous_flags)
{
  bool fault_now  = (current_flags & kFollowMeTelemetryFault) != 0;
  bool armed_now  = (current_flags & kFollowMeTelemetryArmed) != 0;
  bool fault_prev = (previous_flags & kFollowMeTelemetryFault) != 0;
  return fault_now && armed_now && !fault_prev;
}

static inline bool followMeTerminalFaultEdge(uint8_t current_flags,
                                             uint8_t previous_flags)
{
  bool fault_now  = (current_flags & kFollowMeTelemetryFault) != 0;
  bool armed_now  = (current_flags & kFollowMeTelemetryArmed) != 0;
  bool fault_prev = (previous_flags & kFollowMeTelemetryFault) != 0;
  bool armed_prev = (previous_flags & kFollowMeTelemetryArmed) != 0;

  // The armed->not-armed clause catches escalation from a recoverable stop while the 6 s FAULT
  // notification is already sticky, so the terminal event cannot be lost for lack of a new edge.
  return fault_now && !armed_now && (!fault_prev || armed_prev);
}

#endif
