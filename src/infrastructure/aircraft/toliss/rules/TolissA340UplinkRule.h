#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_TOLISSA340UPLINKRULE_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_TOLISSA340UPLINKRULE_H

#include "../../../../domain/ports/AircraftRule.h"

class TolissA340;

class TolissA340UplinkRule final : public AircraftRule
{
public:
    explicit TolissA340UplinkRule(TolissA340& aircraft);

    [[nodiscard]] const char* Name() const override;
    [[nodiscard]] RuleVerdict Evaluate(const RuleContext& context) override;
    void Act(const RuleContext& context, VariableWriter& writer) override;

private:
    TolissA340* aircraft_;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_TOLISSA340UPLINKRULE_H
