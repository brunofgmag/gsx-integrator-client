#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDGDOORRULE_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDGDOORRULE_H

#include "../../../../domain/ports/AircraftRule.h"

class PmdgAircraft;

class PmdgDoorRule final : public AircraftRule
{
public:
    explicit PmdgDoorRule(PmdgAircraft& aircraft);

    [[nodiscard]] const char* Name() const override;
    [[nodiscard]] RuleVerdict Evaluate(const RuleContext& context) override;
    void Act(const RuleContext& context, VariableWriter& writer) override;

private:
    PmdgAircraft* aircraft_;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDGDOORRULE_H
