#include "CallServicesState.h"

#include "../TurnaroundContext.h"
#include "../../ports/Aircraft.h"
#include "../../ports/GsxGateway.h"
#include "../../ports/GsxMenuGateway.h"

namespace
{
    constexpr int kRetryTicks = 20;
    constexpr int kMaxAttempts = 4;

    bool CallJetwayOrStairs(const TurnaroundContext& ctx, const bool jetwayAvailable)
    {
        return jetwayAvailable ? ctx.menuGateway->CallJetway() : ctx.menuGateway->CallStairs();
    }
}

std::optional<TurnaroundTransition> CallServicesState::Evaluate(TurnaroundContext& ctx)
{
    if (!ctx.menuGateway->IsMenuSettled())
    {
        return std::nullopt;
    }

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
        RegisterJetwayOrStairsRequest(ctx, CallJetwayOrStairs(ctx, jetwayAvailable));
        return std::nullopt;
    }

    if (ctx.TickCondition(kRetryTicks))
    {
        if (ctx.data.jetwayOrStairsAttempts >= kMaxAttempts)
        {
            return TurnaroundTransition{TurnaroundPhase::WaitingFlightPlan};
        }

        RegisterJetwayOrStairsRequest(ctx, CallJetwayOrStairs(ctx, jetwayAvailable));
    }

    return std::nullopt;
}

void CallServicesState::RegisterJetwayOrStairsRequest(TurnaroundContext& ctx, const bool requested)
{
    ctx.data.jetwayOrStairsRequested = requested;
    if (requested)
    {
        ++ctx.data.jetwayOrStairsAttempts;
    }
}
