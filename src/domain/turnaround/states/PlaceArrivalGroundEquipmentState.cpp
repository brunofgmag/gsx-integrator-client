#include "PlaceArrivalGroundEquipmentState.h"

#include "../TurnaroundContext.h"
#include "../../model/AutomationSettings.h"
#include "../../ports/Aircraft.h"
#include "../../ports/GsxGateway.h"
#include "../../ports/GsxMenuGateway.h"

std::optional<TurnaroundTransition> PlaceArrivalGroundEquipmentState::EvaluatePhase(TurnaroundContext& ctx)
{
    if (!ctx.data.arrivalDoorsClosed)
    {
        ctx.aircraft->HoldDoorsClosed(false);
        ctx.aircraft->CloseAllDoors();
        ctx.data.arrivalDoorsClosed = true;
    }

    const bool placeChocks = ctx.settings != nullptr && ctx.settings->placeChocksOnArrival;
    const bool callGpu = ctx.settings != nullptr && ctx.settings->callGpuOnArrival;

    if (!placeChocks && !callGpu)
    {
        return TurnaroundTransition{TurnaroundPhase::RequestDeboarding};
    }

    if (ctx.aircraft->IsEngineRunning() || !ctx.aircraft->IsHeldInPlace())
    {
        return std::nullopt;
    }

    if (placeChocks && !ctx.data.arrivalChocksPlaced && ctx.aircraft->SetChocks(true))
    {
        ctx.data.arrivalChocksPlaced = true;
    }

    if (!callGpu)
    {
        return TurnaroundTransition{TurnaroundPhase::RequestDeboarding};
    }

    const GroundPowerStatus gpu =
        ctx.aircraft->GetGroundPowerStatus().value_or(ctx.gsxGateway->GetGpuStatus());

    if (gpu == GroundPowerStatus::Connected || ctx.data.arrivalGpuRequested)
    {
        return TurnaroundTransition{TurnaroundPhase::RequestDeboarding};
    }

    if (gpu == GroundPowerStatus::Unknown)
    {
        return std::nullopt;
    }

    if (ctx.aircraft->SupportsGroundPowerControl())
    {
        ctx.aircraft->SetGroundPower(true);
        ctx.data.arrivalGpuRequested = true;

        return TurnaroundTransition{TurnaroundPhase::RequestDeboarding};
    }

    ctx.menuGateway->ToggleGpu();
    ctx.data.arrivalGpuRequested = true;

    return TurnaroundTransition{TurnaroundPhase::RequestDeboarding};
}
