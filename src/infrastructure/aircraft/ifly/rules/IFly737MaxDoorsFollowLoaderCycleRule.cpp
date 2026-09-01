#include "IFly737MaxDoorsFollowLoaderCycleRule.h"

#include "../IFly737Max.h"
#include "../../../gsx/GsxLVars.h"
#include "../../../logging/LogMacros.h"
#include "../../../simvars/VariableGateway.h"
#include "../../../../domain/ports/GsxGateway.h"

namespace
{
    constexpr auto kRuleName = "ifly-737max-doors-follow-loader-cycle";

    constexpr auto kFwdCargoAnimLVar = "Animation_FWD_Cargo_VAL";
    constexpr auto kAftCargoAnimLVar = "Animation_AFT_Cargo_VAL";

    constexpr double kCargoDoorOpenThreshold = 90.0;
    constexpr double kCargoDoorFullyOpen = 100.0;
    constexpr double kJetwayAtAircraft = 4.0;

    constexpr double kTogglePressed = 1.0;
    constexpr double kToggleReleased = 0.0;

    constexpr int kPulseSettleTicks = 15;
    constexpr int kMaxDoorPulseAttempts = 5;
    constexpr int kAircraftOpensItselfTicks = 10;

    bool IsState(const double lvarValue, const GsxStateStatus state)
    {
        return lvarValue == static_cast<double>(state);
    }

    bool IsServiceRunning(const double state)
    {
        return IsState(state, GsxStateStatus::Requested) || IsState(state, GsxStateStatus::Active);
    }
}

IFly737MaxDoorsFollowLoaderCycleRule::IFly737MaxDoorsFollowLoaderCycleRule(VariableReader& variables,
                                                                          const IFly737Max& aircraft)
    : variables_(&variables), aircraft_(&aircraft),
      fwdCargoDoor_{
          "FWD", kFwdCargoAnimLVar, gsx::lvars::kAircraftCargo1Toggle,
          gsx::lvars::kBaggageLoaderFrontState
      },
      aftCargoDoor_{
          "AFT", kAftCargoAnimLVar, gsx::lvars::kAircraftCargo2Toggle,
          gsx::lvars::kBaggageLoaderRearState
      },
      paxDoors_{
          Door{
              "1L", "ANIMATION_FWD_ENTRY_VAL", gsx::lvars::kAircraftExit1Toggle,
              gsx::lvars::kPassengerStairsFrontState, DoorKind::JetwayOrStairs
          },
          Door{
              "2L", "ANIMATION_AFT_ENTRY_VAL", gsx::lvars::kAircraftExit4Toggle,
              gsx::lvars::kPassengerStairsRearState, DoorKind::Stairs
          },
          Door{
              "1R", "ANIMATION_FWD_SERVICE_VAL", gsx::lvars::kAircraftService1Toggle,
              gsx::lvars::kCateringFrontState, DoorKind::Catering
          },
          Door{
              "2R", "ANIMATION_AFT_SERVICE_VAL", gsx::lvars::kAircraftService2Toggle,
              gsx::lvars::kCateringRearState, DoorKind::Catering
          }
      }
{
}

const char* IFly737MaxDoorsFollowLoaderCycleRule::Name() const
{
    return kRuleName;
}

RuleVerdict IFly737MaxDoorsFollowLoaderCycleRule::Evaluate(const RuleContext&)
{
    return RuleVerdict::Pass();
}

void IFly737MaxDoorsFollowLoaderCycleRule::Act(const RuleContext&, VariableWriter& writer)
{
    FollowAircraftCommands();

    const CargoCycle cycle = CurrentCargoCycle();

    if (cycle != CargoCycle::None && cycle != armedCycle_)
    {
        ArmCloser(cycle);
    }

    LatchCycleCompletion();

    for (Door* const door : AllDoors())
    {
        TrackDoor(*door);
    }

    const bool busy = AdvancePulses(writer);

    if (!busy && cycle == CargoCycle::None && !HasPendingCargoDoorWork())
    {
        DisarmCloser();
    }
}

void IFly737MaxDoorsFollowLoaderCycleRule::FollowAircraftCommands()
{
    const bool held = aircraft_->IsHeldForDeparture();
    const bool closeRequested = aircraft_->WasCloseRequested();

    if ((held && !heldSeen_) || (closeRequested && !closeRequestSeen_))
    {
        for (Door& door : paxDoors_)
        {
            ResetTracking(door);
        }
    }

    if (closeRequested)
    {
        closeRequested_ = true;
    }

    heldSeen_ = held;
    closeRequestSeen_ = closeRequested;
}

