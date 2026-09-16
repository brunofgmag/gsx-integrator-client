#include "WaitingPushbackToStartState.h"

#include "../TurnaroundContext.h"
#include "../../ports/Aircraft.h"
#include "../../ports/DomainLogger.h"
#include "../../ports/GsxGateway.h"
#include "../../ports/GsxMenuGateway.h"

namespace
{
    constexpr double kTaxiGroundSpeedKnots = 5.0;

    bool HasLeftWithoutPushback(const TurnaroundContext& ctx)
    {
        if (!ctx.gsxGateway->IsAircraftOnGround())
        {
            return true;
        }

        return ctx.aircraft->IsEngineRunning()
            && ctx.gsxGateway->GetGroundSpeedKnots() >= kTaxiGroundSpeedKnots;
    }
}

std::optional<TurnaroundTransition> WaitingPushbackToStartState::EvaluatePhase(TurnaroundContext& ctx)
{
    if (ctx.gsxGateway->IsPushbackFinished() || ctx.gsxGateway->WasStateCompleted(GsxState::Pushback))
    {
        return TurnaroundTransition{TurnaroundPhase::WaitingDeparture};
    }

    if (ctx.gsxGateway->HasPushbackStarted())
    {
        return TurnaroundTransition{TurnaroundPhase::WaitingForEngines};
    }

    if (HasLeftWithoutPushback(ctx))
    {
        if (ctx.logger != nullptr)
        {
            ctx.logger->LogInfo("The aircraft is leaving without a pushback; the flow moves on to the departure");
        }

        return TurnaroundTransition{TurnaroundPhase::WaitingDeparture};
    }

    NoteDroppedPushback(ctx);

    ctx.menuGateway->OpenPushbackPanel();

    return std::nullopt;
}

void WaitingPushbackToStartState::NoteDroppedPushback(TurnaroundContext& ctx)
{
    auto& data = ctx.data;

    const bool pending = ctx.gsxGateway->IsServiceInProgress(GroundService::Departure);

    if (pending && !data.pushbackPending)
    {
        data.pushbackLostToGsxRestart = false;
    }

    if (data.pushbackPending && ctx.gsxGateway->WasGsxDownSinceLastObserve())
    {
        data.pushbackLostToGsxRestart = true;
    }

    data.pushbackPending = pending;

    NoteServiceInterruption(ctx, "pushback", data.pushbackLostToGsxRestart && !pending);
}
