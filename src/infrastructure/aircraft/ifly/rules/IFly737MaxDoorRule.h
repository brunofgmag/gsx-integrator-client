#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_IFLY737MAXDOORRULE_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_IFLY737MAXDOORRULE_H

#include "../../../../domain/ports/AircraftRule.h"

class IFly737Max;

class IFly737MaxDoorRule final : public AircraftRule
{
public:
    explicit IFly737MaxDoorRule(IFly737Max& aircraft);

    [[nodiscard]] const char* Name() const override;
    [[nodiscard]] RuleVerdict Evaluate(const RuleContext& context) override;
    void Act(const RuleContext& context, VariableWriter& writer) override;

private:
    IFly737Max* aircraft_;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_IFLY737MAXDOORRULE_H
