#include "AvroRjDoorRule.h"

#include "../AvroRj.h"

namespace
{
    constexpr auto kRuleName = "avro-rj-doors";
}

AvroRjDoorRule::AvroRjDoorRule(AvroRj& aircraft) : aircraft_(&aircraft)
{
}

const char* AvroRjDoorRule::Name() const
{
    return kRuleName;
}

RuleVerdict AvroRjDoorRule::Evaluate(const RuleContext&)
{
    return RuleVerdict::Pass();
}

void AvroRjDoorRule::Act(const RuleContext&, VariableWriter&)
{
    aircraft_->DriveDoors();
}
