#include "WaitCateringState.h"

#include "../TurnaroundContext.h"
#include "../../model/AutomationSettings.h"
#include "../../ports/GsxGateway.h"

namespace
{
    constexpr int kWaitTicks = 30;
    constexpr int kMaxWaitIntervals = 5;
}

std::optional<TurnaroundTransition> WaitCateringState::Evaluate(TurnaroundContext& ctx)
{
    const bool waitCatering = ctx.settings != nullptr && ctx.settings->callCatering == true;

    if (!waitCatering || !ctx.gsxGateway->IsServiceInProgress(GroundService::Catering))
    {
        return TurnaroundTransition{TurnaroundPhase::DisconnectGpu};
    }

    if (ctx.TickCondition(kWaitTicks))
    {
        ++ctx.data.cateringWaitIntervals;
        if (ctx.data.cateringWaitIntervals >= kMaxWaitIntervals)
        {
            return TurnaroundTransition{TurnaroundPhase::DisconnectGpu};
        }
    }

    return std::nullopt;
}
