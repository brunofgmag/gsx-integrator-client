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

    const bool viaInterruptMenu = ctx.aircraft->CompletesPushbackViaInterruptMenu();

    if (!ctx.aircraft->IsEngineRunning())
    {
        return std::nullopt;
    }

    if (!viaInterruptMenu
        && (!ctx.gsxGateway->IsWaitingForEngines() || !ctx.aircraft->IsParkingBrakeSet()))
    {
        return std::nullopt;
    }

    if (ctx.ConsumeSmartSwitch()
        && (viaInterruptMenu ? ctx.menuGateway->CompletePushback() : ctx.menuGateway->ConfirmGoodEngines()))
    {
        return TurnaroundTransition{TurnaroundPhase::WaitingDeparture};
    }

    return std::nullopt;
}
