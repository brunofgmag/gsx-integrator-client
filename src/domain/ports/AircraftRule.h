#ifndef GSX_INTEGRATOR_CLIENT_DOMAIN_AIRCRAFTRULE_H
#define GSX_INTEGRATOR_CLIENT_DOMAIN_AIRCRAFTRULE_H

#include "../turnaround/rules/RuleCadence.h"
#include "../turnaround/rules/RuleContext.h"
#include "../turnaround/rules/RuleVerdict.h"

class VariableWriter;

class AircraftRule
{
public:
    virtual ~AircraftRule() = default;

    [[nodiscard]] virtual const char* Name() const = 0;
    [[nodiscard]] virtual RuleCadence Cadence() const { return RuleCadence::Fast; }
    [[nodiscard]] virtual RuleVerdict Evaluate(const RuleContext& context) = 0;
    virtual void Act(const RuleContext& context, VariableWriter& writer) = 0;
};

#endif // GSX_INTEGRATOR_CLIENT_DOMAIN_AIRCRAFTRULE_H
