#include "WaitingAircraftReadyState.h"

#include "../TurnaroundContext.h"
#include "../../ports/Aircraft.h"

std::optional<TurnaroundTransition> WaitingAircraftReadyState::Evaluate(TurnaroundContext& ctx)
{
    if (ctx.aircraft->IsEngineRunning())
    {
        return std::nullopt;
    }

    return TurnaroundTransition{TurnaroundPhase::RepositionAircraft};
}
