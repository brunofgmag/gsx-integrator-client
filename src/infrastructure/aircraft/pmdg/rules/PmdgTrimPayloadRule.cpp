#include "PmdgTrimPayloadRule.h"

#include "../../../pmdg/PmdgDataGateway.h"
#include "../../../pmdg/PmdgPayloadWriter.h"

namespace
{
    constexpr auto kRuleName = "pmdg-trim-payload";
}

PmdgTrimPayloadRule::PmdgTrimPayloadRule(const PmdgDataGateway& data, PmdgPayloadWriter& payload)
    : data_(&data), payload_(&payload)
{
}

const char* PmdgTrimPayloadRule::Name() const
{
    return kRuleName;
}

RuleVerdict PmdgTrimPayloadRule::Evaluate(const RuleContext&)
{
    return RuleVerdict::Pass();
}

void PmdgTrimPayloadRule::Act(const RuleContext&, VariableWriter&)
{
    if (!data_->HasData())
    {
        return;
    }

    payload_->Trim();
}
