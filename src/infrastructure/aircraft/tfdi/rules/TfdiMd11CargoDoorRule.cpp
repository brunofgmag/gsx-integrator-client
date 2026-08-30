#include "TfdiMd11CargoDoorRule.h"

#include "../TfdiMd11.h"

namespace
{
    constexpr auto kRuleName = "tfdi-md11-cargo-doors";
}

TfdiMd11CargoDoorRule::TfdiMd11CargoDoorRule(TfdiMd11& aircraft) : aircraft_(&aircraft)
{
}

const char* TfdiMd11CargoDoorRule::Name() const
{
    return kRuleName;
}

RuleVerdict TfdiMd11CargoDoorRule::Evaluate(const RuleContext&)
{
    return RuleVerdict::Pass();
}

void TfdiMd11CargoDoorRule::Act(const RuleContext&, VariableWriter&)
{
    aircraft_->DriveCargoDoors();
}
