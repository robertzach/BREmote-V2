#ifndef BREMOTE_FOLLOW_ME_ACTIVATION_H
#define BREMOTE_FOLLOW_ME_ACTIVATION_H

#include <stdint.h>

// The separation proof and the FM lifecycle are deliberately independent of the physical trigger.
// The trigger is applied only at the final automatic-authority gate, so opening it always removes
// motor/steering authority without preventing FM_ARMED -> FM_ACTIVE.
static inline bool followMeSeparationSample(
    bool fault_ok,
    bool position_ok,
    bool min_distance_stop_latched,
    float distance_m,
    float engage_distance_m)
{
  return fault_ok && position_ok && !min_distance_stop_latched &&
      distance_m > engage_distance_m;
}

static inline bool followMeSeparationConfirmed(
    uint32_t separation_since_ms,
    uint32_t now_ms,
    uint32_t dwell_ms)
{
  return separation_since_ms != 0 &&
      (uint32_t)(now_ms - separation_since_ms) >= dwell_ms;
}

static inline bool followMeLifecycleReady(
    bool fault_ok,
    bool return_proof_wait,
    bool separation_latched,
    bool min_distance_stop_latched,
    bool divergence_fault)
{
  return fault_ok && !return_proof_wait && separation_latched &&
      !min_distance_stop_latched && !divergence_fault;
}

static inline bool followMeAutomaticAuthority(
    bool lifecycle_ready,
    bool trigger_held)
{
  return lifecycle_ready && trigger_held;
}

static inline bool followMeActiveLifecycle(
    bool already_active,
    bool lifecycle_ready)
{
  return already_active || lifecycle_ready;
}

#endif
