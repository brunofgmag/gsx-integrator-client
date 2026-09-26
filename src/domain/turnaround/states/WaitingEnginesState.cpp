#include "WaitingEnginesState.h"

#include "../TurnaroundContext.h"
#include "../../ports/Aircraft.h"
#include "../../ports/DomainLogger.h"
#include "../../ports/GsxGateway.h"
#include "../../ports/GsxMenuGateway.h"

namespace
{
    constexpr double kTaxiGroundSpeedKnots = 5.0;

    bool HasLeftWithoutConfirmingEngines(const TurnaroundContext& ctx)
    {
        if (!ctx.gsxGateway->IsAircraftOnGround())
        {
            return true;
        }

        return ctx.aircraft->IsEngineRunning()
            && ctx.gsxGateway->GetGroundSpeedKnots() >= kTaxiGroundSpeedKnots;
    }

    EngineConfirmationBlock BlockingReason(const TurnaroundContext& ctx, const bool viaInterruptMenu)
    {
        if (ctx.data.engineConfirmationSent)
        {
            return EngineConfirmationBlock::None;
        }

        if (!ctx.aircraft->IsEngineRunning())
        {
            return EngineConfirmationBlock::EnginesStopped;
        }

        if (viaInterruptMenu)
        {
            return EngineConfirmationBlock::None;
        }

        if (!ctx.gsxGateway->IsWaitingForEngines())
        {
            return EngineConfirmationBlock::GsxNotAsking;
        }

        if (!ctx.aircraft->IsParkingBrakeSet())
        {
            return EngineConfirmationBlock::ParkingBrakeReleased;
        }

        return EngineConfirmationBlock::None;
    }
}

std::optional<TurnaroundTransition> WaitingEnginesState::EvaluatePhase(TurnaroundContext& ctx)
{
    auto& data = ctx.data;

    if (!ctx.gsxGateway->IsGoodEngineStartConfirmationEnabled() || ctx.gsxGateway->IsPushbackFinished())
    {
        data.engineConfirmationBlock = EngineConfirmationBlock::None;

        return TurnaroundTransition{TurnaroundPhase::WaitingDeparture};
    }

    if (HasLeftWithoutConfirmingEngines(ctx))
    {
        data.engineConfirmationBlock = EngineConfirmationBlock::None;

        if (ctx.logger != nullptr)
        {
            ctx.logger->LogInfo("The aircraft is leaving without the engine confirmation; the flow moves on to the departure");
        }

        return TurnaroundTransition{TurnaroundPhase::WaitingDeparture};
    }

    const bool viaInterruptMenu = ctx.aircraft->CompletesPushbackViaInterruptMenu();

    data.engineConfirmationBlock = BlockingReason(ctx, viaInterruptMenu);

    if (data.engineConfirmationBlock != EngineConfirmationBlock::None)
    {
        return std::nullopt;
    }

    if (ctx.ConsumePilotTouch())
    {
        data.engineConfirmationSent = true;

        if (viaInterruptMenu ? ctx.menuGateway->CompletePushback() : ctx.menuGateway->ConfirmGoodEngines())
        {
            return TurnaroundTransition{TurnaroundPhase::WaitingDeparture};
        }
    }

    return std::nullopt;
}
