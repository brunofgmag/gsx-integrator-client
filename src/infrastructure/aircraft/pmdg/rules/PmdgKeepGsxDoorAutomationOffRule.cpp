#include "PmdgKeepGsxDoorAutomationOffRule.h"

#include "../../../gsx/GsxLVars.h"
#include "../../../pmdg/PmdgDataGateway.h"
#include "../../../simvars/VariableGateway.h"

namespace
{
    constexpr auto kRuleName = "pmdg-keep-gsx-door-automation-off";

    constexpr double kGsxDoorAutomationOn = 1.0;
    constexpr double kGsxDoorAutomationOff = 0.0;

    constexpr int kTicksToWaitForTheEcho = 5;
}

PmdgKeepGsxDoorAutomationOffRule::PmdgKeepGsxDoorAutomationOffRule(VariableReader& variables,
                                                                   const PmdgDataGateway& data)
    : variables_(&variables),
      data_(&data),
      echoWait_(kTicksToWaitForTheEcho)
{
}

const char* PmdgKeepGsxDoorAutomationOffRule::Name() const
{
    return kRuleName;
}

RuleVerdict PmdgKeepGsxDoorAutomationOffRule::Evaluate(const RuleContext&)
{
    return RuleVerdict::Pass();
}

void PmdgKeepGsxDoorAutomationOffRule::Act(const RuleContext&, VariableWriter& writer)
{
    if (!data_->HasData())
    {
        return;
    }

    if (!echoWait_.WriteIsDue(IsAutomationOff()))
    {
        return;
    }

    writer.SetLVar(gsx::lvars::kAutomationDoors, kGsxDoorAutomationOff);
}

bool PmdgKeepGsxDoorAutomationOffRule::IsAutomationOff() const
{
    return variables_->GetLVar(gsx::lvars::kAutomationDoors, kGsxDoorAutomationOn) == kGsxDoorAutomationOff;
}
