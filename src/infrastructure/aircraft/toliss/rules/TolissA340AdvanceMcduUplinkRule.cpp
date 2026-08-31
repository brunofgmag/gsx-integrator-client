#include "TolissA340AdvanceMcduUplinkRule.h"

#include <array>

#include "../TolissA340.h"
#include "../../../logging/LogMacros.h"
#include "../../../simvars/VariableGateway.h"

namespace
{
    constexpr auto kRuleName = "toliss-a340-advance-mcdu-uplink";

    constexpr std::array kMcduUplinkKeys =
        {"AB_MCDU3_MENU", "AB_MCDU3_LSK6L", "AB_MCDU3_LSK1R", "AB_MCDU3_LSK1L"};

    constexpr double kKeyPressed = 1.0;
}

TolissA340AdvanceMcduUplinkRule::TolissA340AdvanceMcduUplinkRule(const TolissA340& aircraft)
    : aircraft_(&aircraft)
{
}

const char* TolissA340AdvanceMcduUplinkRule::Name() const
{
    return kRuleName;
}

RuleVerdict TolissA340AdvanceMcduUplinkRule::Evaluate(const RuleContext&)
{
    return RuleVerdict::Pass();
}

void TolissA340AdvanceMcduUplinkRule::Act(const RuleContext& context, VariableWriter& writer)
{
    if (!context.needs.loading)
    {
        loadingSeen_ = false;
        step_ = -1;

        return;
    }

    if (!loadingSeen_)
    {
        loadingSeen_ = true;
        step_ = 0;

        LOG_INFO("SimBrief uplink armed: waiting for the MCDU to be available");
    }

    if (!aircraft_->IsPowered() || step_ < 0 || step_ >= static_cast<int>(kMcduUplinkKeys.size()))
    {
        return;
    }

    if (step_ == 0)
    {
        LOG_INFO("Starting SimBrief uplink through the center MCDU");
    }

    writer.SetLVar(kMcduUplinkKeys[step_], kKeyPressed);
    ++step_;
}
