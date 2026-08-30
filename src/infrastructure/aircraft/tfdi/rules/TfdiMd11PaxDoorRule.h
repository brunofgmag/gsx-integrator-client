#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_TFDIMD11PAXDOORRULE_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_TFDIMD11PAXDOORRULE_H

#include "../../../../domain/ports/AircraftRule.h"

class TfdiMd11;

class TfdiMd11PaxDoorRule final : public AircraftRule
{
public:
    explicit TfdiMd11PaxDoorRule(TfdiMd11& aircraft);

    [[nodiscard]] const char* Name() const override;
    [[nodiscard]] RuleVerdict Evaluate(const RuleContext& context) override;
    void Act(const RuleContext& context, VariableWriter& writer) override;

private:
    TfdiMd11* aircraft_;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_TFDIMD11PAXDOORRULE_H
