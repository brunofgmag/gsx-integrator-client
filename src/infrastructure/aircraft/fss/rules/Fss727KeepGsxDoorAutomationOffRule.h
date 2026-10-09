#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FSS727KEEPGSXDOORAUTOMATIONOFFRULE_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FSS727KEEPGSXDOORAUTOMATIONOFFRULE_H

#include "../../../../domain/ports/AircraftRule.h"
#include "../../EchoWait.h"

class VariableReader;

class Fss727KeepGsxDoorAutomationOffRule final : public AircraftRule
{
public:
    explicit Fss727KeepGsxDoorAutomationOffRule(VariableReader& variables);

    [[nodiscard]] const char* Name() const override;
    [[nodiscard]] RuleVerdict Evaluate(const RuleContext& context) override;
    void Act(const RuleContext& context, VariableWriter& writer) override;

private:
    [[nodiscard]] bool IsAutomationOff() const;

    VariableReader* variables_;
    EchoWait echoWait_;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FSS727KEEPGSXDOORAUTOMATIONOFFRULE_H
