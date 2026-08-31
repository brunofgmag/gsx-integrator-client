#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FENIXA32XDISARMREFUELWHENDONERULE_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FENIXA32XDISARMREFUELWHENDONERULE_H

#include "../../../../domain/ports/AircraftRule.h"

class VariableReader;

class FenixA32xDisarmRefuelWhenDoneRule final : public AircraftRule
{
public:
    explicit FenixA32xDisarmRefuelWhenDoneRule(VariableReader& variables);

    [[nodiscard]] const char* Name() const override;
    [[nodiscard]] RuleVerdict Evaluate(const RuleContext& context) override;
    void Act(const RuleContext& context, VariableWriter& writer) override;

private:
    VariableReader* variables_;
    bool armed_ = false;
    bool loadingSeen_ = false;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FENIXA32XDISARMREFUELWHENDONERULE_H
