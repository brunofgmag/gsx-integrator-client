#include "PlaceArrivalGroundEquipmentState.h"

#include "../TurnaroundContext.h"
#include "../../model/AutomationSettings.h"
#include "../../ports/Aircraft.h"
#include "../../ports/DomainLogger.h"
#include "../../ports/GsxGateway.h"
#include "../../ports/GsxMenuGateway.h"

namespace
{
    constexpr int kConfirmationGiveUpTicks = 30;
    constexpr auto kGpuAlreadyRequested = "The ground power was already requested; not asking for it again";
    constexpr auto kGsxNeverConfirmed = "GSX never confirmed the ground power; moving on to the deboarding";

    void RequestTheGroundPower(TurnaroundContext& ctx)
    {
        if (ctx.aircraft->SupportsGroundPowerControl())
        {
            ctx.aircraft->SetGroundPower(true);

            return;
        }

        if (ctx.gsxGateway->IsServiceInProgress(GroundService::Gpu))
        {
            ctx.logger->LogInfo(kGpuAlreadyRequested);

            return;
        }

        ctx.menuGateway->ToggleGpu();
    }

    bool StopsWaitingForTheGroundPower(TurnaroundContext& ctx)
    {
        if (ctx.aircraft->SupportsGroundPowerControl() || ctx.gsxGateway->IsServiceInProgress(GroundService::Gpu))
        {
            return true;
        }

        if (ctx.data.arrivalGpuConfirmationTicks++ < kConfirmationGiveUpTicks)
        {
            return false;
        }

        ctx.logger->LogInfo(kGsxNeverConfirmed);

        return true;
    }
}

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

    if (gpu == GroundPowerStatus::Connected)
    {
        return TurnaroundTransition{TurnaroundPhase::RequestDeboarding};
    }

    if (!ctx.data.arrivalGpuRequested)
    {
        if (gpu == GroundPowerStatus::Unknown)
        {
            return std::nullopt;
        }

        RequestTheGroundPower(ctx);
        ctx.data.arrivalGpuRequested = true;
    }

    if (StopsWaitingForTheGroundPower(ctx))
    {
        return TurnaroundTransition{TurnaroundPhase::RequestDeboarding};
    }

    return std::nullopt;
}
