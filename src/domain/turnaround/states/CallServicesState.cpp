#include "CallServicesState.h"

#include "../TurnaroundContext.h"
#include "../../ports/Aircraft.h"
#include "../../ports/GsxGateway.h"
#include "../../ports/GsxMenuGateway.h"

namespace
{
    constexpr int kRetryTicks = 60;
    constexpr int kStairsInPlaceHoldTicks = 3;
    constexpr int kStalledTicks = 20;
    constexpr int kStuckTicks = 60;
    constexpr int kGiveUpTicks = 240;

    void CallJetwayOrStairs(const TurnaroundContext& ctx, const bool jetwayAvailable)
    {
        if (jetwayAvailable)
        {
            ctx.menuGateway->CallJetway();

            return;
        }

        ctx.menuGateway->CallStairs();
    }
}

std::optional<TurnaroundTransition> CallServicesState::EvaluatePhase(TurnaroundContext& ctx)
{
    if (!ctx.aircraft->SupportsStairsOrJetways())
    {
        return TurnaroundTransition{TurnaroundPhase::WaitingFlightPlan};
    }

    return ResolveJetwayOrStairs(ctx);
}

std::optional<TurnaroundTransition> CallServicesState::ResolveJetwayOrStairs(TurnaroundContext& ctx)
{
    ctx.data.stairsInPlaceTicks = ctx.gsxGateway->AreStairsInPlace() ? ctx.data.stairsInPlaceTicks + 1 : 0;

    ctx.data.jetwayOrStairsCompleted = ctx.data.stairsInPlaceTicks >= kStairsInPlaceHoldTicks
        || ctx.gsxGateway->IsJetwayInPlace();
    if (ctx.data.jetwayOrStairsCompleted)
    {
        ctx.data.servicesStalled = false;

        return TurnaroundTransition{TurnaroundPhase::WaitingFlightPlan};
    }

    if (ctx.gsxGateway->IsJetwayOrStairsOperating())
    {
        if (ctx.gsxGateway->IsServiceVehicleActive())
        {
            ctx.data.servicesOperatingTicks = 0;
            ctx.data.servicesStalled = false;

            return std::nullopt;
        }

        ++ctx.data.servicesOperatingTicks;
        ctx.data.servicesStalled = ctx.data.servicesOperatingTicks >= kStalledTicks;
        ctx.data.servicesWaitSeconds = kStuckTicks - ctx.data.servicesOperatingTicks;

        if (ctx.data.servicesOperatingTicks >= kStuckTicks)
        {
            ctx.data.servicesStalled = false;

            return TurnaroundTransition{TurnaroundPhase::WaitingFlightPlan};
        }

        return std::nullopt;
    }

    ctx.data.servicesOperatingTicks = 0;
    ctx.data.servicesStalled = false;

    if (ctx.data.stateTickCount >= kGiveUpTicks)
    {
        return TurnaroundTransition{TurnaroundPhase::WaitingFlightPlan};
    }

    const bool jetwayAvailable = ctx.gsxGateway->IsJetwayAvailable();

    if (!ctx.data.jetwayOrStairsRequested
        && (jetwayAvailable || ctx.gsxGateway->AreStairsAvailable()))
    {
        CallJetwayOrStairs(ctx, jetwayAvailable);
        ctx.data.jetwayOrStairsRequested = true;

        return std::nullopt;
    }

    if (ctx.TickCondition(kRetryTicks))
    {
        CallJetwayOrStairs(ctx, jetwayAvailable);
    }

    return std::nullopt;
}
