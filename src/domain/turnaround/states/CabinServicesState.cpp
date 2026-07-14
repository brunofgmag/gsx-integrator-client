#include "CabinServicesState.h"

#include "../TurnaroundContext.h"
#include "../../model/AutomationSettings.h"
#include "../../ports/GsxGateway.h"
#include "../../ports/GsxMenuGateway.h"

namespace
{
    constexpr int kExitDelayTicks = 60;
    constexpr int kMaxTriggerAttempts = 3;
    constexpr int kWaitTicks = 30;
    constexpr int kMaxWaitIntervals = 10;
}

std::optional<TurnaroundTransition> CabinServicesState::Evaluate(TurnaroundContext& ctx)
{
    if (ctx.settings == nullptr)
    {
        return TurnaroundTransition{TurnaroundPhase::WaitingNewFlight, kExitDelayTicks};
    }

    if (DispatchNextService(ctx))
    {
        return std::nullopt;
    }

    UpdateActiveSeen(ctx);

    if (AllEnabledCompleted(ctx))
    {
        return TurnaroundTransition{TurnaroundPhase::WaitingNewFlight, kExitDelayTicks};
    }

    if (ctx.TickCondition(kWaitTicks))
    {
        ++ctx.data.cabinWaitIntervals;
        if (ctx.data.cabinWaitIntervals >= kMaxWaitIntervals)
        {
            return TurnaroundTransition{TurnaroundPhase::WaitingNewFlight, kExitDelayTicks};
        }
    }

    return std::nullopt;
}

bool CabinServicesState::DispatchNextService(TurnaroundContext& ctx)
{
    if (ctx.settings->callLavatory && !ctx.data.lavatoryRequested)
    {
        if (ctx.menuGateway->RequestLavatory())
        {
            ctx.data.lavatoryRequested = true;
        }
        else if (++ctx.data.lavatoryTriggerAttempts >= kMaxTriggerAttempts)
        {
            ctx.data.lavatoryRequested = true;
        }

        return true;
    }

    if (ctx.settings->callWater && !ctx.data.waterRequested)
    {
        if (ctx.menuGateway->RequestWater())
        {
            ctx.data.waterRequested = true;
        }
        else if (++ctx.data.waterTriggerAttempts >= kMaxTriggerAttempts)
        {
            ctx.data.waterRequested = true;
        }

        return true;
    }

    if (ctx.settings->callCleaning && !ctx.data.cleaningRequested)
    {
        if (ctx.menuGateway->RequestCleaning())
        {
            ctx.data.cleaningRequested = true;
        }
        else if (++ctx.data.cleaningTriggerAttempts >= kMaxTriggerAttempts)
        {
            ctx.data.cleaningRequested = true;
        }

        return true;
    }

    return false;
}

void CabinServicesState::UpdateActiveSeen(TurnaroundContext& ctx)
{
    if (ctx.gsxGateway->IsServiceInProgress(GroundService::Lavatory))
    {
        ctx.data.lavatoryActiveSeen = true;
    }

    if (ctx.gsxGateway->IsServiceInProgress(GroundService::Water))
    {
        ctx.data.waterActiveSeen = true;
    }

    if (ctx.gsxGateway->IsServiceInProgress(GroundService::Cleaning))
    {
        ctx.data.cleaningActiveSeen = true;
    }
}

bool CabinServicesState::AllEnabledCompleted(const TurnaroundContext& ctx)
{
    return IsServiceDone(ctx, ctx.settings->callLavatory, ctx.data.lavatoryActiveSeen, GroundService::Lavatory)
        && IsServiceDone(ctx, ctx.settings->callWater, ctx.data.waterActiveSeen, GroundService::Water)
        && IsServiceDone(ctx, ctx.settings->callCleaning, ctx.data.cleaningActiveSeen, GroundService::Cleaning);
}

bool CabinServicesState::IsServiceDone(const TurnaroundContext& ctx, const bool enabled,
                                       const bool activeSeen, const GroundService service)
{
    if (!enabled)
    {
        return true;
    }

    return activeSeen && !ctx.gsxGateway->IsServiceInProgress(service);
}
