#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_TOLISSA340ADVANCEMCDUUPLINKRULE_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_TOLISSA340ADVANCEMCDUUPLINKRULE_H

#include "../../../../domain/ports/AircraftRule.h"

class TolissA340;

class TolissA340AdvanceMcduUplinkRule final : public AircraftRule
{
public:
    explicit TolissA340AdvanceMcduUplinkRule(const TolissA340& aircraft);

    [[nodiscard]] const char* Name() const override;
    [[nodiscard]] RuleVerdict Evaluate(const RuleContext& context) override;
    void Act(const RuleContext& context, VariableWriter& writer) override;

private:
    const TolissA340* aircraft_;
    bool loadingSeen_ = false;
    int step_ = -1;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_TOLISSA340ADVANCEMCDUUPLINKRULE_H
