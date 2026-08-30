#include "AvroRjModuleLivenessRule.h"

#include "../AvroRj.h"

namespace
{
    constexpr auto kRuleName = "avro-rj-module-liveness";
}

AvroRjModuleLivenessRule::AvroRjModuleLivenessRule(AvroRj& aircraft) : aircraft_(&aircraft)
{
}

const char* AvroRjModuleLivenessRule::Name() const
{
    return kRuleName;
}

RuleVerdict AvroRjModuleLivenessRule::Evaluate(const RuleContext&)
{
    aircraft_->ObserveModuleLiveness();

    return RuleVerdict::Pass();
}

void AvroRjModuleLivenessRule::Act(const RuleContext&, VariableWriter&)
{
}
