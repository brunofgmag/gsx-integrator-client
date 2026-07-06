#include "RepositionAircraftState.h"

#include "../TurnaroundContext.h"
#include "../../ports/GsxGateway.h"
#include "../../ports/GsxMenuGateway.h"

namespace
{
    constexpr int kRetryTicks = 10;
    constexpr int kGiveUpTicks = 60;
}

std::optional<TurnaroundTransition> RepositionAircraftState::Evaluate(TurnaroundContext& ctx)
{
    bool& repositionRequested = ctx.data.repositionRequested;
    bool& repositionCompleted = ctx.data.repositionCompleted;

    if (!repositionCompleted && ctx.data.stateTickCount > kGiveUpTicks)
    {
        repositionCompleted = true;
    }

    if (!repositionRequested && !repositionCompleted)
    {
        repositionRequested = ctx.menuGateway->RepositionAircraft();
        return std::nullopt;
    }

    const bool isRepositioning = ctx.gsxGateway->IsRepositioning();
    if (repositionRequested && !repositionCompleted && isRepositioning)
    {
        repositionCompleted = true;
        return std::nullopt;
    }

    if (isRepositioning && !repositionCompleted)
    {
        return std::nullopt;
    }

    if (repositionCompleted)
    {
        return TurnaroundTransition{TurnaroundPhase::CallStairsOrJetway};
    }

    if (ctx.TickCondition(kRetryTicks) && repositionRequested)
    {
        ctx.menuGateway->DisableGsxMenu();
        repositionRequested = false;
    }

    return std::nullopt;
}
