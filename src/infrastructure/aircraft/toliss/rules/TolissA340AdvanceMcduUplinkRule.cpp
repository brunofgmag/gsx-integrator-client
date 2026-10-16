#include "TolissA340AdvanceMcduUplinkRule.h"

#include <array>
#include <utility>

#include "../TolissA340.h"
#include "../../../logging/LogMacros.h"
#include "../../../simvars/VariableGateway.h"

namespace
{
    constexpr auto kRuleName = "toliss-a340-advance-mcdu-uplink";

    constexpr int kAtsuPageSettleTicks = 2;

    struct McduUplinkKey
    {
        const char* lvar;
        int settleTicks;
    };

    constexpr std::array kMcduUplinkKeys =
        {McduUplinkKey{.lvar = "AB_MCDU3_MENU", .settleTicks = 0},
         McduUplinkKey{.lvar = "AB_MCDU3_LSK6L", .settleTicks = kAtsuPageSettleTicks},
         McduUplinkKey{.lvar = "AB_MCDU3_LSK1R", .settleTicks = 0},
         McduUplinkKey{.lvar = "AB_MCDU3_LSK1L", .settleTicks = 0}};

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

void TolissA340AdvanceMcduUplinkRule::ResumeWithTheUplinkDone()
{
    loadingSeen_ = true;
    step_ = static_cast<int>(kMcduUplinkKeys.size());
    settleTicksLeft_ = 0;
}

void TolissA340AdvanceMcduUplinkRule::Act(const RuleContext& context, VariableWriter& writer)
{
    if (!context.needs.loading)
    {
        loadingSeen_ = false;
        step_ = -1;
        settleTicksLeft_ = 0;

        return;
    }

    if (!loadingSeen_)
    {
        loadingSeen_ = true;
        step_ = 0;

        LOG_INFO("SimBrief uplink armed: waiting for the MCDU to be available");
    }

    if (!aircraft_->IsPowered() || step_ < 0 || std::cmp_greater_equal(step_, kMcduUplinkKeys.size()))
    {
        return;
    }

    if (settleTicksLeft_ > 0)
    {
        --settleTicksLeft_;

        return;
    }

    if (step_ == 0)
    {
        LOG_INFO("Starting SimBrief uplink through the center MCDU");
    }

    const McduUplinkKey& key = kMcduUplinkKeys[step_];

    writer.SetLVar(key.lvar, kKeyPressed);
    settleTicksLeft_ = key.settleTicks;
    ++step_;
}
