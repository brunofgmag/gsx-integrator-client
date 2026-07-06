#include "CallStairsOrJetwayState.h"

#include "../TurnaroundContext.h"
#include "../../ports/Aircraft.h"
#include "../../ports/GsxGateway.h"
#include "../../ports/GsxMenuGateway.h"

namespace
{
    constexpr int kRetryTicks = 120;
    constexpr int kMaxAttempts = 2;
}

std::optional<TurnaroundTransition> CallStairsOrJetwayState::Evaluate(TurnaroundContext& ctx)
{
    if (!ctx.aircraft->SupportsStairsOrJetways())
    {
        return TurnaroundTransition{TurnaroundPhase::WaitingPowerOn};
    }

    bool& jetwayOrStairsRequested = ctx.data.jetwayOrStairsRequested;
    bool& jetwayOrStairsCompleted = ctx.data.jetwayOrStairsCompleted;

    jetwayOrStairsCompleted = ctx.gsxGateway->AreStairsInPlace() || ctx.gsxGateway->IsJetwayInPlace();
    if (jetwayOrStairsCompleted)
    {
        return TurnaroundTransition{TurnaroundPhase::WaitingPowerOn};
    }

    if (ctx.gsxGateway->IsJetwayAvailable() && !jetwayOrStairsRequested)
    {
        jetwayOrStairsRequested = ctx.menuGateway->CallJetway();
        if (jetwayOrStairsRequested)
        {
            ++ctx.data.jetwayOrStairsAttempts;
        }
        return std::nullopt;
    }

    if (ctx.gsxGateway->AreStairsAvailable() && !jetwayOrStairsRequested)
    {
        jetwayOrStairsRequested = ctx.menuGateway->CallStairs();
        if (jetwayOrStairsRequested)
        {
            ++ctx.data.jetwayOrStairsAttempts;
        }
        return std::nullopt;
    }

    if (ctx.TickCondition(kRetryTicks))
    {
        if (ctx.data.jetwayOrStairsAttempts >= kMaxAttempts)
        {
            return TurnaroundTransition{TurnaroundPhase::WaitingPowerOn};
        }

        jetwayOrStairsRequested = ctx.menuGateway->CallStairs();
        if (jetwayOrStairsRequested)
        {
            ++ctx.data.jetwayOrStairsAttempts;
        }
        return std::nullopt;
    }

    return std::nullopt;
}
