#include "CabinServicesState.h"

#include "../TurnaroundContext.h"
#include "../../model/AutomationSettings.h"
#include "../../ports/GsxGateway.h"
#include "../../ports/GsxMenuGateway.h"

namespace
{
    constexpr int kExitDelayTicks = 60;
    constexpr int kServiceGiveUpTicks = 30;
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
    return DispatchService(ctx,
                           ctx.settings->callLavatory,
                           ctx.data.lavatoryAsked,
                           ctx.data.lavatoryRequested,
                           ctx.data.lavatoryActiveSeen,
                           GroundService::Lavatory)
        || DispatchService(ctx,
                           ctx.settings->callWater,
                           ctx.data.waterAsked,
                           ctx.data.waterRequested,
                           ctx.data.waterActiveSeen,
                           GroundService::Water)
        || DispatchService(ctx,
                           ctx.settings->callCleaning,
                           ctx.data.cleaningAsked,
                           ctx.data.cleaningRequested,
                           ctx.data.cleaningActiveSeen,
                           GroundService::Cleaning);
}

bool CabinServicesState::DispatchService(const TurnaroundContext& ctx,
                                         const bool enabled,
                                         bool& asked,
                                         bool& requested,
                                         bool& activeSeen,
                                         const GroundService service)
{
    if (!enabled || requested)
    {
        return false;
    }

    if (ctx.gsxGateway->IsServiceInProgress(service))
    {
        activeSeen = true;
        requested = true;

        return true;
    }

    if (!asked)
    {
        SendServiceTrigger(ctx, service);
        asked = true;

        return true;
    }

    if (ctx.TickCondition(kServiceGiveUpTicks))
    {
        requested = true;
    }

    return true;
}

void CabinServicesState::SendServiceTrigger(const TurnaroundContext& ctx, const GroundService service)
{
    switch (service)
    {
    case GroundService::Lavatory:
        ctx.menuGateway->RequestLavatory();
        break;
    case GroundService::Water:
        ctx.menuGateway->RequestWater();
        break;
    case GroundService::Cleaning:
        ctx.menuGateway->RequestCleaning();
        break;
    default:
        break;
    }
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
    return IsServiceDone(ctx, ctx.data.lavatoryActiveSeen, GroundService::Lavatory)
        && IsServiceDone(ctx, ctx.data.waterActiveSeen, GroundService::Water)
        && IsServiceDone(ctx, ctx.data.cleaningActiveSeen, GroundService::Cleaning);
}

bool CabinServicesState::IsServiceDone(const TurnaroundContext& ctx,
                                       const bool activeSeen, const GroundService service)
{
    if (activeSeen)
    {
        return !ctx.gsxGateway->IsServiceInProgress(service);
    }

    return true;
}
