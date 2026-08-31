#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_TFDIMD11COMMITEFBTARGETSRULE_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_TFDIMD11COMMITEFBTARGETSRULE_H

#include "../../../../domain/ports/AircraftRule.h"

class TfdiMd11;
class VariableReader;

class TfdiMd11CommitEfbTargetsRule final : public AircraftRule
{
public:
    TfdiMd11CommitEfbTargetsRule(VariableReader& variables, const TfdiMd11& aircraft);

    [[nodiscard]] const char* Name() const override;
    [[nodiscard]] RuleCadence Cadence() const override;
    [[nodiscard]] RuleVerdict Evaluate(const RuleContext& context) override;
    void Act(const RuleContext& context, VariableWriter& writer) override;

private:
    struct EfbTarget
    {
        double value = 0.0;
        bool seeded = false;
    };

    void SeedFuelIfNeeded();
    void SeedZfwIfNeeded();
    void CommitTargets(VariableWriter& writer) const;

    VariableReader* variables_;
    const TfdiMd11* aircraft_;
    EfbTarget fuelTarget_;
    EfbTarget zfwTarget_;
    double committedFuelKg_ = 0.0;
    double committedZfwKg_ = 0.0;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_TFDIMD11COMMITEFBTARGETSRULE_H
