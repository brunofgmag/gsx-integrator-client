#include "WaitingNewFlightState.h"

#include "../TurnaroundContext.h"

std::optional<TurnaroundTransition> WaitingNewFlightState::Evaluate(TurnaroundContext& ctx)
{
    if (ctx.ConsumeSmartSwitch())
    {
        return TurnaroundTransition{TurnaroundPhase::WaitingSupportedAircraft};
    }

    return std::nullopt;
}
