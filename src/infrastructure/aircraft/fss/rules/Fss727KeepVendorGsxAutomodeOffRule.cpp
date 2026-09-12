#include "Fss727KeepVendorGsxAutomodeOffRule.h"

#include "../../../logging/LogMacros.h"
#include "../../../probe/ProbeLog.h"
#include "../../../simvars/VariableGateway.h"

namespace
{
    constexpr auto kRuleName = "fss-727-keep-vendor-gsx-automode-off";

    constexpr auto kAutomodeDisabledLVar = "FSS_B727_GSX_AUTOMODE_DISABLED";
    constexpr double kAutomodeDisabled = 1.0;

    constexpr int kTicksToWaitForTheEcho = 5;
}

Fss727KeepVendorGsxAutomodeOffRule::Fss727KeepVendorGsxAutomodeOffRule(VariableReader& variables)
    : variables_(&variables),
      ticksSinceWrite_(kTicksToWaitForTheEcho)
{
}

const char* Fss727KeepVendorGsxAutomodeOffRule::Name() const
{
    return kRuleName;
}

RuleVerdict Fss727KeepVendorGsxAutomodeOffRule::Evaluate(const RuleContext&)
{
    return RuleVerdict::Pass();
}

void Fss727KeepVendorGsxAutomodeOffRule::Act(const RuleContext&, VariableWriter& writer)
{
    if (IsAutomodeOff())
    {
        ticksSinceWrite_ = kTicksToWaitForTheEcho;

        return;
    }

    if (ticksSinceWrite_ < kTicksToWaitForTheEcho)
    {
        ++ticksSinceWrite_;

        return;
    }

    probe::Line(QStringLiteral("write automode FSS_B727_GSX_AUTOMODE_DISABLED=1"));
    writer.SetLVar(kAutomodeDisabledLVar, kAutomodeDisabled);
    ticksSinceWrite_ = 0;

    LOG_INFO("FSS 727 GSX auto mode turned off: the client drives the GSX menus");
}

bool Fss727KeepVendorGsxAutomodeOffRule::IsAutomodeOff() const
{
    return variables_->HasReceivedLVar(kAutomodeDisabledLVar)
        && variables_->GetLVar(kAutomodeDisabledLVar, 0.0) >= kAutomodeDisabled;
}
