#include "RequestFuelState.h"

#include <format>

#include "../TurnaroundContext.h"
#include "../../model/AutomationSettings.h"
#include "../../ports/Aircraft.h"
#include "../../ports/DomainLogger.h"
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

    if (ctx.ConsumePilotTouch())
    {
        data.loadingConfirmed = true;
    }

    const bool loadingAllowed = ctx.settings == nullptr || ctx.settings->autoStartLoading || data.loadingConfirmed;

    const GsxStateStatus refuelingState = ctx.gsxGateway->GetStateStatus(GsxState::Refueling);
    if (loadingAllowed && refuelingState == GsxStateStatus::Callable && !data.refuelingRequested)
    {
        WarnWhenPlanExceedsCapacity(ctx);
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

void RequestFuelState::WarnWhenPlanExceedsCapacity(TurnaroundContext& ctx)
{
    auto& data = ctx.data;

    const double capacityKg = ctx.aircraft->GetFuelCapacityKg();
    data.fuelPlanOverCapacity = capacityKg > 0.0 && data.plannedFuelKg > capacityKg + 1.0;

    if (data.fuelPlanOverCapacity)
    {
        ctx.logger->LogInfo(std::format(
            "The plan asks for {:.0f} kg of fuel but this airframe holds {:.0f} kg; the tanks stop at capacity",
            data.plannedFuelKg, capacityKg));
    }
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
