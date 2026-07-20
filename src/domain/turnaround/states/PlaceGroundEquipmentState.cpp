#include "PlaceGroundEquipmentState.h"

#include "../TurnaroundContext.h"
#include "../../model/AutomationSettings.h"
#include "../../ports/Aircraft.h"
#include "../../ports/GsxGateway.h"
#include "../../ports/GsxMenuGateway.h"

std::optional<TurnaroundTransition> PlaceGroundEquipmentState::Evaluate(TurnaroundContext& ctx)
{
    if (!ctx.data.doorsClosed)
    {
        ctx.aircraft->CloseAllDoors();
        ctx.data.doorsClosed = true;
    }

    if (ctx.settings == nullptr || !ctx.settings->callGpu)
    {
        return TurnaroundTransition{TurnaroundPhase::CallServices};
    }

    if (ctx.aircraft->SupportsChocksControl() && !ctx.data.chocksPlaced)
    {
        ctx.aircraft->SetChocks(true);
        ctx.data.chocksPlaced = true;
    }

    const GroundPowerStatus gpu =
        ctx.aircraft->GetGroundPowerStatus().value_or(ctx.gsxGateway->GetGpuStatus());

    if (gpu == GroundPowerStatus::Connected || ctx.data.gpuRequested)
    {
        return TurnaroundTransition{TurnaroundPhase::CallServices};
    }

    if (gpu == GroundPowerStatus::Unknown)
    {
        return std::nullopt;
    }

    if (ctx.aircraft->SupportsGroundPowerControl())
    {
        ctx.aircraft->SetGroundPower(true);
        ctx.data.gpuRequested = true;

        return TurnaroundTransition{TurnaroundPhase::CallServices};
    }

    if (!ctx.menuGateway->IsMenuSettled())
    {
        return std::nullopt;
    }

    if (ctx.menuGateway->ToggleGpu())
    {
        ctx.data.gpuRequested = true;
        return TurnaroundTransition{TurnaroundPhase::CallServices};
    }

    return std::nullopt;
}
