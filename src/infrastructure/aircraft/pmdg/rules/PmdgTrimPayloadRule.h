#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDGTRIMPAYLOADRULE_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDGTRIMPAYLOADRULE_H

#include "../../../../domain/ports/AircraftRule.h"

class PmdgDataGateway;
class PmdgPayloadWriter;

class PmdgTrimPayloadRule final : public AircraftRule
{
public:
    PmdgTrimPayloadRule(const PmdgDataGateway& data, PmdgPayloadWriter& payload);

    [[nodiscard]] const char* Name() const override;
    [[nodiscard]] RuleVerdict Evaluate(const RuleContext& context) override;
    void Act(const RuleContext& context, VariableWriter& writer) override;

private:
    const PmdgDataGateway* data_;
    PmdgPayloadWriter* payload_;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDGTRIMPAYLOADRULE_H
