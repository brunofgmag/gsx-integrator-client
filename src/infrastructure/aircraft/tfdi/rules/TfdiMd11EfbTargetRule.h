#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_TFDIMD11EFBTARGETRULE_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_TFDIMD11EFBTARGETRULE_H

#include "../../../../domain/ports/AircraftRule.h"

class TfdiMd11;

class TfdiMd11EfbTargetRule final : public AircraftRule
{
public:
    explicit TfdiMd11EfbTargetRule(TfdiMd11& aircraft);

    [[nodiscard]] const char* Name() const override;
    [[nodiscard]] RuleCadence Cadence() const override;
    [[nodiscard]] RuleVerdict Evaluate(const RuleContext& context) override;
    void Act(const RuleContext& context, VariableWriter& writer) override;

private:
    TfdiMd11* aircraft_;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_TFDIMD11EFBTARGETRULE_H
