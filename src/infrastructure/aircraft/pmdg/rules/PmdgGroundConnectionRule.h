#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDGGROUNDCONNECTIONRULE_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDGGROUNDCONNECTIONRULE_H

#include "../../../../domain/ports/AircraftRule.h"

class PmdgAircraft;

class PmdgGroundConnectionRule final : public AircraftRule
{
public:
    explicit PmdgGroundConnectionRule(PmdgAircraft& aircraft);

    [[nodiscard]] const char* Name() const override;
    [[nodiscard]] RuleVerdict Evaluate(const RuleContext& context) override;
    void Act(const RuleContext& context, VariableWriter& writer) override;

private:
    PmdgAircraft* aircraft_;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDGGROUNDCONNECTIONRULE_H
