#include "RemoveGroundEquipmentState.h"

#include "../TurnaroundContext.h"
#include "../../model/AutomationSettings.h"
#include "../../ports/Aircraft.h"
#include "../../ports/GsxGateway.h"
#include "../../ports/GsxMenuGateway.h"

namespace
{
    constexpr int kRetryTicks = 60;
}

std::optional<TurnaroundTransition> RemoveGroundEquipmentState::Evaluate(TurnaroundContext& ctx)
{
    const bool manageEquipment = ctx.settings != nullptr && (ctx.settings->callGpu || ctx.settings->callGpuOnArrival);
    const bool removeChocks = manageEquipment || ctx.data.chocksPlaced || ctx.data.arrivalChocksPlaced;

    if (removeChocks && ctx.aircraft->SupportsChocksControl() && !ctx.data.chocksRemoved)
    {
        ctx.aircraft->SetChocks(false);
        ctx.data.chocksRemoved = true;
    }

    const bool connected =
        ctx.aircraft->GetGroundPowerStatus().value_or(ctx.gsxGateway->GetGpuStatus())
        == GroundPowerStatus::Connected;
    const bool gpuBusy = ctx.gsxGateway->IsServiceInProgress(GroundService::Gpu);
    const bool gpuGone = !connected && !gpuBusy;
    const bool gpuUnmanaged = !manageEquipment && !ctx.data.gpuDismissRequested;

    if (gpuUnmanaged || gpuGone)
    {
        return TurnaroundTransition{TurnaroundPhase::RequestPushback};
    }

    if (ctx.aircraft->SupportsGroundPowerControl())
    {
        if (!ctx.data.gpuDismissRequested)
        {
            ctx.aircraft->SetGroundPower(false);
            ctx.data.gpuDismissRequested = true;
        }
        else if (ctx.TickCondition(kRetryTicks))
        {
            return TurnaroundTransition{TurnaroundPhase::RequestPushback};
        }

        return std::nullopt;
    }

    if (!ctx.data.gpuDismissRequested)
    {
        if (!ctx.menuGateway->IsMenuSettled())
        {
            return std::nullopt;
        }

        static_cast<void>(ctx.menuGateway->ToggleGpu());
        ctx.data.gpuDismissRequested = true;

        return std::nullopt;
    }

    if (ctx.TickCondition(kRetryTicks))
    {
        if (!gpuBusy)
        {
            return TurnaroundTransition{TurnaroundPhase::RequestPushback};
        }

        ctx.data.gpuDismissRequested = false;
    }

    return std::nullopt;
}
