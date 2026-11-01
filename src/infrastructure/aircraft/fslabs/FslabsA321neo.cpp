#include "FslabsA321neo.h"

#include <array>
#include <cmath>
#include <memory>
#include <optional>
#include "../AircraftRegistry.h"
#include "../DoorReading.h"
#include "../../fslabs/FslabsGroundClient.h"
#include "../../fslabs/FslabsMcduClient.h"
#include "../../gsx/GsxLVars.h"
#include "../../logging/LogMacros.h"
#include "../../simvars/SimVars.h"
#include "../../../domain/model/AutomationStatus.h"
#include "../../../domain/model/FlightPlan.h"

using namespace simvars;

namespace
{
    constexpr double kRecommendedFuelRateKgs = 0.0;

    constexpr auto kSmartSwitchLVar = "VC_PED_DRAIMS_1_INT_RAD_Switch";
    constexpr auto kSmartSwitchControl = "INT/RAD";
    constexpr auto kSmartSwitchSide = "RAD";
    constexpr double kSmartSwitchRadio = 0.0;

    constexpr auto kParkingBrakeLVar = "FSLA320_ParkBrake";
    constexpr auto kChocksLVar = "FSLA320_Wheel_Chocks";

    constexpr std::array kDoorLVars = {
        "FSLA320_pax_door1", "FSLA320_pax_door2", "FSLA320_pax_door3", "FSLA320_pax_door4",
        "FSLA320_upper_cargo_door", "FSLA320_lower_cargo_door",
    };

    constexpr int kEngineCount = 2;
    constexpr double kEngineRunningDefault = 1.0;
    constexpr int kShutdownTicksBeforeTheAircraftDeboards = 75;
    constexpr int kTicksAPowerAskCounts = 5;
    constexpr double kRemoteControlOn = 1.0;
}

FslabsA321neo::FslabsA321neo(VariableGateway* variableGateway, const AutomationStatus* status,
                             std::unique_ptr<FslabsMcduGateway> rightMcdu, std::unique_ptr<FslabsMcduDisplay> leftMcdu,
                             std::unique_ptr<FslabsGroundGateway> ground)
    : variableGateway_(variableGateway),
      status_(status),
      rightMcdu_(std::move(rightMcdu)),
      leftMcdu_(std::move(leftMcdu)),
      ground_(std::move(ground)),
      aoc_(*rightMcdu_, *leftMcdu_),
      holds_(*variableGateway, *ground_),
      smartSwitch_(*variableGateway, {kSmartSwitchLVar},
                   [](const double min, double) { return min <= kSmartSwitchRadio; })
{
    smartSwitch_.Subscribe();

    LOG_INFO("Profile loaded: FSLabs A321neo");
}

bool FslabsA321neo::IsCargoVariant() const
{
    return false;
}

void FslabsA321neo::Observe()
{
    ++observeTicks_;
    rightMcdu_->Poll();
    leftMcdu_->Poll();
    ground_->Poll();
    WatchThePassengerCount();
    aoc_.Advance(AwaitsTheInit(), PowerIsAsked());
    holds_.Advance();
    CountTheShutdownTicks();
}

void FslabsA321neo::OnLoadingStarted()
{
    LOG_INFO("FSLabs A321neo loading started: the aircraft drives the GSX services and loads itself");
}

void FslabsA321neo::OnTurnaroundStarted()
{
    aoc_.ResetForNewTurnaround();
    holds_.ResetForNewTurnaround();
}

bool FslabsA321neo::IsDrivingTheGsxMenu() const
{
    return variableGateway_->HasReceivedLVar(gsx::lvars::kSetRemoteControl)
        && variableGateway_->GetLVar(gsx::lvars::kSetRemoteControl, 0.0) >= kRemoteControlOn;
}

bool FslabsA321neo::IsReachable() const
{
    return rightMcdu_->IsAvailable();
}

bool FslabsA321neo::AwaitsTheInit() const
{
    return status_->flightPlanStatus == FlightPlanStatus::Ready;
}

bool FslabsA321neo::PowerIsAsked() const
{
    return powerAskedTick_.has_value() && observeTicks_ - *powerAskedTick_ <= kTicksAPowerAskCounts;
}

bool FslabsA321neo::IsFlightPlanLoaded() const
{
    return AwaitsTheInit() && aoc_.IsInitDone();
}

double FslabsA321neo::GetPlannedFuelKg() const
{
    return status_->plannedFuelKg;
}

double FslabsA321neo::GetPlannedZfwKg() const
{
    return status_->plannedZfwKg;
}

int FslabsA321neo::GetPlannedPassengers() const
{
    return status_->plannedPassengers;
}

double FslabsA321neo::GetEmptyZfwKg() const
{
    return EmptyZfwKg(*variableGateway_);
}

