#include "RepositionAircraftState.h"

#include <optional>

#include "../TurnaroundContext.h"
#include "../../model/AutomationSettings.h"
#include "../../model/AutomationStatus.h"
#include "../../ports/DomainLogger.h"
#include "../../ports/GsxGateway.h"
#include "../../ports/GsxMenuGateway.h"

namespace
{
    constexpr int kRetryTicks = 10;
    constexpr int kGiveUpTicks = 60;

    bool TheOptionSkipsTheReposition(const TurnaroundContext& ctx)
    {
        return ctx.settings != nullptr && ctx.settings->skipReposition && !ctx.data.repositionRequested;
    }

    bool ThisSessionAlreadyRepositionedAndTheNewTurnaroundSkipsIt(const TurnaroundContext& ctx)
    {
        return ctx.settings != nullptr
            && ctx.settings->skipRepositionOnNewTurnaround
            && ctx.data.repositionedThisSession
            && !ctx.data.repositionRequested;
    }

    TurnaroundTransition SkipRepositionOverTheService(TurnaroundContext& ctx)
    {
        ctx.logger->LogInfo("A GSX service is underway: skipping the reposition because it would cancel it");
        ctx.data.repositionCompleted = true;

        return TurnaroundTransition{TurnaroundPhase::PlaceGroundEquipment};
    }
}

std::optional<TurnaroundTransition> RepositionAircraftState::EvaluatePhase(TurnaroundContext& ctx)
{
    if (TheOptionSkipsTheReposition(ctx) || ThisSessionAlreadyRepositionedAndTheNewTurnaroundSkipsIt(ctx))
    {
        return TurnaroundTransition{TurnaroundPhase::PlaceGroundEquipment};
    }

    if (ctx.status != nullptr && !ctx.status->gsxAvailable)
    {
        ctx.data.stateTickCount = 0;
        return std::nullopt;
    }

    bool& repositionRequested = ctx.data.repositionRequested;
    bool& repositionCompleted = ctx.data.repositionCompleted;

    if (!repositionCompleted && ctx.data.stateTickCount > kGiveUpTicks)
    {
        repositionCompleted = true;
    }

    if (!repositionRequested && !repositionCompleted)
    {
        const std::optional<bool> serviceUnderway = ctx.gsxGateway->HasServiceUnderway();
        if (!serviceUnderway.has_value())
        {
            return std::nullopt;
        }

        if (*serviceUnderway)
        {
            return SkipRepositionOverTheService(ctx);
        }

        ctx.menuGateway->RepositionAircraft();
        repositionRequested = true;
        ctx.data.repositionAttempted = true;
        return std::nullopt;
    }

    const bool isRepositioning = ctx.gsxGateway->IsRepositioning();
    if (repositionRequested && !repositionCompleted && isRepositioning)
    {
        repositionCompleted = true;
        return std::nullopt;
    }

    if (repositionCompleted)
    {
        if (ctx.data.repositionAttempted)
        {
            ctx.data.repositionedThisSession = true;
        }

        return TurnaroundTransition{TurnaroundPhase::PlaceGroundEquipment};
    }

    if (ctx.TickCondition(kRetryTicks) && repositionRequested)
    {
        ctx.menuGateway->DisableGsxMenu();
        repositionRequested = false;
    }

    return std::nullopt;
}
