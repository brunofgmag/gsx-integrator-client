#include "PmdgRetryGroundConnUntilSetRule.h"

#include "../../../pmdg/PmdgDataGateway.h"
#include "../../../pmdg/PmdgGroundConnReconciler.h"

namespace
{
    constexpr auto kRuleName = "pmdg-retry-ground-conn-until-set";
}

PmdgRetryGroundConnUntilSetRule::PmdgRetryGroundConnUntilSetRule(const PmdgDataGateway& data,
                                                                 PmdgGroundConnReconciler& groundConn)
    : data_(&data), groundConn_(&groundConn)
{
}

const char* PmdgRetryGroundConnUntilSetRule::Name() const
{
    return kRuleName;
}

RuleVerdict PmdgRetryGroundConnUntilSetRule::Evaluate(const RuleContext&)
{
    return RuleVerdict::Pass();
}

void PmdgRetryGroundConnUntilSetRule::Act(const RuleContext&, VariableWriter&)
{
    if (!data_->HasData())
    {
        return;
    }

    groundConn_->Reconcile();
}
