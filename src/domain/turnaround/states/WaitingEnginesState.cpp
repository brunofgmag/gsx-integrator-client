#include "WaitingEnginesState.h"

#include "../TurnaroundContext.h"
#include "../../ports/Aircraft.h"
#include "../../ports/GsxGateway.h"
#include "../../ports/GsxMenuGateway.h"

std::optional<TurnaroundTransition> WaitingEnginesState::Evaluate(TurnaroundContext& ctx)
{
    if (!ctx.gsxGateway->IsGoodEngineStartConfirmationEnabled() || ctx.gsxGateway->IsPushbackFinished())
    {
        return TurnaroundTransition{TurnaroundPhase::WaitingDeparture};
    }

    if (!ctx.gsxGateway->IsWaitingForEngines()
        || !ctx.aircraft->IsEngineRunning()
        || !ctx.aircraft->IsParkingBrakeSet())
    {
        return std::nullopt;
    }

    if (ctx.ConsumeSmartSwitch() && ctx.menuGateway->ConfirmGoodEngines())
    {
        return TurnaroundTransition{TurnaroundPhase::WaitingDeparture};
    }

    return std::nullopt;
}
