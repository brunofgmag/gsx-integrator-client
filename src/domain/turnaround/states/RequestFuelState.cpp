#include "RequestFuelState.h"

#include "../TurnaroundContext.h"
#include "../../model/AutomationSettings.h"
#include "../../ports/Aircraft.h"
#include "../../ports/GsxGateway.h"
#include "../../ports/GsxMenuGateway.h"

namespace
{
    constexpr int kRetryTicks = 60;
    constexpr int kStalledRequestTicks = 600;
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
        ctx.menuGateway->RequestRefueling();
        data.refuelingRequested = true;
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

    TrackStalledRequest(ctx, refuelingState == GsxStateStatus::Callable);

    return std::nullopt;
}

void RequestFuelState::TrackStalledRequest(TurnaroundContext& ctx, const bool gsxStillOffersService)
{
    auto& data = ctx.data;

    if (!data.refuelingRequested || gsxStillOffersService)
    {
        data.fuelRequestStallTicks = 0;
        data.fuelRequestStalled = false;

        return;
    }

    data.fuelRequestStalled = ++data.fuelRequestStallTicks >= kStalledRequestTicks;
}
