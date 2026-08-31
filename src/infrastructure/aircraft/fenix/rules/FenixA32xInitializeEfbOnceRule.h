#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FENIXA32XINITIALIZEEFBONCERULE_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FENIXA32XINITIALIZEEFBONCERULE_H

#include "../../../../domain/ports/AircraftRule.h"

class FenixEfbGateway;

class FenixA32xInitializeEfbOnceRule final : public AircraftRule
{
public:
    explicit FenixA32xInitializeEfbOnceRule(FenixEfbGateway& efb);

    [[nodiscard]] const char* Name() const override;
    [[nodiscard]] RuleVerdict Evaluate(const RuleContext& context) override;
    void Act(const RuleContext& context, VariableWriter& writer) override;

private:
    FenixEfbGateway* efb_;
    bool initialized_ = false;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FENIXA32XINITIALIZEEFBONCERULE_H
