#include "CallServicesState.h"

#include "../TurnaroundContext.h"
#include "../../model/AutomationSettings.h"
#include "../../ports/Aircraft.h"
#include "../../ports/GsxGateway.h"
#include "../../ports/GsxMenuGateway.h"

namespace
{
    constexpr int kRetryTicks = 120;
    constexpr int kMaxAttempts = 2;
}

std::optional<TurnaroundTransition> CallServicesState::Evaluate(TurnaroundContext& ctx)
{
    if (RequestNextGroundService(ctx))
    {
        return std::nullopt;
    }

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

bool CallServicesState::RequestNextGroundService(TurnaroundContext& ctx)
{
    if (ctx.settings == nullptr)
    {
        return false;
    }

    if (ctx.settings->callGpu && !ctx.data.gpuRequested && !ctx.gsxGateway->IsGpuConnected())
    {
        ctx.data.gpuRequested = ctx.menuGateway->ToggleGpu();
        return ctx.data.gpuRequested;
    }

    if (ctx.settings->callCatering && !ctx.data.cateringRequested)
    {
        ctx.data.cateringRequested = ctx.menuGateway->RequestCatering();
        return ctx.data.cateringRequested;
    }

    return false;
}