double FslabsA321neo::GetCurrentFuelKg() const
{
    return CurrentFuelKg(*variableGateway_);
}

double FslabsA321neo::GetCurrentZfwKg() const
{
    if (!variableGateway_->HasReceivedAVar(kSimEmptyWeight, kKgUnit))
    {
        return 0.0;
    }

    return CurrentZfwKg(*variableGateway_);
}

void FslabsA321neo::WatchThePassengerCount() const
{
    static_cast<void>(variableGateway_->HasReceivedLVar(gsx::lvars::kNumPassengers));
}

std::optional<int> FslabsA321neo::GetPassengersItBoards() const
{
    if (!variableGateway_->HasReceivedLVar(gsx::lvars::kNumPassengers))
    {
        return std::nullopt;
    }

    const double passengers = variableGateway_->GetLVar(gsx::lvars::kNumPassengers, 0.0);
    if (passengers <= 0.0)
    {
        return std::nullopt;
    }

    return static_cast<int>(std::lround(passengers));
}

bool FslabsA321neo::RequestRefuelingItself()
{
    static_cast<void>(aoc_.Request(AocRequest::SendOfp));

    return true;
}

bool FslabsA321neo::RequestBoardingItself()
{
    static_cast<void>(aoc_.Request(AocRequest::Board));

    return true;
}

bool FslabsA321neo::RequestPushbackItself()
{
    static_cast<void>(aoc_.Request(AocRequest::Pushback));

    return true;
}

bool FslabsA321neo::ConsumeSmartSwitch()
{
    return smartSwitch_.Consume();
}

DoorStatus FslabsA321neo::GetDoorStatus() const
{
    DoorStatus status = doors::kNoDoorsSeen;

    for (const char* const doorLVar : kDoorLVars)
    {
        status = doors::Combine(status, doors::OpenAboveZero(*variableGateway_, doorLVar));
    }

    return status;
}

bool FslabsA321neo::IsPowered() const
{
    powerAskedTick_ = observeTicks_;

    return aoc_.IsAlive();
}

bool FslabsA321neo::IsReadyToPush() const
{
    return IsPowered() && !IsEngineRunning() && IsBeaconOn();
}

bool FslabsA321neo::IsReadyToDeboard() const
{
    return shutdownTicks_ >= kShutdownTicksBeforeTheAircraftDeboards;
}

bool FslabsA321neo::IsEngineRunning() const
{
    return AnyEngineCombusting(*variableGateway_, kEngineRunningDefault, kEngineCount);
}

bool FslabsA321neo::IsHeldInPlace() const
{
    return IsParkingBrakeSet() || variableGateway_->GetLVar(kChocksLVar, 0.0) > 0.0;
}

std::optional<ChocksStatus> FslabsA321neo::GetChocksStatus() const
{
    if (!variableGateway_->HasReceivedLVar(kChocksLVar))
    {
        return ChocksStatus::Unknown;
    }

    return variableGateway_->GetLVar(kChocksLVar, 0.0) > 0.0 ? ChocksStatus::Placed : ChocksStatus::Removed;
}

bool FslabsA321neo::IsParkingBrakeSet() const
{
    return variableGateway_->GetLVar(kParkingBrakeLVar, 0.0) > 0.0;
}

bool FslabsA321neo::IsBeaconOn() const
{
    return variableGateway_->GetAVar(kSimBeaconLight, kBoolUnit, 0.0) > 0.0;
}

void FslabsA321neo::CountTheShutdownTicks()
{
    shutdownTicks_ = !IsEngineRunning() && !IsBeaconOn() ? shutdownTicks_ + 1 : 0;
}

namespace
{
    std::unique_ptr<Aircraft> CreateFslabsA321neo(const AircraftContext& context, const AircraftIdentity&)
    {
        return std::make_unique<FslabsA321neo>(context.variableGateway, context.status,
                                               std::make_unique<FslabsMcduClient>(FslabsMcduSide::Right),
                                               std::make_unique<FslabsMcduClient>(FslabsMcduSide::Left),
                                               std::make_unique<FslabsGroundClient>());
    }

    const AircraftDescriptor kFslabsA321neoDescriptor{
        FslabsA321neo::kName,
        {
            {MatchField::Title, MatchOp::StartsWith, "FSLabs A321-"},
        },
        &CreateFslabsA321neo, "fslabs-a321neo", "A21N", RefuelBy::Gsx, SmartSwitchCue{kSmartSwitchControl, kSmartSwitchSide, SmartSwitchMove::Flip}, kRecommendedFuelRateKgs,
    };

    [[maybe_unused]] const AircraftRegistration kFslabsA321neoRegistration{kFslabsA321neoDescriptor};
}
