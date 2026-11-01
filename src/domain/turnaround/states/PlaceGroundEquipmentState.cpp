#include "PlaceGroundEquipmentState.h"

#include "GroundEquipment.h"
#include "../TurnaroundContext.h"
#include "../../model/AutomationSettings.h"
#include "../../ports/Aircraft.h"
#include "../../ports/DomainLogger.h"
#include "../../ports/GsxGateway.h"
#include "../../ports/GsxMenuGateway.h"

namespace
{
    constexpr auto kGaveUpOnTheEquipment =
        "The ground power or the chocks were not in place after 4 minutes; moving on to the services";
    constexpr auto kGpuAlreadyRequested = "The ground power was already requested; waiting for it instead of asking again";
    constexpr auto kLeavingDoorsOverService =
        "A GSX service is underway: leaving the doors as they are because closing them would interrupt it";

    bool WaitsForTheServiceStateBeforeTouchingDoors(TurnaroundContext& ctx)
    {
        if (ctx.data.doorsClosed)
        {
            return false;
        }

        const std::optional<bool> serviceUnderway = ctx.gsxGateway->HasServiceUnderway();
        if (!serviceUnderway.has_value() && ctx.data.stateTickCount < turnaround::kGroundEquipmentGiveUpTicks)
        {
            return true;
        }

        ctx.aircraft->HoldDoorsClosed(false);

        if (serviceUnderway.value_or(false))
        {
            ctx.logger->LogInfo(kLeavingDoorsOverService);
        }
        else
        {
            ctx.aircraft->CloseAllDoors();
        }

        ctx.data.doorsClosed = true;

        return false;
    }

    bool WaitsForTheAircraftToBeHeldBeforeAskingGsx(const TurnaroundContext& ctx)
    {
        return !ctx.aircraft->SupportsGroundPowerControl() && !ctx.aircraft->IsHeldInPlace();
    }

    bool IsReadyToRequestTheGroundPower(const TurnaroundContext& ctx)
    {
        if (ctx.data.gpuRequested || turnaround::CurrentGroundPower(ctx) == GroundPowerStatus::Unknown)
        {
            return false;
        }

        return !WaitsForTheAircraftToBeHeldBeforeAskingGsx(ctx);
    }

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
}

std::optional<TurnaroundTransition> PlaceGroundEquipmentState::EvaluatePhase(TurnaroundContext& ctx)
{
    if (!ctx.data.ownGroundEquipmentCleared)
    {
        ctx.aircraft->ClearOwnGroundEquipment();
        ctx.data.ownGroundEquipmentCleared = true;
    }

    if (WaitsForTheServiceStateBeforeTouchingDoors(ctx))
    {
        return std::nullopt;
    }

    const bool placeChocks = ctx.settings != nullptr && ctx.settings->placeChocks;
    const bool callGpu = ctx.settings != nullptr && ctx.settings->callGpu;

    if (placeChocks && !ctx.data.chocksPlaced && ctx.aircraft->SetChocks(true))
    {
        ctx.data.chocksPlaced = true;
    }

    const turnaround::GroundEquipmentWish wish{.placeChocks = placeChocks, .chocksCommanded = ctx.data.chocksPlaced, .callGpu = callGpu};

    if (turnaround::IsGroundEquipmentInPlace(ctx, wish))
    {
        return TurnaroundTransition{TurnaroundPhase::CallServices};
    }

    if (ctx.data.stateTickCount >= turnaround::kGroundEquipmentGiveUpTicks)
    {
        ctx.logger->LogInfo(kGaveUpOnTheEquipment);
        ctx.data.groundEquipmentTimedOut = true;

        return TurnaroundTransition{TurnaroundPhase::CallServices};
    }

    if (!turnaround::IsGroundPowerInPlace(ctx, wish) && IsReadyToRequestTheGroundPower(ctx))
    {
        RequestTheGroundPower(ctx);
        ctx.data.gpuRequested = true;
    }

    return std::nullopt;
}
