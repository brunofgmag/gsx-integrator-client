#include "PlaceArrivalGroundEquipmentState.h"

#include "../TurnaroundContext.h"
#include "../../model/AutomationSettings.h"
#include "../../ports/Aircraft.h"
#include "../../ports/GsxGateway.h"
#include "../../ports/GsxMenuGateway.h"

std::optional<TurnaroundTransition> PlaceArrivalGroundEquipmentState::Evaluate(TurnaroundContext& ctx)
{
    if (!ctx.data.arrivalDoorsClosed)
    {
        ctx.aircraft->CloseAllDoors();
        ctx.data.arrivalDoorsClosed = true;
    }

    if (ctx.settings == nullptr || !ctx.settings->callGpuOnArrival)
    {
        return TurnaroundTransition{TurnaroundPhase::RequestDeboarding};
    }

    if (ctx.aircraft->IsEngineRunning() || !ctx.aircraft->IsParkingBrakeSet())
    {
        return std::nullopt;
    }

    if (ctx.aircraft->SupportsChocksControl() && !ctx.data.arrivalChocksPlaced)
    {
        ctx.aircraft->SetChocks(true);
        ctx.data.arrivalChocksPlaced = true;
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

    if (!ctx.menuGateway->IsMenuSettled())
    {
        return std::nullopt;
    }

    if (ctx.menuGateway->ToggleGpu())
    {
        ctx.data.arrivalGpuRequested = true;
        return TurnaroundTransition{TurnaroundPhase::RequestDeboarding};
    }

    return std::nullopt;
}
