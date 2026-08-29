#ifndef BREMOTE_FOLLOW_ME_ACTIVATION_H
#define BREMOTE_FOLLOW_ME_ACTIVATION_H

#include <stdint.h>

// The radio failsafe is an authority hold, not an FM lifecycle transition. Match PWM.ino's strict
// boundary exactly: at age == failsafe_ms the PWM task has already stopped emitting motor pulses.
// Requiring a non-zero timestamp also keeps startup fail-closed before the first valid packet.
static inline bool followMeLinkHealthy(
    uint32_t last_packet_ms,
    uint32_t now_ms,
    uint32_t failsafe_ms)
{
  return last_packet_ms != 0 &&
      (uint32_t)(now_ms - last_packet_ms) < failsafe_ms;
}

// A recovered control packet must not expose a stale pre-outage TX position. Wait for one newly
// decoded GPS meta-packet before leaving the link hold; the caller bounds this wait separately.
static inline bool followMeTxGpsRefreshedAfterLinkHold(
    uint32_t tx_gps_ms_at_hold,
    uint32_t current_tx_gps_ms)
{
  return current_tx_gps_ms != 0 && current_tx_gps_ms != tx_gps_ms_at_hold;
}

static inline bool followMeLinkRecoveryShouldHold(
    bool tx_gps_refreshed,
    uint32_t recovery_start_ms,
    uint32_t now_ms,
    uint32_t grace_ms)
{
  return !tx_gps_refreshed &&
      (uint32_t)(now_ms - recovery_start_ms) < grace_ms;
}

struct FollowMeLinkHoldPolicy {
  uint8_t lifecycle_state;
  bool automatic_authority;
  uint8_t throttle_cap;
};

// A link timeout changes outputs, never the lifecycle. Keeping this tiny policy in the native-test
// surface makes ARMED/ACTIVE/RETURN preservation an explicit contract rather than a side effect of
// a branch that happens not to assign the state.
static inline FollowMeLinkHoldPolicy followMeLinkHold(uint8_t current_state)
{
  FollowMeLinkHoldPolicy policy = { current_state, false, 0 };
  return policy;
}

// The separation proof and the FM lifecycle are deliberately independent of the physical trigger.
// The trigger and live link are applied only at the final automatic-authority gate, so opening the
// trigger always removes motor/steering authority without preventing FM_ARMED -> FM_ACTIVE.
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
    bool trigger_held,
    bool link_ok)
{
  return lifecycle_ready && trigger_held && link_ok;
}

static inline bool followMeActiveLifecycle(
    bool already_active,
    bool lifecycle_ready)
{
  return already_active || lifecycle_ready;
}

// Once an ACTIVE lifecycle has proven that the rider stopped, always route through FM_RETURN so
// the shared RETURN exit clears every separation/stop/controller latch. Distance only gates a
// stationary ARMED declaration: it may start a real retrieval only from outside D_engage. An ACTIVE
// session may start the same proof at any trustworthy distance; at/below D_engage it immediately
// satisfies followMeReturnArrived() and normalises to FM_ARMED without commanding return motion.
static inline bool followMeReturnCandidate(
    bool armed_session,
    bool active_session,
    bool position_ok,
    float distance_m,
    float engage_distance_m,
    bool rider_speed_ok,
    float rider_speed_kmh,
    float stationary_speed_kmh)
{
  return (armed_session || active_session) && position_ok && rider_speed_ok &&
      rider_speed_kmh < stationary_speed_kmh &&
      (active_session || distance_m > engage_distance_m);
}

// Complement the strict outside-entry test at the boundary. This prevents an ACTIVE stationary
// normalisation at exactly D_engage from lingering in FM_RETURN or receiving return authority.
static inline bool followMeReturnArrived(
    bool position_ok,
    float distance_m,
    float engage_distance_m)
{
  return position_ok && distance_m <= engage_distance_m;
}

#endif
