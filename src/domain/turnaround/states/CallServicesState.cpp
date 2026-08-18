#include "CallServicesState.h"

#include "../TurnaroundContext.h"
#include "../../ports/Aircraft.h"
#include "../../ports/GsxGateway.h"
#include "../../ports/GsxMenuGateway.h"

namespace
{
    constexpr int kRetryTicks = 60;
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

std::optional<TurnaroundTransition> CallServicesState::Evaluate(TurnaroundContext& ctx)
{
    if (!ctx.aircraft->SupportsStairsOrJetways())
    {
        return TurnaroundTransition{TurnaroundPhase::WaitingFlightPlan};
    }

    return ResolveJetwayOrStairs(ctx);
}

std::optional<TurnaroundTransition> CallServicesState::ResolveJetwayOrStairs(TurnaroundContext& ctx)
{
    ctx.data.jetwayOrStairsCompleted = ctx.gsxGateway->AreStairsInPlace() || ctx.gsxGateway->IsJetwayInPlace();
    if (ctx.data.jetwayOrStairsCompleted)
    {
        return TurnaroundTransition{TurnaroundPhase::WaitingFlightPlan};
    }

    if (ctx.gsxGateway->IsJetwayOrStairsOperating())
    {
        return std::nullopt;
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
        if (ctx.data.stateTickCount >= kGiveUpTicks)
        {
            return TurnaroundTransition{TurnaroundPhase::WaitingFlightPlan};
        }

        CallJetwayOrStairs(ctx, jetwayAvailable);
    }

    return std::nullopt;
}
