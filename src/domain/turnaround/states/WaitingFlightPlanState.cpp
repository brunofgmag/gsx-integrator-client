#include "WaitingFlightPlanState.h"

#include <cmath>
#include <format>
#include <string>
#include "../TurnaroundContext.h"
#include "../TurnaroundMath.h"
#include "../../ports/Aircraft.h"
#include "../../ports/GsxGateway.h"
#include "../../ports/GsxMenuGateway.h"
#include "../../ports/DomainLogger.h"
#include "../../ports/FlightPlanSource.h"

namespace
{
    constexpr int kRetryTicks = 10;
    constexpr int kDifferingFlightPlanFetchTicks = 30;
}

std::optional<TurnaroundTransition> WaitingFlightPlanState::EvaluatePhase(TurnaroundContext& ctx)
{
    ctx.aircraft->SetCurrentZfwKg(ctx.aircraft->GetEmptyZfwKg());

    if (AwaitsLatestFlightPlan(ctx))
    {
        return std::nullopt;
    }

    FetchTheLatestPlanWhileTheAircraftPlanDiffers(ctx);

    if (!ctx.aircraft->IsFlightPlanLoaded())
    {
        return std::nullopt;
    }

    if (!GsxServesTheLatestPlan(ctx))
    {
        AwaitSimbriefLoad(ctx);

        return std::nullopt;
    }

    ctx.data.flightPlanRequested = false;

    if (ctx.data.flightPlanRefused && !ctx.data.latestFlightPlanRequested && ctx.flightPlanSource != nullptr)
    {
        ctx.logger->LogInfo("GSX accepted the SimBrief plan after refusing it: fetching the latest OFP before capturing it");
        ctx.flightPlanSource->RequestLatest();
        ctx.data.latestFlightPlanRequested = true;

        return std::nullopt;
    }

    CaptureFlightPlan(ctx);

    return TurnaroundTransition{TurnaroundPhase::WaitingPowerOn};
}

bool WaitingFlightPlanState::GsxServesTheLatestPlan(const TurnaroundContext& ctx)
{
    if (!ctx.gsxGateway->IsSimbriefLoaded())
    {
        return false;
    }

    const std::optional<int>& staleGeneration = ctx.data.staleSimbriefGeneration;

    return !staleGeneration.has_value()
        || (ctx.gsxGateway->GetServedSimbriefGeneration() > *staleGeneration
            && ctx.gsxGateway->GetSimbriefRefusal().empty());
}

void WaitingFlightPlanState::AwaitSimbriefLoad(TurnaroundContext& ctx)
{
    auto& data = ctx.data;
    if (!data.flightPlanRequested)
    {
        if (data.staleSimbriefGeneration.has_value())
        {
            ctx.logger->LogInfo(std::format(
                "Asking GSX to reload SimBrief: it must serve a generation newer than {}", *data.staleSimbriefGeneration));
        }
        ctx.menuGateway->RequestSimbriefLoad();
        data.flightPlanRequested = true;
        data.flightPlanRequestTicks = 0;

        return;
    }

    if (++data.flightPlanRequestTicks < kRetryTicks)
    {
        return;
    }

    if (const std::string refusal = ctx.gsxGateway->GetSimbriefRefusal(); !refusal.empty())
    {
        ctx.logger->LogInfo(std::format("GSX refused the SimBrief plan: {}", refusal));
        data.flightPlanRefused = true;
    }
    else
    {
        ctx.logger->LogInfo(std::format("GSX Simbrief plan not loaded after {} seconds", kRetryTicks));
    }
    data.flightPlanRequested = false;
}

bool WaitingFlightPlanState::AwaitsLatestFlightPlan(TurnaroundContext& ctx)
{
    if (!ctx.data.latestFlightPlanRequested)
    {
        return false;
    }

    const FlightPlanStatus status = ctx.status->flightPlanStatus;
    if (status == FlightPlanStatus::Ready)
    {
        return false;
    }

    if (status == FlightPlanStatus::Error && ctx.TickCondition(kRetryTicks))
    {
        ctx.logger->LogInfo("The latest SimBrief OFP failed to load: fetching it again");
        ctx.flightPlanSource->RequestLatest();
    }

    return true;
}

void WaitingFlightPlanState::FetchTheLatestPlanWhileTheAircraftPlanDiffers(TurnaroundContext& ctx)
{
    int& differingTicks = ctx.data.differingFlightPlanTicks;
    if (ctx.flightPlanSource == nullptr || !ctx.aircraft->FlightPlanDiffersFromTheOfp())
    {
        differingTicks = 0;

        return;
    }

    if (differingTicks % kDifferingFlightPlanFetchTicks == 0)
    {
        ctx.logger->LogInfo("The aircraft flight plan differs from the OFP: fetching the latest OFP");
        ctx.flightPlanSource->RequestLatest();
        ctx.data.staleSimbriefGeneration = ctx.gsxGateway->GetServedSimbriefGeneration();
        ctx.data.flightPlanRequested = false;
    }

    ++differingTicks;
}

void WaitingFlightPlanState::CaptureFlightPlan(TurnaroundContext& ctx)
{
    auto& data = ctx.data;
    data.plannedFuelKg = ctx.aircraft->GetPlannedFuelKg();
    data.plannedZfwKg = ctx.aircraft->GetPlannedZfwKg();
    data.plannedPassengers = ctx.aircraft->GetPlannedPassengers();
    data.loadedFuelKg = ctx.aircraft->GetCurrentFuelKg();
    data.initialFuelKg = data.loadedFuelKg;
    data.loadedZfwKg = ctx.aircraft->GetCurrentZfwKg();
    data.initialZfwKg = ctx.aircraft->GetEmptyZfwKg();

    ctx.aircraft->SetCurrentZfwKg(data.initialZfwKg);

    if (data.plannedPassengers == 0)
    {
        data.plannedPassengers = ctx.gsxGateway->GetPlannedPassengers();
    }

    NoteCrewLeftOutOfThePlan(ctx);
}

void WaitingFlightPlanState::NoteCrewLeftOutOfThePlan(TurnaroundContext& ctx)
{
    auto& data = ctx.data;

    const double crewKg = ctx.aircraft->GetCrewOnBoardKg();
    const double plannedOperatingEmptyKg = ctx.aircraft->GetPlannedOperatingEmptyKg();

    data.planOmitsCrew = crewKg > 0.0
        && std::abs(plannedOperatingEmptyKg - data.initialZfwKg) <= turnaround::kWeightEpsilonKg;
    data.omittedCrewKg = data.planOmitsCrew ? crewKg : 0.0;
    data.operatingEmptyWithCrewKg = data.planOmitsCrew ? plannedOperatingEmptyKg + crewKg : 0.0;

    if (data.planOmitsCrew)
    {
        ctx.logger->LogInfo(std::format(
            "The plan's operating empty weight of {:.0f} kg leaves out {:.0f} kg of crew; "
            "the ZFW target is {:.0f} kg, and SimBrief needs {:.0f} kg to count the crew",
            plannedOperatingEmptyKg, crewKg, data.plannedZfwKg, data.operatingEmptyWithCrewKg));
    }
}
