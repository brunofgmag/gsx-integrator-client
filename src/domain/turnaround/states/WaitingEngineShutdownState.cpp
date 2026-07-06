#include "WaitingEngineShutdownState.h"

#include "../TurnaroundContext.h"
#include "../../ports/Aircraft.h"

std::optional<TurnaroundTransition> WaitingEngineShutdownState::Evaluate(TurnaroundContext& ctx)
{
    if (ctx.aircraft->IsReadyToDeboard())
    {
        return TurnaroundTransition{TurnaroundPhase::RequestDeboarding};
    }

    return std::nullopt;
}
