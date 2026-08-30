#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_IFLY737MAXPLANIMPORTRULE_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_IFLY737MAXPLANIMPORTRULE_H

#include "../../../../domain/ports/AircraftRule.h"

class IFly737Max;

class IFly737MaxPlanImportRule final : public AircraftRule
{
public:
    explicit IFly737MaxPlanImportRule(IFly737Max& aircraft);

    [[nodiscard]] const char* Name() const override;
    [[nodiscard]] RuleCadence Cadence() const override;
    [[nodiscard]] RuleVerdict Evaluate(const RuleContext& context) override;
    void Act(const RuleContext& context, VariableWriter& writer) override;

private:
    IFly737Max* aircraft_;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_IFLY737MAXPLANIMPORTRULE_H
