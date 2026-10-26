#include "RemoveGroundEquipmentState.h"

#include <optional>

#include "../TurnaroundContext.h"
#include "../../model/AutomationSettings.h"
#include "../../ports/Aircraft.h"
#include "../../ports/DomainLogger.h"
#include "../../ports/GsxGateway.h"
#include "../../ports/GsxMenuGateway.h"

namespace
{
    constexpr int kRetryTicks = 60;

    constexpr auto kWaitingForThePilot = "Waiting for the pilot to remove the aircraft's own chocks and ground power";

    bool ManagesGpu(const TurnaroundContext& ctx)
    {
        const bool manageGpu =
            ctx.settings != nullptr && (ctx.settings->callGpu || ctx.settings->callGpuOnArrival);

        return manageGpu || ctx.data.gpuRequested || ctx.data.arrivalGpuRequested;
    }

    bool ShouldRemoveChocks(const TurnaroundContext& ctx)
    {
        const bool manageChocks =
            ctx.settings != nullptr && (ctx.settings->placeChocks || ctx.settings->placeChocksOnArrival);

        return ctx.aircraft->SupportsChocksControl()
            && (manageChocks || ctx.data.chocksPlaced || ctx.data.arrivalChocksPlaced);
    }
}

bool RemoveGroundEquipmentState::IsHeldByTheOwnGroundEquipment(TurnaroundContext& ctx)
{
    const bool attached = ctx.aircraft->IsOwnGroundEquipmentAttached();

    if (attached && !ownGroundEquipmentHoldAnnounced_)
    {
        ctx.logger->LogInfo(kWaitingForThePilot);
    }

    ownGroundEquipmentHoldAnnounced_ = attached;

    return attached;
}

std::optional<TurnaroundTransition> RemoveGroundEquipmentState::EvaluatePhase(TurnaroundContext& ctx)
{
    if (IsHeldByTheOwnGroundEquipment(ctx))
    {
        return std::nullopt;
    }

    const bool manageGpu = ManagesGpu(ctx);

    if (ShouldRemoveChocks(ctx) && !ctx.data.chocksRemoved)
    {
        if (!ctx.aircraft->IsParkingBrakeSet())
        {
            return std::nullopt;
        }

        if (ctx.aircraft->SetChocks(false))
        {
            ctx.data.chocksRemoved = true;
        }
    }

    const bool gpuDismissUnneeded = !manageGpu && !ctx.data.gpuDismissRequested;
    const std::optional<GroundPowerStatus> aircraftGpu = ctx.aircraft->GetGroundPowerStatus();
    if (!gpuDismissUnneeded && aircraftGpu == GroundPowerStatus::Unknown)
    {
        if (ctx.data.stateTickCount >= kRetryTicks)
        {
            return TurnaroundTransition{TurnaroundPhase::RequestPushback};
        }

        return std::nullopt;
    }

    const bool connected =
        aircraftGpu.value_or(ctx.gsxGateway->GetGpuStatus()) == GroundPowerStatus::Connected;
    const bool gpuBusy = ctx.gsxGateway->IsServiceInProgress(GroundService::Gpu);
    const bool gpuGone = !connected && !gpuBusy;

    if (gpuDismissUnneeded || gpuGone)
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
        const bool gpuIsOursToDismiss =
            gpuBusy || ctx.data.gpuRequested || ctx.data.arrivalGpuRequested;
        if (!gpuIsOursToDismiss)
        {
            ctx.logger->LogInfo(
                "GPU reads connected but GSX offers it as callable and this turnaround never asked; leaving it alone");

            return TurnaroundTransition{TurnaroundPhase::RequestPushback};
        }

        ctx.menuGateway->ToggleGpu();
        ctx.data.gpuDismissRequested = true;

        return std::nullopt;
    }

    if (ctx.TickCondition(kRetryTicks) && !gpuBusy)
    {
        return TurnaroundTransition{TurnaroundPhase::RequestPushback};
    }

    return std::nullopt;
}
