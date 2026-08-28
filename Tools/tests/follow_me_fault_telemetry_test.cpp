#include <assert.h>
#include <stdint.h>

#include "../../Source/Common/FollowMeFaultTelemetry.h"

int main()
{
  const uint8_t armed = kFollowMeTelemetryArmed;
  const uint8_t fault = kFollowMeTelemetryFault;

  // A fault that retains ARMED is recoverable and is reported only on its first sticky edge.
  assert(followMeRecoverableFaultEdge(armed | fault, armed));
  assert(!followMeTerminalFaultEdge(armed | fault, armed));
  assert(!followMeRecoverableFaultEdge(armed | fault, armed | fault));

  // FAULT without ARMED is terminal, including an ordinary first packet from older RX firmware.
  assert(followMeTerminalFaultEdge(fault, armed));
  assert(!followMeRecoverableFaultEdge(fault, armed));
  assert(!followMeTerminalFaultEdge(fault, fault));

  // Escalation cannot hide inside the 6 s sticky window: recoverable -> terminal is a new edge even
  // though FAULT never fell back to zero between the two events.
  assert(followMeTerminalFaultEdge(fault, armed | fault));

  // Unrelated flags and plain armed-state changes never manufacture a fault event.
  assert(!followMeRecoverableFaultEdge(armed, 0));
  assert(!followMeTerminalFaultEdge(0, armed | fault));
  assert(!followMeTerminalFaultEdge(armed, fault));

  return 0;
}
