#include "CallCateringState.h"

#include "../TurnaroundContext.h"
#include "../../model/AutomationSettings.h"
#include "../../ports/Aircraft.h"
#include "../../ports/GsxGateway.h"
#include "../../ports/GsxMenuGateway.h"

namespace
{
    constexpr int kCateringGiveUpTicks = 30;
}

std::optional<TurnaroundTransition> CallCateringState::EvaluatePhase(TurnaroundContext& ctx)
{
    if (RequestNextGroundService(ctx))
    {
        return std::nullopt;
    }

    return TurnaroundTransition{TurnaroundPhase::RequestFuel};
}

bool CallCateringState::RequestNextGroundService(TurnaroundContext& ctx)
{
    if (ctx.settings == nullptr)
    {
        return false;
    }

    if (ctx.settings->callCatering && !ctx.aircraft->IsCargoVariant() && !ctx.data.cateringRequested)
    {
        return DispatchCatering(ctx);
    }

    return false;
}

bool CallCateringState::DispatchCatering(TurnaroundContext& ctx)
{
    if (ctx.gsxGateway->IsServiceInProgress(GroundService::Catering))
    {
        ctx.data.cateringRequested = true;
        return false;
    }

    if (!ctx.data.cateringAsked)
    {
        ctx.menuGateway->RequestCatering();
        ctx.data.cateringAsked = true;

        return true;
    }

    if (ctx.TickCondition(kCateringGiveUpTicks))
    {
        ctx.data.cateringRequested = true;
        return false;
    }

    return true;
}
