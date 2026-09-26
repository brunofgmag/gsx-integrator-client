#include "FssEJetKeepVendorAutomationOffRule.h"

#include <cstddef>
#include <QtCore/QString>

#include "../../../logging/LogMacros.h"
#include "../../../probe/ProbeLog.h"
#include "../../../simvars/VariableGateway.h"

namespace
{
    constexpr auto kRuleName = "fss-ejet-keep-vendor-automation-off";

    constexpr std::array kAutomationLVars = {
        "FSS_GNDSVC_AUTO_DEPARTURE", "FSS_GNDSVC_AUTO_DEBOARDING", "FSS_GNDSVC_AUTO_NOTIFY_GOOD_ENGSTART"
    };
    constexpr double kAutomationOff = 0.0;

    constexpr int kTicksToWaitForTheEcho = 5;
}

FssEJetKeepVendorAutomationOffRule::FssEJetKeepVendorAutomationOffRule(VariableReader& variables)
    : variables_(&variables),
      ticksSinceWrite_{kTicksToWaitForTheEcho, kTicksToWaitForTheEcho, kTicksToWaitForTheEcho}
{
}

const char* FssEJetKeepVendorAutomationOffRule::Name() const
{
    return kRuleName;
}

RuleVerdict FssEJetKeepVendorAutomationOffRule::Evaluate(const RuleContext&)
{
    return RuleVerdict::Pass();
}

void FssEJetKeepVendorAutomationOffRule::Act(const RuleContext&, VariableWriter& writer)
{
    for (std::size_t i = 0; i < kAutomationLVars.size(); ++i)
    {
        const char* lVar = kAutomationLVars[i];
        int& ticksSinceWrite = ticksSinceWrite_[i];

        if (IsOff(lVar))
        {
            ticksSinceWrite = kTicksToWaitForTheEcho;

            continue;
        }

        if (ticksSinceWrite < kTicksToWaitForTheEcho)
        {
            ++ticksSinceWrite;

            continue;
        }

        probe::Line(probe::Channel::Writes, QStringLiteral("write automation %1=0").arg(lVar));
        writer.SetLVar(lVar, kAutomationOff);
        ticksSinceWrite = 0;

        LOG_INFO("FSS E-Jet vendor ground service automation turned off: %s", lVar);
    }
}

bool FssEJetKeepVendorAutomationOffRule::IsOff(const char* lVar) const
{
    return variables_->HasReceivedLVar(lVar) && variables_->GetLVar(lVar, 0.0) <= kAutomationOff;
}
