#include "WaitingEnginesState.h"

#include "StandDeparture.h"
#include "../TurnaroundContext.h"
#include "../../ports/Aircraft.h"
#include "../../ports/DomainLogger.h"
#include "../../ports/GsxGateway.h"
#include "../../ports/GsxMenuGateway.h"

namespace
{
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

    TurnaroundTransition LeaveForTheDeparture(TurnaroundContext& ctx, const char* const reason)
    {
        ctx.data.engineConfirmationBlock = EngineConfirmationBlock::None;

        if (ctx.logger != nullptr)
        {
            ctx.logger->LogInfo(reason);
        }

        return TurnaroundTransition{TurnaroundPhase::WaitingDeparture};
    }
}

std::optional<TurnaroundTransition> WaitingEnginesState::EvaluatePhase(TurnaroundContext& ctx)
{
    auto& data = ctx.data;

    if (!ctx.gsxGateway->IsGoodEngineStartConfirmationEnabled())
    {
        return LeaveForTheDeparture(ctx, "GSX has the good engine start confirmation disabled; the flow moves on to the departure");
    }

    if (ctx.gsxGateway->IsPushbackFinished())
    {
        return LeaveForTheDeparture(ctx, "GSX finished the pushback; the flow moves on to the departure");
    }

    if (turnaround::HasLeftTheStand(ctx))
    {
        return LeaveForTheDeparture(ctx, "The aircraft is leaving without the engine confirmation; the flow moves on to the departure");
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

        if (viaInterruptMenu)
        {
            (void)ctx.menuGateway->CompletePushback();
        }
        else
        {
            (void)ctx.menuGateway->ConfirmGoodEngines();
        }
    }

    return std::nullopt;
}
