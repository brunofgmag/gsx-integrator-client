#include "GsxStateService.h"

#include <algorithm>
#include <cctype>
#include <ranges>

#include "GsxLVars.h"
#include "../logging/LogMacros.h"
#include "../../infrastructure/simvars/VariableGateway.h"

using namespace gsx::lvars;

GsxStateService::GsxStateService(VariableGateway* variableGateway, const GsxRemoteState* remoteState)
    : varManager_(variableGateway), remote_(remoteState)
{
    statesStatusMap_ = {
        {GsxState::Refueling, GsxStateStatus::Unavailable},
        {GsxState::Boarding, GsxStateStatus::Unavailable},
        {GsxState::Pushback, GsxStateStatus::Unavailable},
        {GsxState::Deboarding, GsxStateStatus::Unavailable},
    };

    statesCompletedMap_ = {
        {GsxState::Refueling, false},
        {GsxState::Boarding, false},
        {GsxState::Pushback, false},
        {GsxState::Deboarding, false},
    };
}


void GsxStateService::Reset()
{
    lastBoardingPassengers_ = 0;
    boardingPassengersTotal_ = 0;
    deboardingPassengersTotal_ = 0;
    lastDeboardingPassengers_ = 0;

    for (auto& completed : statesCompletedMap_ | std::views::values)
    {
        completed = false;
    }

    for (auto& status : statesStatusMap_ | std::views::values)
    {
        status = GsxStateStatus::Unavailable;
    }
}

bool GsxStateService::IsAvailable() const
{
    return varManager_->GetLVar(kCouatlStarted) >= 1.0;
}

GsxStateStatus GsxStateService::GetStateStatus(const GsxState gsxState)
{
    const char* stateLVar = nullptr;

    switch (gsxState)
    {
    case GsxState::Refueling:
        stateLVar = kRefuelingState;
        break;
    case GsxState::Boarding:
        stateLVar = kBoardingState;
        break;
    case GsxState::Pushback:
        stateLVar = kPushbackVehicleState;
        break;
    case GsxState::Deboarding:
        stateLVar = kDeboardingState;
        break;
    default:
        return GsxStateStatus::Unavailable;
    }

    const auto stateStatus = static_cast<GsxStateStatus>(varManager_->GetLVar(stateLVar));

    ParseCompleted(gsxState, stateStatus);
    statesStatusMap_.at(gsxState) = stateStatus;

    return stateStatus;
}

bool GsxStateService::WasStateCompleted(const GsxState gsxState) const
{
    return statesCompletedMap_.at(gsxState);
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
    const int currentBoarding = static_cast<int>(varManager_->GetLVar(kNumPassengersBoardingTotal));
    if (currentBoarding < lastBoardingPassengers_)
    {
        boardingPassengersTotal_ += lastBoardingPassengers_;
    }

    lastBoardingPassengers_ = currentBoarding;

    return boardingPassengersTotal_ + currentBoarding;
}

int GsxStateService::GetDeboardedPassengers()
{
    const int currentDeboarding = static_cast<int>(varManager_->GetLVar(kNumPassengersDeboardingTotal));
    if (currentDeboarding < lastDeboardingPassengers_)
    {
        deboardingPassengersTotal_ += lastDeboardingPassengers_;
    }

    lastDeboardingPassengers_ = currentDeboarding;

    return deboardingPassengersTotal_ + currentDeboarding;
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

bool GsxStateService::IsGpuConnected() const
{
    return varManager_->GetLVar(kGpuConnected) == 1.0;
}

bool GsxStateService::IsServiceInProgress(const GroundService service) const
{
    if (remote_ == nullptr)
    {
        return false;
    }

    const char* id = nullptr;
    switch (service)
    {
    case GroundService::Catering:
        id = "Catering";
        break;
    case GroundService::Lavatory:
        id = "Lavatory";
        break;
    case GroundService::Water:
        id = "Water";
        break;
    case GroundService::Cleaning:
        id = "Cleaning";
        break;
    default:
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

bool GsxStateService::IsAircraftOnGround() const
{
    return varManager_->GetAVar("SIM ON GROUND", "Bool", 1.0) == 1.0;
}

void GsxStateService::TakeOverFuelAndPayload()
{
    varManager_->SetLVar(kAutomationFuel, 0.0);
    varManager_->SetLVar(kAutomationPayload, 0.0);

    LOG_INFO("Taking over fuel and payload insertion");
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
    const bool returnedToIdle =
        (stateStatus == GsxStateStatus::Callable || stateStatus == GsxStateStatus::Bypassed)
        && statesStatusMap_.at(gsxState) == GsxStateStatus::Active;

    statesCompletedMap_.at(gsxState) = statesCompletedMap_.at(gsxState) ||
        stateStatus == GsxStateStatus::Completed || returnedToIdle;
}
