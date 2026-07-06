#include "RequestPushbackState.h"

#include "../TurnaroundContext.h"
#include "../../ports/GsxGateway.h"
#include "../../ports/GsxMenuGateway.h"

namespace
{
    constexpr int kRetryTicks = 60;
}

std::optional<TurnaroundTransition> RequestPushbackState::Evaluate(TurnaroundContext& ctx)
{
    auto& data = ctx.data;

    const GsxStateStatus departureState = ctx.gsxGateway->GetStateStatus(GsxState::Pushback);
    if (departureState == GsxStateStatus::Callable && !data.pushbackRequested)
    {
        data.pushbackRequested = ctx.menuGateway->RequestPushback();
    }

    if (departureState == GsxStateStatus::Requested || departureState == GsxStateStatus::Active)
    {
        return TurnaroundTransition{TurnaroundPhase::WaitingPushbackToStart};
    }

    if (departureState == GsxStateStatus::Completed || ctx.gsxGateway->WasStateCompleted(GsxState::Pushback))
    {
        return TurnaroundTransition{TurnaroundPhase::WaitingPushbackToStart};
    }

    if (departureState == GsxStateStatus::Callable && data.pushbackRequested && ctx.TickCondition(kRetryTicks))
    {
        data.pushbackRequested = false;
    }

    return std::nullopt;
}
