#include "WaitCateringState.h"

#include "../TurnaroundContext.h"
#include "../../ports/GsxGateway.h"

namespace
{
    constexpr int kWaitTicks = 30;
    constexpr int kMaxWaitIntervals = 5;
}

std::optional<TurnaroundTransition> WaitCateringState::Evaluate(TurnaroundContext& ctx)
{
    const bool waitCatering = ctx.data.cateringRequested;

    if (!waitCatering || !ctx.gsxGateway->IsServiceInProgress(GroundService::Catering))
    {
        return TurnaroundTransition{TurnaroundPhase::RemoveGroundEquipment};
    }

    if (ctx.TickCondition(kWaitTicks))
    {
        ++ctx.data.cateringWaitIntervals;
        if (ctx.data.cateringWaitIntervals >= kMaxWaitIntervals)
        {
            return TurnaroundTransition{TurnaroundPhase::RemoveGroundEquipment};
        }
    }

    return std::nullopt;
}
