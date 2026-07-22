#include "GsxStateService.h"

#include <algorithm>
#include <cctype>
#include <ranges>

#include "GsxLVars.h"
#include "../logging/LogMacros.h"
#include "../../infrastructure/simvars/VariableGateway.h"

using namespace gsx::lvars;

namespace
{
    const char* StateLVarName(const GsxState gsxState)
    {
        switch (gsxState)
        {
        case GsxState::Refueling: return kRefuelingState;
        case GsxState::Boarding: return kBoardingState;
        case GsxState::Pushback: return kPushbackVehicleState;
        case GsxState::Deboarding: return kDeboardingState;
        case GsxState::Deice: return kDeiceState;
        default: return nullptr;
        }
    }

    const char* ServiceId(const GroundService service)
    {
        switch (service)
        {
        case GroundService::Catering: return "Catering";
        case GroundService::Lavatory: return "Lavatory";
        case GroundService::Water: return "Water";
        case GroundService::Cleaning: return "Cleaning";
        case GroundService::Gpu: return "GPU";
        case GroundService::Departure: return "Departure";
        default: return nullptr;
        }
    }
}

GsxStateService::GsxStateService(VariableGateway* variableGateway, const GsxRemoteState* remoteState)
    : varManager_(variableGateway), remote_(remoteState),
      states_{
          {GsxState::Refueling, {}},
          {GsxState::Boarding, {}},
          {GsxState::Pushback, {}},
          {GsxState::Deboarding, {}},
          {GsxState::Deice, {}},
      }
{
}

void GsxStateService::Reset()
{
    boarding_ = {};
    deboarding_ = {};
    fuelAndPayloadTakenOver_ = false;

    for (auto& track : states_ | std::views::values)
    {
        track = {};
    }
}

bool GsxStateService::IsAvailable() const
{
    return varManager_->GetLVar(kCouatlStarted) >= 1.0;
}

GsxStateStatus GsxStateService::GetStateStatus(const GsxState gsxState)
{
    const char* stateLVar = StateLVarName(gsxState);
    if (stateLVar == nullptr)
    {
        return GsxStateStatus::Unavailable;
    }

    const auto stateStatus = static_cast<GsxStateStatus>(varManager_->GetLVar(stateLVar));

    ParseCompleted(gsxState, stateStatus);
    states_.at(gsxState).status = stateStatus;

    return stateStatus;
}

bool GsxStateService::WasStateCompleted(const GsxState gsxState) const
{
    return states_.at(gsxState).completed;
}

bool GsxStateService::IsFuelHoseConnected() const
{
    return varManager_->GetLVar(kFuelHoseConnected) >= 1.0;
}

double GsxStateService::GetRefuelCounterGallons() const
{
    return std::max(varManager_->GetLVar(kFuelCounter),
                    varManager_->GetLVar(kFuelCounterMax));
}

bool GsxStateService::HasPushbackStarted() const
{
    return varManager_->GetLVar(kPushbackStatus) >= 5.0;
}

bool GsxStateService::IsPushbackFinished() const
{
    return varManager_->GetLVar(kPushbackStatus) == 11;
}

bool GsxStateService::IsWaitingForEngines() const
{
    return varManager_->GetLVar(kPushbackStatus) == 8;
}

bool GsxStateService::IsRepositioning() const
{
    return varManager_->GetLVar(kRepositioning) == 1.0;
}

int GsxStateService::GetPlannedPassengers() const
{
    return static_cast<int>(varManager_->GetLVar(kMaxPassengers));
}

int GsxStateService::GetBoardedPassengers()
{
    const bool active = varManager_->GetLVar(kBoardingState) == static_cast<double>(GsxStateStatus::Active);

    return boarding_.Update(static_cast<int>(varManager_->GetLVar(kNumPassengersBoardingTotal)), active);
}

int GsxStateService::GetDeboardedPassengers()
{
    const bool active = varManager_->GetLVar(kDeboardingState) == static_cast<double>(GsxStateStatus::Active);

    return deboarding_.Update(static_cast<int>(varManager_->GetLVar(kNumPassengersDeboardingTotal)), active);
}

