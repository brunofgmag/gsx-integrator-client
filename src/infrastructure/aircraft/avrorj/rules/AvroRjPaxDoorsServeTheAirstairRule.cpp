#include "AvroRjPaxDoorsServeTheAirstairRule.h"

#include <QtCore/QString>

#include "../AvroRj.h"
#include "../../../gsx/GsxDoorSync.h"
#include "../../../gsx/GsxLVars.h"
#include "../../../logging/LogMacros.h"
#include "../../../probe/ProbeLog.h"
#include "../../../simvars/VariableGateway.h"

namespace
{
    constexpr auto kRuleName = "avro-rj-pax-doors-serve-the-airstair";

    constexpr auto kFwdPaxDoorLVar = "EXT_Door_pax_1L";
    constexpr auto kAftPaxDoorLVar = "EXT_Door_pax_2L";
    constexpr auto kStairPositionLVar = "EXT_Door_stairs_pos";

    constexpr double kDoorOpen = 1.0;
    constexpr double kDoorClosed = 0.0;
    constexpr double kJetwayDocked = 5.0;
    constexpr double kJetwayUnavailable = 2.0;
    constexpr double kStairStowedPosition = 55.0;
}

AvroRjPaxDoorsServeTheAirstairRule::AvroRjPaxDoorsServeTheAirstairRule(VariableReader& variables,
                                                                      const AvroRj& aircraft,
                                                                      GsxDoorSync& doors,
                                                                      const AvroRjAirstairState& airstair)
    : variables_(&variables), aircraft_(&aircraft), doors_(&doors), airstair_(&airstair)
{
}

const char* AvroRjPaxDoorsServeTheAirstairRule::Name() const
{
    return kRuleName;
}

RuleVerdict AvroRjPaxDoorsServeTheAirstairRule::Evaluate(const RuleContext&)
{
    return RuleVerdict::Pass();
}

void AvroRjPaxDoorsServeTheAirstairRule::Act(const RuleContext&, VariableWriter& writer)
{
    DriveFrontDoor(writer);
    KeepAftDoorClosed(writer);
}

bool AvroRjPaxDoorsServeTheAirstairRule::IsFrontDoorWanted() const
{
    if (aircraft_->IsHeldForDeparture())
    {
        return false;
    }

    if (variables_->GetLVar(gsx::lvars::kCouatlStarted, 0.0) < 1.0)
    {
        return false;
    }

    if (doors_->VehicleState(gsx::lvars::kJetway, kJetwayUnavailable) == kJetwayDocked)
    {
        return true;
    }

    return airstair_->requested
        || gsx::states::AreStairsArriving(
            doors_->VehicleState(gsx::lvars::kPassengerStairsFrontState, 0.0));
}

void AvroRjPaxDoorsServeTheAirstairRule::DriveFrontDoor(VariableWriter& writer)
{
    doors_->Report();

    if (IsFrontDoorWanted())
    {
        if (lastFrontDoorTarget_ != kDoorOpen)
        {
            lastFrontDoorTarget_ = kDoorOpen;
            probe::Line(QStringLiteral("write front FwdPax open=1"));
            writer.SetLVar(kFwdPaxDoorLVar, kDoorOpen);
        }

        return;
    }

    if (lastFrontDoorTarget_ == kDoorOpen && airstair_->stowed
        && variables_->HasReceivedLVar(kStairPositionLVar)
        && variables_->GetLVar(kStairPositionLVar, 0.0) <= kStairStowedPosition)
    {
        lastFrontDoorTarget_ = kDoorClosed;
        probe::Line(QStringLiteral("write front FwdPax open=0"));
        writer.SetLVar(kFwdPaxDoorLVar, kDoorClosed);
    }
}

void AvroRjPaxDoorsServeTheAirstairRule::KeepAftDoorClosed(VariableWriter& writer)
{
    if (variables_->GetLVar(kAftPaxDoorLVar, 0.0) != kDoorOpen)
    {
        aftDoorCloseWritten_ = false;

        return;
    }

    if (aftDoorCloseWritten_)
    {
        return;
    }

    aftDoorCloseWritten_ = true;
    probe::Line(QStringLiteral("write aft AftPax open=0"));
    LOG_INFO("Closing the 2L: this aircraft boards through its own airstair at the 1L");
    writer.SetLVar(kAftPaxDoorLVar, kDoorClosed);
}
