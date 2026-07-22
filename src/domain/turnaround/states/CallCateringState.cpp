#include "CallCateringState.h"

#include "../TurnaroundContext.h"
#include "../../model/AutomationSettings.h"
#include "../../ports/Aircraft.h"
#include "../../ports/GsxGateway.h"
#include "../../ports/GsxMenuGateway.h"

namespace
{
    constexpr int kCateringRetryTicks = 10;
    constexpr int kMaxCateringAttempts = 3;
}

std::optional<TurnaroundTransition> CallCateringState::Evaluate(TurnaroundContext& ctx)
{
    if (!ctx.menuGateway->IsMenuSettled())
    {
        return std::nullopt;
    }

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
