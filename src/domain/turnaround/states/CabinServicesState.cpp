#include "CabinServicesState.h"

#include "../TurnaroundContext.h"
#include "../../model/AutomationSettings.h"
#include "../../ports/GsxMenuGateway.h"

std::optional<TurnaroundTransition> CabinServicesState::Evaluate(TurnaroundContext& ctx)
{
    if (RequestNextService(ctx))
    {
        return std::nullopt;
    }

    return TurnaroundTransition{TurnaroundPhase::WaitingNewFlight, 60};
}

bool CabinServicesState::RequestNextService(TurnaroundContext& ctx)
{
    if (ctx.settings == nullptr)
    {
        return false;
    }

    if (ctx.settings->callLavatory && !ctx.data.lavatoryRequested)
    {
        ctx.data.lavatoryRequested = ctx.menuGateway->RequestLavatory();
        return ctx.data.lavatoryRequested;
    }

    if (ctx.settings->callWater && !ctx.data.waterRequested)
    {
        ctx.data.waterRequested = ctx.menuGateway->RequestWater();
        return ctx.data.waterRequested;
    }

    if (ctx.settings->callCleaning && !ctx.data.cleaningRequested)
    {
        ctx.data.cleaningRequested = ctx.menuGateway->RequestCleaning();
        return ctx.data.cleaningRequested;
    }

    return false;
}
