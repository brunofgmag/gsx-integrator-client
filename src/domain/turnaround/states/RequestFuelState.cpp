#include "RequestFuelState.h"

#include "../TurnaroundContext.h"
#include "../../ports/GsxGateway.h"
#include "../../ports/GsxMenuGateway.h"

namespace
{
    constexpr int kRetryTicks = 60;
}

std::optional<TurnaroundTransition> RequestFuelState::Evaluate(TurnaroundContext& ctx)
{
    auto& data = ctx.data;

    const GsxStateStatus refuelingState = ctx.gsxGateway->GetStateStatus(GsxState::Refueling);
    if (refuelingState == GsxStateStatus::Callable && !data.refuelingRequested)
    {
        ctx.gsxGateway->TakeOverFuelAndPayload();
        data.refuelingRequested = ctx.menuGateway->RequestRefueling();
    }

    const bool gsxReady =
        ctx.gsxGateway->IsFuelHoseConnected() && refuelingState == GsxStateStatus::Active;
    if (gsxReady)
    {
        return TurnaroundTransition{TurnaroundPhase::Refueling};
    }

    if (refuelingState == GsxStateStatus::Completed || ctx.gsxGateway->WasStateCompleted(GsxState::Refueling))
    {
        return TurnaroundTransition{TurnaroundPhase::Refueling};
    }

    if (refuelingState == GsxStateStatus::Callable && data.refuelingRequested && ctx.TickCondition(kRetryTicks))
    {
        data.refuelingRequested = false;
    }

    return std::nullopt;
}
