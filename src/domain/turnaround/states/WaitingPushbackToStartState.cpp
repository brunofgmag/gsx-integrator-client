#include "WaitingPushbackToStartState.h"

#include "../TurnaroundContext.h"
#include "../../ports/GsxGateway.h"

std::optional<TurnaroundTransition> WaitingPushbackToStartState::Evaluate(TurnaroundContext& ctx)
{
    if (ctx.gsxGateway->IsPushbackFinished() || ctx.gsxGateway->WasStateCompleted(GsxState::Pushback))
    {
        return TurnaroundTransition{TurnaroundPhase::WaitingDeparture};
    }

    if (ctx.gsxGateway->HasPushbackStarted())
    {
        return TurnaroundTransition{TurnaroundPhase::WaitingForEngines};
    }

    return std::nullopt;
}
