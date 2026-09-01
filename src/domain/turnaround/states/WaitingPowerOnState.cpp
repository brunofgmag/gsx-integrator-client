#include "WaitingPowerOnState.h"

#include "../TurnaroundContext.h"
#include "../../ports/Aircraft.h"

std::optional<TurnaroundTransition> WaitingPowerOnState::EvaluatePhase(TurnaroundContext& ctx)
{
    if (ctx.aircraft->IsPowered())
    {
        return TurnaroundTransition{TurnaroundPhase::CallCatering};
    }

    return std::nullopt;
}
