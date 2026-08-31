#include "FenixA32xInitializeEfbOnceRule.h"

#include <array>

#include "../../../fenix/FenixEfbGateway.h"
#include "../../../logging/LogMacros.h"

namespace
{
    constexpr auto kRuleName = "fenix-a32x-initialize-efb-once";

    constexpr std::array kAutomationTogglesToDisable = {
        "fenix.efb.autoDoor",
        "fenix.efb.autoJetway",
        "fenix.gsx.autoConnectGpu",
        "fenix.gsx.autoDisconnectGpu",
        "fenix.gsx.autoDeboard",
        "fenix.gsx.autoCatering",
        "fenix.gsx.autoPushback",
        "fenix.gsx.autoSelectOperator"
    };
}

FenixA32xInitializeEfbOnceRule::FenixA32xInitializeEfbOnceRule(FenixEfbGateway& efb) : efb_(&efb)
{
}

const char* FenixA32xInitializeEfbOnceRule::Name() const
{
    return kRuleName;
}

RuleVerdict FenixA32xInitializeEfbOnceRule::Evaluate(const RuleContext&)
{
    return RuleVerdict::Pass();
}

void FenixA32xInitializeEfbOnceRule::Act(const RuleContext&, VariableWriter&)
{
    if (!efb_->IsAvailable())
    {
        initialized_ = false;

        return;
    }

    if (initialized_)
    {
        return;
    }

    initialized_ = true;
    for (const auto* const toggle : kAutomationTogglesToDisable)
    {
        efb_->SetBool(toggle, false);
    }

    LOG_INFO("Fenix EFB automation disabled: the client now controls doors and GSX services");
}
