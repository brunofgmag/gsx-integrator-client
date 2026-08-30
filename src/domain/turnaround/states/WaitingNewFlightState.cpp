#include "WaitingNewFlightState.h"

#include "../TurnaroundContext.h"

std::optional<TurnaroundTransition> WaitingNewFlightState::EvaluatePhase(TurnaroundContext& ctx)
{
    if (ctx.ConsumePilotTouch())
    {
        return TurnaroundTransition{TurnaroundPhase::WaitingSupportedAircraft};
    }

    return std::nullopt;
}
