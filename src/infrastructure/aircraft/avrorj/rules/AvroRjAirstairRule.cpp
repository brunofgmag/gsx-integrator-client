#include "AvroRjAirstairRule.h"

#include "../AvroRj.h"

namespace
{
    constexpr int kHoldTicks = 120;
    constexpr auto kRuleName = "avro-rj-airstair";
    constexpr auto kHoldReason = "the Avro RJ is putting its own airstair out";
}

AvroRjAirstairRule::AvroRjAirstairRule(AvroRj& aircraft) : aircraft_(&aircraft)
{
}

const char* AvroRjAirstairRule::Name() const
{
    return kRuleName;
}

RuleVerdict AvroRjAirstairRule::Evaluate(const RuleContext& context)
{
    aircraft_->ObserveAirstairTravel();

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

void AvroRjAirstairRule::Act(const RuleContext& context, VariableWriter&)
{
    if (context.needs.passengerAccess && !aircraft_->IsJetwayAvailable())
    {
        aircraft_->WantAirstairs(true);
    }

    aircraft_->DriveAirstair();
}
