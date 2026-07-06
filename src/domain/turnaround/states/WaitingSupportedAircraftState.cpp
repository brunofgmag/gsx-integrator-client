#include "WaitingSupportedAircraftState.h"

#include "../TurnaroundContext.h"
#include "../../ports/Aircraft.h"

std::optional<TurnaroundTransition> WaitingSupportedAircraftState::Evaluate(TurnaroundContext& ctx)
{
    if (ctx.aircraft != nullptr)
    {
        return TurnaroundTransition{TurnaroundPhase::WaitingAircraftReady};
    }

    return std::nullopt;
}
