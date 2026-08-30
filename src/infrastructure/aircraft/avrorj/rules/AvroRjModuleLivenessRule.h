#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_AVRORJMODULELIVENESSRULE_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_AVRORJMODULELIVENESSRULE_H

#include "../../../../domain/ports/AircraftRule.h"

class AvroRj;

class AvroRjModuleLivenessRule final : public AircraftRule
{
public:
    explicit AvroRjModuleLivenessRule(AvroRj& aircraft);

    [[nodiscard]] const char* Name() const override;
    [[nodiscard]] RuleVerdict Evaluate(const RuleContext& context) override;
    void Act(const RuleContext& context, VariableWriter& writer) override;

private:
    AvroRj* aircraft_;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_AVRORJMODULELIVENESSRULE_H
