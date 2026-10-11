#include "Fss727KeepGsxDoorAutomationOffRule.h"

#include <QtCore/QString>

#include "../../../gsx/GsxLVars.h"
#include "../../../probe/ProbeLog.h"
#include "../../../simvars/VariableGateway.h"

namespace
{
    constexpr auto kRuleName = "fss-727-keep-gsx-door-automation-off";

    constexpr double kGsxDoorAutomationOn = 1.0;
    constexpr double kGsxDoorAutomationOff = 0.0;

    constexpr int kTicksToWaitForTheEcho = 5;
}

Fss727KeepGsxDoorAutomationOffRule::Fss727KeepGsxDoorAutomationOffRule(VariableReader& variables)
    : variables_(&variables),
      echoWait_(kTicksToWaitForTheEcho)
{
}

const char* Fss727KeepGsxDoorAutomationOffRule::Name() const
{
    return kRuleName;
}

RuleVerdict Fss727KeepGsxDoorAutomationOffRule::Evaluate(const RuleContext&)
{
    return RuleVerdict::Pass();
}

void Fss727KeepGsxDoorAutomationOffRule::Act(const RuleContext&, VariableWriter& writer)
{
    if (!echoWait_.WriteIsDue(IsAutomationOff()))
    {
        return;
    }

    probe::Line(probe::Channel::Writes, QStringLiteral("write automation FSDT_GSX_AUTOMATION_DOORS=0"));
    writer.SetLVar(gsx::lvars::kAutomationDoors, kGsxDoorAutomationOff);
}

bool Fss727KeepGsxDoorAutomationOffRule::IsAutomationOff() const
{
    return variables_->GetLVar(gsx::lvars::kAutomationDoors, kGsxDoorAutomationOn) == kGsxDoorAutomationOff;
}
