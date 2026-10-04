#include "WaitingDepartureState.h"

#include "StandDeparture.h"
#include "../TurnaroundContext.h"
#include "../../ports/DomainLogger.h"
#include "../../ports/GsxGateway.h"

namespace
{
    constexpr int kOnFlightDelayTicks = 30;

    bool GsxStillAsksForTheEngineConfirmation(const TurnaroundContext& ctx)
    {
        return !ctx.data.engineWaitResumed
            && ctx.gsxGateway->IsGoodEngineStartConfirmationEnabled()
            && ctx.gsxGateway->IsWaitingForEngines()
            && !turnaround::HasLeftTheStand(ctx);
    }
}

std::optional<TurnaroundTransition> WaitingDepartureState::EvaluatePhase(TurnaroundContext& ctx)
{
    const bool isAircraftOnGround = ctx.gsxGateway->IsAircraftOnGround();
    if (!isAircraftOnGround)
    {
        return TurnaroundTransition{.next = TurnaroundPhase::OnFlight, .delayTicks = kOnFlightDelayTicks};
    }

    if (GsxStillAsksForTheEngineConfirmation(ctx))
    {
        ctx.data.engineWaitResumed = true;
        ctx.data.engineConfirmationSent = false;

        if (ctx.logger != nullptr)
        {
            ctx.logger->LogInfo("GSX still asks for the good engine start confirmation; the flow returns to the engine wait");
        }

        return TurnaroundTransition{TurnaroundPhase::WaitingForEngines};
    }

    return std::nullopt;
}