void IFly737MaxDoorsFollowLoaderCycleRule::ResetTracking(Door& door)
{
    door.servedSeen = false;
    door.loaderDone = false;
    door.attempts = 0;
}

void IFly737MaxDoorsFollowLoaderCycleRule::ArmCloser(const CargoCycle cycle)
{
    armedCycle_ = cycle;
    boardingCompleteSeen_ = false;
    deboardingCompleteSeen_ = false;
    boardingWasRunning_ = false;
    deboardingWasRunning_ = false;
    ResetTracking(fwdCargoDoor_);
    ResetTracking(aftCargoDoor_);
}

void IFly737MaxDoorsFollowLoaderCycleRule::DisarmCloser()
{
    armedCycle_ = CargoCycle::None;
}

void IFly737MaxDoorsFollowLoaderCycleRule::LatchCycleCompletion()
{
    if (armedCycle_ == CargoCycle::Boarding && HasCycleEnded(gsx::lvars::kBoardingState, boardingWasRunning_))
    {
        boardingCompleteSeen_ = true;
    }

    if (armedCycle_ == CargoCycle::Deboarding
        && HasCycleEnded(gsx::lvars::kDeboardingState, deboardingWasRunning_))
    {
        deboardingCompleteSeen_ = true;
    }
}

void IFly737MaxDoorsFollowLoaderCycleRule::TrackDoor(Door& door) const
{
    TrackDoorTravel(door);

    if (WantsOpen(door))
    {
        ++door.wantsOpenTicks;
    }
    else
    {
        door.wantsOpenTicks = 0;
        door.openAttempts = 0;
    }

    if (armedCycle_ == CargoCycle::Deboarding && door.kind == DoorKind::Cargo)
    {
        TrackBaggageLoader(door);
    }
}

void IFly737MaxDoorsFollowLoaderCycleRule::TrackDoorTravel(Door& door) const
{
    door.moving = variables_->HasLVarChangedThisTick(door.animLVar);
}

void IFly737MaxDoorsFollowLoaderCycleRule::TrackBaggageLoader(Door& door) const
{
    const double loaderState = variables_->GetLVar(door.loaderLVar, 0.0);

    if (loaderState == gsx::states::kLoaderUnloading || loaderState == gsx::states::kLoaderLoading)
    {
        door.servedSeen = true;
    }

    if (gsx::states::IsLoaderAtDoor(loaderState) || loaderState == gsx::states::kLoaderRetracting)
    {
        door.loaderDone = false;
        door.attempts = 0;

        return;
    }

    if (door.servedSeen)
    {
        door.loaderDone = true;
    }
}

bool IFly737MaxDoorsFollowLoaderCycleRule::AdvancePulses(VariableWriter& writer)
{
    bool busy = false;

    for (Door* const door : AllDoors())
    {
        busy = AdvancePulse(*door, writer) || busy;
    }

    return busy;
}

bool IFly737MaxDoorsFollowLoaderCycleRule::AdvancePulse(Door& door, VariableWriter& writer)
{
    if (door.pulseHigh)
    {
        writer.SetLVar(door.toggleLVar, kToggleReleased);
        door.pulseHigh = false;
        door.settleTicks = kPulseSettleTicks;

        return true;
    }

    if (door.settleTicks > 0)
    {
        --door.settleTicks;

        return true;
    }

    if (IsDoorOpenable(door))
    {
        return Pulse(door, writer, door.openAttempts, "reopening");
    }

    if (IsDoorCloseable(door))
    {
        return Pulse(door, writer, door.attempts, "closing");
    }

    return false;
}

bool IFly737MaxDoorsFollowLoaderCycleRule::Pulse(Door& door, VariableWriter& writer, int& attempts,
                                                 const char* verb) const
{
    ++attempts;
    writer.SetLVar(door.toggleLVar, kTogglePressed);
    door.pulseHigh = true;

    LOG_INFO("iFly: %s %s door (attempt %d/%d)",
             verb, door.doorName, attempts, kMaxDoorPulseAttempts);

    return true;
}

IFly737MaxDoorsFollowLoaderCycleRule::CargoCycle
IFly737MaxDoorsFollowLoaderCycleRule::CurrentCargoCycle() const
{
    if (IsStateActive(gsx::lvars::kBoardingState))
    {
        return CargoCycle::Boarding;
    }

    if (IsStateActive(gsx::lvars::kDeboardingState))
    {
        return CargoCycle::Deboarding;
    }

    return CargoCycle::None;
}

