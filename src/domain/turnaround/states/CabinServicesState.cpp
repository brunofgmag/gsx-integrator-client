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
    constexpr int kMaxWaitIntervals = 30;
}

std::optional<TurnaroundTransition> CabinServicesState::EvaluatePhase(TurnaroundContext& ctx)
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
    return DispatchService(ctx, ctx.settings->callLavatory, ctx.data.lavatory, GroundService::Lavatory)
        || DispatchService(ctx, ctx.settings->callWater, ctx.data.water, GroundService::Water)
        || DispatchService(ctx, ctx.settings->callCleaning, ctx.data.cleaning, GroundService::Cleaning);
}

bool CabinServicesState::DispatchService(const TurnaroundContext& ctx,
                                         const bool enabled,
                                         CabinServiceProgress& progress,
                                         const GroundService service)
{
    if (!enabled || progress.requested)
    {
        return false;
    }

    if (ctx.gsxGateway->IsServiceInProgress(service))
    {
        progress.activeSeen = true;
        progress.requested = true;

        return true;
    }

    if (!progress.asked)
    {
        SendServiceTrigger(ctx, service);
        progress.asked = true;

        return true;
    }

    if (ctx.TickCondition(kServiceGiveUpTicks))
    {
        progress.requested = true;
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
        ctx.data.lavatory.activeSeen = true;
    }

    if (ctx.gsxGateway->IsServiceInProgress(GroundService::Water))
    {
        ctx.data.water.activeSeen = true;
    }

    if (ctx.gsxGateway->IsServiceInProgress(GroundService::Cleaning))
    {
        ctx.data.cleaning.activeSeen = true;
    }
}

bool CabinServicesState::AllEnabledCompleted(const TurnaroundContext& ctx)
{
    return IsServiceDone(ctx, ctx.data.lavatory, GroundService::Lavatory)
        && IsServiceDone(ctx, ctx.data.water, GroundService::Water)
        && IsServiceDone(ctx, ctx.data.cleaning, GroundService::Cleaning);
}

bool CabinServicesState::IsServiceDone(const TurnaroundContext& ctx,
                                       const CabinServiceProgress& progress, const GroundService service)
{
    if (progress.activeSeen)
    {
        return !ctx.gsxGateway->IsServiceInProgress(service);
    }

    return true;
}
