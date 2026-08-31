#include "AvroRjHoldForOwnAirstairRule.h"

#include <QtCore/QString>

#include "../AvroRj.h"
#include "../../../gsx/GsxDoorSync.h"
#include "../../../gsx/GsxLVars.h"
#include "../../../logging/LogMacros.h"
#include "../../../probe/ProbeLog.h"
#include "../../../simvars/VariableGateway.h"

namespace
{
    constexpr int kHoldTicks = 120;
    constexpr auto kRuleName = "avro-rj-hold-for-own-airstair";
    constexpr auto kHoldReason = "the Avro RJ is putting its own airstair out";

    constexpr auto kFwdPaxDoorLVar = "EXT_Door_pax_1L";
    constexpr double kDoorOpen = 1.0;

    constexpr auto kStairArmClickspotLVar = "VC_Stairs_clickspot_LC";
    constexpr auto kStairExtendSwitchLVar = "CAB_CTRLS_Fwd_StairRetract";
    constexpr auto kStairAccumPressureLVar = "Stairs_accum_press";
    constexpr auto kStairPositionLVar = "EXT_Door_stairs_pos";

    constexpr double kStairMinPressure = 500.0;
    constexpr double kStairSwitchExtended = 1.0;
    constexpr double kStairSwitchRetracted = 0.0;
    constexpr double kClickspotPressed = 1.0;
    constexpr double kStairStowedPosition = 55.0;
    constexpr int kStairSettledHoldTicks = 2;
}

AvroRjHoldForOwnAirstairRule::AvroRjHoldForOwnAirstairRule(VariableReader& variables, const AvroRj& aircraft,
                                                           GsxDoorSync& doors, AvroRjAirstairState& airstair)
    : variables_(&variables), aircraft_(&aircraft), doors_(&doors), airstair_(&airstair)
{
}

const char* AvroRjHoldForOwnAirstairRule::Name() const
{
    return kRuleName;
}

RuleVerdict AvroRjHoldForOwnAirstairRule::Evaluate(const RuleContext& context)
{
    ObserveTravel();

    if (!context.needs.passengerAccess || aircraft_->IsJetwayAvailable())
    {
        return RuleVerdict::Pass();
    }

    if (aircraft_->AreAirstairsSettled())
    {
        return RuleVerdict::Pass();
    }

    return RuleVerdict::Hold(kHoldTicks, kHoldReason);
}

void AvroRjHoldForOwnAirstairRule::Act(const RuleContext& context, VariableWriter& writer)
{
    if (context.needs.passengerAccess && !aircraft_->IsJetwayAvailable())
    {
        airstair_->requested = true;
    }

    Drive(writer);

    airstair_->stowed = phase_ == Phase::Stowed;
}

void AvroRjHoldForOwnAirstairRule::ObserveTravel()
{
    if (!variables_->HasReceivedLVar(kStairPositionLVar))
    {
        positionStillTicks_ = 0;
    }
    else
    {
        positionStillTicks_ = variables_->HasLVarChangedThisTick(kStairPositionLVar)
                                  ? 0
                                  : positionStillTicks_ + 1;
    }

    airstair_->settled = phase_ == Phase::Extended
        && IsOutOfItsWell()
        && positionStillTicks_ >= kStairSettledHoldTicks;
}

bool AvroRjHoldForOwnAirstairRule::IsWanted() const
{
    if (aircraft_->IsHeldForDeparture() || !airstair_->requested)
    {
        return false;
    }

    if (variables_->GetLVar(gsx::lvars::kCouatlStarted, 0.0) < 1.0)
    {
        return false;
    }

    if (aircraft_->IsJetwayAvailable())
    {
        return false;
    }

    return doors_->VehicleState(gsx::lvars::kPassengerStairsFrontState, 0.0)
        < gsx::states::kVehicleDispatched;
}

bool AvroRjHoldForOwnAirstairRule::IsOutOfItsWell() const
{
    return variables_->HasReceivedLVar(kStairPositionLVar)
        && variables_->GetLVar(kStairPositionLVar, 0.0) > kStairStowedPosition;
}

bool AvroRjHoldForOwnAirstairRule::IsMoving() const
{
    return variables_->HasReceivedLVar(kStairPositionLVar)
        && positionStillTicks_ < kStairSettledHoldTicks;
}

bool AvroRjHoldForOwnAirstairRule::HasPressure() const
{
    return variables_->GetLVar(kStairAccumPressureLVar, 0.0) >= kStairMinPressure;
}

bool AvroRjHoldForOwnAirstairRule::PressureReady()
{
    if (!HasPressure())
    {
        if (!pressureWaitLogged_)
        {
            pressureWaitLogged_ = true;
            LOG_INFO("Airstair is waiting: no accumulator pressure; the pilot recharges it with the AC pump");
        }

        return false;
    }

    pressureWaitLogged_ = false;

    return true;
}

void AvroRjHoldForOwnAirstairRule::Drive(VariableWriter& writer)
{
    if (IsMoving())
    {
        return;
    }

    const bool wanted = IsWanted();

    switch (phase_)
    {
    case Phase::Stowed:
        if (variables_->GetLVar(kStairExtendSwitchLVar, 0.0) == kStairSwitchExtended
            && IsOutOfItsWell())
        {
            phase_ = Phase::Extended;
            break;
        }

        if (!wanted || variables_->GetLVar(kFwdPaxDoorLVar, 0.0) != kDoorOpen)
        {
            break;
        }

        if (!PressureReady())
        {
            break;
        }

        probe::Line(QStringLiteral("write airstair arm clickspot=1"));
        writer.SetLVar(kStairArmClickspotLVar, kClickspotPressed);
        phase_ = Phase::Arming;
        break;
    case Phase::Arming:
        probe::Line(QStringLiteral("write airstair extend switch=1"));
        writer.SetLVar(kStairExtendSwitchLVar, kStairSwitchExtended);
        phase_ = Phase::Extended;
        break;
    case Phase::Extended:
        if (variables_->GetLVar(kStairExtendSwitchLVar, 0.0) == kStairSwitchRetracted)
        {
            phase_ = Phase::Stowed;
            break;
        }

        if (!wanted)
        {
            if (!PressureReady())
            {
                break;
            }

            probe::Line(QStringLiteral("write airstair retract switch=0"));
            writer.SetLVar(kStairExtendSwitchLVar, kStairSwitchRetracted);
            phase_ = Phase::Unarming;
        }

        break;
    case Phase::Unarming:
        probe::Line(QStringLiteral("write airstair stow clickspot=1"));
        writer.SetLVar(kStairArmClickspotLVar, kClickspotPressed);
        phase_ = Phase::Stowed;
        break;
    }
}