int GsxStateService::PassengerCounter::Update(const int current, const bool active)
{
    if (!counting)
    {
        if (!active)
        {
            return 0;
        }

        counting = true;
        last = current;

        return total + current;
    }

    if (current < last)
    {
        if (grown)
        {
            total += last;
        }

        grown = current > 0;
    }
    else if (current > last)
    {
        grown = true;
    }

    last = current;

    return total + current;
}

double GsxStateService::GetBoardingCargoPercent() const
{
    return varManager_->GetLVar(kBoardingCargoPercent);
}

double GsxStateService::GetDeboardingCargoPercent() const
{
    return varManager_->GetLVar(kDeboardingCargoPercent);
}

bool GsxStateService::AreStairsInPlace() const
{
    return varManager_->GetLVar(kStairs) == 5.0;
}

bool GsxStateService::IsJetwayInPlace() const
{
    return varManager_->GetLVar(kJetway) == 5.0;
}

GroundPowerStatus GsxStateService::GetGpuStatus() const
{
    if (!varManager_->HasReceivedLVar(kGpuState))
    {
        return GroundPowerStatus::Unknown;
    }

    const bool connected =
        varManager_->GetLVar(kGpuState) == static_cast<double>(GsxStateStatus::Active)
        || varManager_->GetLVar(kGpuConnected) == 1.0;

    return connected ? GroundPowerStatus::Connected : GroundPowerStatus::Disconnected;
}

bool GsxStateService::IsServiceInProgress(const GroundService service) const
{
    if (remote_ == nullptr)
    {
        return false;
    }

    const char* id = ServiceId(service);
    if (id == nullptr)
    {
        return false;
    }

    const GsxRemoteService* svc = FindService(*remote_, id);
    if (svc == nullptr)
    {
        return false;
    }

    return svc->stateRaw == static_cast<int>(GsxStateStatus::Requested)
        || svc->stateRaw == static_cast<int>(GsxStateStatus::Active);
}

bool GsxStateService::AreStairsAvailable() const
{
    return varManager_->GetLVar(kStairs, 2.0) != 2.0;
}

bool GsxStateService::IsJetwayAvailable() const
{
    return varManager_->GetLVar(kJetway, 2.0) != 2.0;
}

bool GsxStateService::IsJetwayOrStairsOperating() const
{
    return varManager_->GetLVar(kJetway) == static_cast<double>(GsxStateStatus::Requested)
        || varManager_->GetLVar(kStairs) == static_cast<double>(GsxStateStatus::Requested);
}

bool GsxStateService::IsAircraftOnGround() const
{
    return varManager_->GetAVar("SIM ON GROUND", "Bool", 1.0) == 1.0;
}

void GsxStateService::TakeOverFuelAndPayload()
{
    varManager_->SetLVar(kAutomationFuel, 0.0);
    varManager_->SetLVar(kAutomationPayload, 0.0);
    fuelAndPayloadTakenOver_ = true;

    LOG_INFO("Taking over fuel and payload insertion");
}

void GsxStateService::ReassertTakeovers() const
{
    if (!fuelAndPayloadTakenOver_)
    {
        return;
    }

    if (varManager_->GetLVar(kAutomationFuel, 1.0) != 0.0
        || varManager_->GetLVar(kAutomationPayload, 1.0) != 0.0)
    {
        LOG_INFO("GSX automation flags reset by couatl; re-taking fuel and payload");
        varManager_->SetLVar(kAutomationFuel, 0.0);
        varManager_->SetLVar(kAutomationPayload, 0.0);
    }
}


bool GsxStateService::IsSimbriefLoaded() const
{
    return varManager_->GetLVar(kSimbriefSuccess) >= 1.0;
}

bool GsxStateService::IsGoodEngineStartConfirmationEnabled() const
{
    return varManager_->GetLVar(kGoodEngineStart, 1.0) >= 1.0;
}

void GsxStateService::ParseCompleted(const GsxState gsxState, const GsxStateStatus stateStatus)
{
    StateTrack& track = states_.at(gsxState);

    const bool returnedToIdle =
        (stateStatus == GsxStateStatus::Callable || stateStatus == GsxStateStatus::Bypassed)
        && track.status == GsxStateStatus::Active;

    track.completed = track.completed || stateStatus == GsxStateStatus::Completed || returnedToIdle;
}
