#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDGKEEPGSXDOORAUTOMATIONOFFRULE_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDGKEEPGSXDOORAUTOMATIONOFFRULE_H

#include "../../../../domain/ports/AircraftRule.h"
#include "../../EchoWait.h"

class PmdgDataGateway;
class VariableReader;

class PmdgKeepGsxDoorAutomationOffRule final : public AircraftRule
{
public:
    PmdgKeepGsxDoorAutomationOffRule(VariableReader& variables, const PmdgDataGateway& data);

    [[nodiscard]] const char* Name() const override;
    [[nodiscard]] RuleVerdict Evaluate(const RuleContext& context) override;
    void Act(const RuleContext& context, VariableWriter& writer) override;

private:
    [[nodiscard]] bool IsAutomationOff() const;

    VariableReader* variables_;
    const PmdgDataGateway* data_;
    EchoWait echoWait_;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDGKEEPGSXDOORAUTOMATIONOFFRULE_H