bool IFly737MaxDoorsFollowLoaderCycleRule::IsStateActive(const char* stateLVar) const
{
    return IsState(variables_->GetLVar(stateLVar, 0.0), GsxStateStatus::Active);
}

bool IFly737MaxDoorsFollowLoaderCycleRule::HasCycleEnded(const char* stateLVar, bool& wasRunning)
{
    const double state = variables_->GetLVar(stateLVar, 0.0);

    if (IsState(state, GsxStateStatus::Completed))
    {
        return true;
    }

    if (IsServiceRunning(state))
    {
        wasRunning = true;

        return false;
    }

    return wasRunning;
}

bool IFly737MaxDoorsFollowLoaderCycleRule::IsJetwayOnItsWay() const
{
    const double state = variables_->GetLVar(gsx::lvars::kOperateJetwaysState, 0.0);

    return IsServiceRunning(state);
}

bool IFly737MaxDoorsFollowLoaderCycleRule::HasPendingCargoDoorWork() const
{
    return IsBaggageLoaderPresent(fwdCargoDoor_.loaderLVar)
        || IsBaggageLoaderPresent(aftCargoDoor_.loaderLVar)
        || IsDoorClosePending(fwdCargoDoor_)
        || IsDoorClosePending(aftCargoDoor_);
}

bool IFly737MaxDoorsFollowLoaderCycleRule::IsBaggageLoaderPresent(const char* loaderLVar) const
{
    return variables_->HasReceivedLVar(loaderLVar)
        && gsx::states::IsLoaderPresent(variables_->GetLVar(loaderLVar, 0.0));
}

bool IFly737MaxDoorsFollowLoaderCycleRule::IsLoaderAtDoorNow(const Door& door) const
{
    return door.kind == DoorKind::Cargo && WantsOpen(door);
}

bool IFly737MaxDoorsFollowLoaderCycleRule::IsDoorReleased(const Door& door) const
{
    if (door.kind != DoorKind::Cargo)
    {
        return aircraft_->IsHeldForDeparture() || (closeRequested_ && !WantsOpen(door));
    }

    if (armedCycle_ == CargoCycle::Boarding)
    {
        return boardingCompleteSeen_ && !IsLoaderAtDoorNow(door);
    }

    return (door.loaderDone || (deboardingCompleteSeen_ && door.servedSeen))
        && !IsLoaderAtDoorNow(door);
}

bool IFly737MaxDoorsFollowLoaderCycleRule::IsDoorCloseable(const Door& door) const
{
    return IsDoorReleased(door)
        && !door.moving
        && door.attempts < kMaxDoorPulseAttempts
        && variables_->GetLVar(door.animLVar, 0.0) > kCargoDoorOpenThreshold;
}

bool IFly737MaxDoorsFollowLoaderCycleRule::IsDoorOpenable(const Door& door) const
{
    return !aircraft_->IsHeldForDeparture()
        && door.wantsOpenTicks >= kAircraftOpensItselfTicks
        && !door.moving
        && door.openAttempts < kMaxDoorPulseAttempts
        && variables_->HasReceivedLVar(door.animLVar)
        && variables_->GetLVar(door.animLVar, kCargoDoorFullyOpen) <= kCargoDoorOpenThreshold;
}

bool IFly737MaxDoorsFollowLoaderCycleRule::IsDoorClosePending(const Door& door) const
{
    if (armedCycle_ == CargoCycle::Boarding)
    {
        return !boardingCompleteSeen_ || IsDoorCloseable(door);
    }

    return (door.servedSeen && !door.loaderDone) || IsDoorCloseable(door);
}

bool IFly737MaxDoorsFollowLoaderCycleRule::WantsOpen(const Door& door) const
{
    const double equipment = variables_->GetLVar(door.loaderLVar, 0.0);

    switch (door.kind)
    {
    case DoorKind::JetwayOrStairs:
        return variables_->GetLVar(gsx::lvars::kJetway, 0.0) >= kJetwayAtAircraft
            || IsJetwayOnItsWay()
            || gsx::states::AreStairsArriving(equipment);
    case DoorKind::Stairs:
        return gsx::states::AreStairsArriving(equipment);
    case DoorKind::Catering:
        return gsx::states::IsCateringArriving(equipment);
    default:
        return gsx::states::IsLoaderAtDoor(equipment);
    }
}

std::array<IFly737MaxDoorsFollowLoaderCycleRule::Door*, 6>
IFly737MaxDoorsFollowLoaderCycleRule::AllDoors()
{
    return {
        &fwdCargoDoor_, &aftCargoDoor_,
        &paxDoors_[0], &paxDoors_[1], &paxDoors_[2], &paxDoors_[3]
    };
}
