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
    constexpr int kCateringRetryTicks = 10;
    constexpr int kMaxCateringAttempts = 3;
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

    return ResolveJetwayOrStairs(ctx);
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

    if (ctx.settings->callCatering && !ctx.aircraft->IsCargoVariant() && !ctx.data.cateringRequested)
    {
        return DispatchCatering(ctx);
    }

    return false;
}

bool CallServicesState::DispatchCatering(TurnaroundContext& ctx)
{
    if (ctx.gsxGateway->IsServiceInProgress(GroundService::Catering))
    {
        ctx.data.cateringRequested = true;
        return false;
    }

    if (ctx.data.cateringAttempts == 0 || ctx.TickCondition(kCateringRetryTicks))
    {
        if (ctx.data.cateringAttempts >= kMaxCateringAttempts)
        {
            ctx.data.cateringRequested = true;
            return false;
        }

        static_cast<void>(ctx.menuGateway->RequestCatering());
        ++ctx.data.cateringAttempts;
    }

    return true;
}

std::optional<TurnaroundTransition> CallServicesState::ResolveJetwayOrStairs(TurnaroundContext& ctx)
{
    ctx.data.jetwayOrStairsCompleted = ctx.gsxGateway->AreStairsInPlace() || ctx.gsxGateway->IsJetwayInPlace();
    if (ctx.data.jetwayOrStairsCompleted)
    {
        return TurnaroundTransition{TurnaroundPhase::WaitingPowerOn};
    }

    if (ctx.gsxGateway->IsJetwayAvailable() && !ctx.data.jetwayOrStairsRequested)
    {
        RegisterJetwayOrStairsRequest(ctx, ctx.menuGateway->CallJetway());
        return std::nullopt;
    }

    if (ctx.gsxGateway->AreStairsAvailable() && !ctx.data.jetwayOrStairsRequested)
    {
        RegisterJetwayOrStairsRequest(ctx, ctx.menuGateway->CallStairs());
        return std::nullopt;
    }

    if (ctx.TickCondition(kRetryTicks))
    {
        if (ctx.data.jetwayOrStairsAttempts >= kMaxAttempts)
        {
            return TurnaroundTransition{TurnaroundPhase::WaitingPowerOn};
        }

        RegisterJetwayOrStairsRequest(ctx, ctx.menuGateway->CallStairs());
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
