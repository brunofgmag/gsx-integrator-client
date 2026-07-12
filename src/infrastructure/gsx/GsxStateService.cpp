#include "GsxStateService.h"

#include <algorithm>
#include <cctype>
#include <ranges>

#include "GsxLVars.h"
#include "../logging/LogMacros.h"
#include "../../infrastructure/simvars/VariableGateway.h"

namespace
{
    // Services / Progress
    constexpr auto kRefuelingStateLVar = "FSDT_GSX_REFUELING_STATE";
    constexpr auto kRefuelingProgressLVar = "FSDT_GSX_FUELHOSE_CONNECTED";
    constexpr auto kFuelCounterLVar = "FSDT_GSX_FUEL_COUNTER";
    constexpr auto kFuelCounterMaxLVar = "FSDT_GSX_FUEL_COUNTER_MAX";
    constexpr auto kBoardingStateLVar = "FSDT_GSX_BOARDING_STATE";
    constexpr auto kDeboardingStateLVar = "FSDT_GSX_DEBOARDING_STATE";
    constexpr auto kPushbackVehicleStateLVar = "FSDT_GSX_VEHICLE_PUSHBACK_STATE";
    constexpr auto kPushbackStatusLVar = "FSDT_GSX_PUSHBACK_STATUS";

    // Passengers and Cargo
    constexpr auto kMaxPassengersLVar = "FSDT_GSX_MAX_NUMPASSENGERS";
    constexpr auto kNumPassengersBoardingTotalLVar = "FSDT_GSX_NUMPASSENGERS_BOARDING_TOTAL";
    constexpr auto kNumPassengerDeboardingTotalLVar = "FSDT_GSX_NUMPASSENGERS_DEBOARDING_TOTAL";
    constexpr auto kBoardingCargoPercentLVar = "FSDT_GSX_BOARDING_CARGO_PERCENT";
    constexpr auto kDeboardingCargoPercentLVar = "FSDT_GSX_DEBOARDING_CARGO_PERCENT";

    // Fuel Automation
    constexpr auto kAutomationFuelLVar = "FSDT_GSX_AUTOMATION_FUEL";
    constexpr auto kAutomationPayloadLVar = "FSDT_GSX_AUTOMATION_PAYLOAD";

    // GSX Simbrief integration
    constexpr auto kSimbriefSuccessLVar = "FSDT_GSX_SIMBRIEF_SUCCESS";

    // GSX Extras
    constexpr auto kJetwayAvailableLVar = "FSDT_GSX_JETWAY";
    constexpr auto kStairsAvailableLVar = "FSDT_GSX_STAIRS";
    constexpr auto kRepositioningStatusLVar = "FSDT_GSX_REPOSITIONING";

    // GSX Settings
    constexpr auto kGoodEngineStartLVar = "FSDT_GSX_SETTINGS_GOOD_ENGINE_START";
}

GsxStateService::GsxStateService(VariableGateway* variableGateway)
    : varManager_(variableGateway)
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
    return varManager_->GetLVar(gsx::lvars::kCouatlStarted) >= 1.0;
}

GsxStateStatus GsxStateService::GetStateStatus(const GsxState gsxState)
{
    const char* stateLVar = nullptr;

    switch (gsxState)
    {
    case GsxState::Refueling:
        stateLVar = kRefuelingStateLVar;
        break;
    case GsxState::Boarding:
        stateLVar = kBoardingStateLVar;
        break;
    case GsxState::Pushback:
        stateLVar = kPushbackVehicleStateLVar;
        break;
    case GsxState::Deboarding:
        stateLVar = kDeboardingStateLVar;
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
    return varManager_->GetLVar(kRefuelingProgressLVar) >= 1.0;
}

double GsxStateService::GetRefuelCounterGallons() const
{
    return std::max(varManager_->GetLVar(kFuelCounterLVar),
                    varManager_->GetLVar(kFuelCounterMaxLVar));
}

bool GsxStateService::HasPushbackStarted() const
{
    return varManager_->GetLVar(kPushbackStatusLVar) >= 5.0;
}

bool GsxStateService::IsPushbackFinished() const
{
    return varManager_->GetLVar(kPushbackStatusLVar) == 11;
}

bool GsxStateService::IsWaitingForEngines() const
{
    return varManager_->GetLVar(kPushbackStatusLVar) == 8;
}

bool GsxStateService::IsRepositioning() const
{
    return varManager_->GetLVar(kRepositioningStatusLVar) == 1.0;
}

int GsxStateService::GetPlannedPassengers() const
{
    return static_cast<int>(varManager_->GetLVar(kMaxPassengersLVar));
}

int GsxStateService::GetBoardedPassengers()
{
    const int currentBoarding = static_cast<int>(varManager_->GetLVar(kNumPassengersBoardingTotalLVar));
    if (currentBoarding < lastBoardingPassengers_)
    {
        boardingPassengersTotal_ += lastBoardingPassengers_;
    }

    lastBoardingPassengers_ = currentBoarding;

    return boardingPassengersTotal_ + currentBoarding;
}

int GsxStateService::GetDeboardedPassengers()
{
    const int currentDeboarding = static_cast<int>(varManager_->GetLVar(kNumPassengerDeboardingTotalLVar));
    if (currentDeboarding < lastDeboardingPassengers_)
    {
        deboardingPassengersTotal_ += lastDeboardingPassengers_;
    }

    lastDeboardingPassengers_ = currentDeboarding;

    return deboardingPassengersTotal_ + currentDeboarding;
}

double GsxStateService::GetBoardingCargoPercent() const
{
    return varManager_->GetLVar(kBoardingCargoPercentLVar);
}

double GsxStateService::GetDeboardingCargoPercent() const
{
    return varManager_->GetLVar(kDeboardingCargoPercentLVar);
}

bool GsxStateService::AreStairsInPlace() const
{
    return varManager_->GetLVar(kStairsAvailableLVar) == 5.0;
}

bool GsxStateService::IsJetwayInPlace() const
{
    return varManager_->GetLVar(kJetwayAvailableLVar) == 5.0;
}

bool GsxStateService::AreStairsAvailable() const
{
    return varManager_->GetLVar(kStairsAvailableLVar, 2.0) != 2.0;
}

bool GsxStateService::IsJetwayAvailable() const
{
    return varManager_->GetLVar(kJetwayAvailableLVar, 2.0) != 2.0;
}

bool GsxStateService::IsAircraftOnGround() const
{
    return varManager_->GetAVar("SIM ON GROUND", "Bool", 1.0) == 1.0;
}

void GsxStateService::TakeOverFuelAndPayload()
{
    varManager_->SetLVar(kAutomationFuelLVar, 0.0);
    varManager_->SetLVar(kAutomationPayloadLVar, 0.0);

    LOG_INFO("Taking over fuel and payload insertion");
}


bool GsxStateService::IsSimbriefLoaded() const
{
    return varManager_->GetLVar(kSimbriefSuccessLVar) >= 1.0;
}

bool GsxStateService::IsGoodEngineStartConfirmationEnabled() const
{
    return varManager_->GetLVar(kGoodEngineStartLVar, 1.0) >= 1.0;
}

void GsxStateService::ParseCompleted(const GsxState gsxState, const GsxStateStatus stateStatus)
{
    const bool returnedToIdle =
        (stateStatus == GsxStateStatus::Callable || stateStatus == GsxStateStatus::Bypassed)
        && statesStatusMap_.at(gsxState) == GsxStateStatus::Active;

    statesCompletedMap_.at(gsxState) = statesCompletedMap_.at(gsxState) ||
        stateStatus == GsxStateStatus::Completed || returnedToIdle;
}
