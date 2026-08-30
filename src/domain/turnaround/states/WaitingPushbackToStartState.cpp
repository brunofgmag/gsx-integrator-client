#include "WaitingPushbackToStartState.h"

#include "../TurnaroundContext.h"
#include "../../ports/GsxGateway.h"
#include "../../ports/GsxMenuGateway.h"

std::optional<TurnaroundTransition> WaitingPushbackToStartState::EvaluatePhase(TurnaroundContext& ctx)
{
    if (ctx.gsxGateway->IsPushbackFinished() || ctx.gsxGateway->WasStateCompleted(GsxState::Pushback))
    {
        return TurnaroundTransition{TurnaroundPhase::WaitingDeparture};
    }

    if (ctx.gsxGateway->HasPushbackStarted())
    {
        return TurnaroundTransition{TurnaroundPhase::WaitingForEngines};
    }

    ctx.menuGateway->OpenPushbackPanel();

    return std::nullopt;
}
