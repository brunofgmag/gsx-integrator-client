#include "PlaceGroundEquipmentState.h"

#include "../TurnaroundContext.h"
#include "../../model/AutomationSettings.h"
#include "../../ports/Aircraft.h"
#include "../../ports/GsxGateway.h"
#include "../../ports/GsxMenuGateway.h"

namespace
{
    constexpr int kGiveUpTicks = 240;
}

std::optional<TurnaroundTransition> PlaceGroundEquipmentState::EvaluatePhase(TurnaroundContext& ctx)
{
    if (!ctx.data.ownGroundEquipmentCleared)
    {
        ctx.aircraft->ClearOwnGroundEquipment();
        ctx.data.ownGroundEquipmentCleared = true;
    }

    if (!ctx.data.doorsClosed)
    {
        ctx.aircraft->HoldDoorsClosed(false);
        ctx.aircraft->CloseAllDoors();
        ctx.data.doorsClosed = true;
    }

    if (ctx.settings == nullptr || !ctx.settings->callGpu)
    {
        return TurnaroundTransition{TurnaroundPhase::CallServices};
    }

    if (!ctx.data.chocksPlaced && ctx.aircraft->SetChocks(true))
    {
        ctx.data.chocksPlaced = true;
    }

    const GroundPowerStatus gpu =
        ctx.aircraft->GetGroundPowerStatus().value_or(ctx.gsxGateway->GetGpuStatus());

    if (gpu == GroundPowerStatus::Connected)
    {
        return TurnaroundTransition{TurnaroundPhase::CallServices};
    }

    if (ctx.data.stateTickCount >= kGiveUpTicks)
    {
        return TurnaroundTransition{TurnaroundPhase::CallServices};
    }

    if (gpu == GroundPowerStatus::Unknown || ctx.data.gpuRequested)
    {
        return std::nullopt;
    }

    if (ctx.aircraft->SupportsGroundPowerControl())
    {
        ctx.aircraft->SetGroundPower(true);
        ctx.data.gpuRequested = true;

        return std::nullopt;
    }

    ctx.menuGateway->ToggleGpu();
    ctx.data.gpuRequested = true;

    return std::nullopt;
}
