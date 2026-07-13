#include "WaitingReadyToPushState.h"

#include "../TurnaroundContext.h"
#include "../../ports/Aircraft.h"

std::optional<TurnaroundTransition> WaitingReadyToPushState::Evaluate(TurnaroundContext& ctx)
{
    if (ctx.aircraft->IsReadyToPush())
    {
        return TurnaroundTransition{TurnaroundPhase::DisconnectGpu};
    }

    return std::nullopt;
}
