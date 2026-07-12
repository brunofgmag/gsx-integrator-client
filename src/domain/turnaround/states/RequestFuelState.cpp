#include "RequestFuelState.h"

#include "../TurnaroundContext.h"
#include "../../model/AutomationSettings.h"
#include "../../ports/Aircraft.h"
#include "../../ports/GsxGateway.h"
#include "../../ports/GsxMenuGateway.h"

namespace
{
    constexpr int kRetryTicks = 60;
}

std::optional<TurnaroundTransition> RequestFuelState::Evaluate(TurnaroundContext& ctx)
{
    auto& data = ctx.data;

    if (ctx.ConsumeSmartSwitch())
    {
        data.loadingConfirmed = true;
    }

    const bool loadingAllowed = ctx.settings == nullptr || ctx.settings->autoStartLoading || data.loadingConfirmed;

    const GsxStateStatus refuelingState = ctx.gsxGateway->GetStateStatus(GsxState::Refueling);
    if (loadingAllowed && refuelingState == GsxStateStatus::Callable && !data.refuelingRequested)
    {
        ctx.gsxGateway->TakeOverFuelAndPayload();
        data.refuelingRequested = ctx.menuGateway->RequestRefueling();
    }

    if (ctx.gsxGateway->IsFuelHoseConnected() && refuelingState == GsxStateStatus::Active)
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
