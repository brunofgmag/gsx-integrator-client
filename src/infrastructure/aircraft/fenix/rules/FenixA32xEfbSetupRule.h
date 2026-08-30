#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FENIXA32XEFBSETUPRULE_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FENIXA32XEFBSETUPRULE_H

#include "../../../../domain/ports/AircraftRule.h"

class FenixA32x;

class FenixA32xEfbSetupRule final : public AircraftRule
{
public:
    explicit FenixA32xEfbSetupRule(FenixA32x& aircraft);

    [[nodiscard]] const char* Name() const override;
    [[nodiscard]] RuleVerdict Evaluate(const RuleContext& context) override;
    void Act(const RuleContext& context, VariableWriter& writer) override;

private:
    FenixA32x* aircraft_;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FENIXA32XEFBSETUPRULE_H
