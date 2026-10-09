#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_AVRORJWATCHMODULEFUELMIRRORRULE_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_AVRORJWATCHMODULEFUELMIRRORRULE_H

#include "../../../../domain/ports/AircraftRule.h"

class VariableReader;

class AvroRjWatchModuleFuelMirrorRule final : public AircraftRule
{
public:
    explicit AvroRjWatchModuleFuelMirrorRule(VariableReader& variables);

    [[nodiscard]] const char* Name() const override;
    [[nodiscard]] RuleVerdict Evaluate(const RuleContext& context) override;
    void Act(const RuleContext& context, VariableWriter& writer) override;

private:
    VariableReader* variables_;
    double lastSimFuelKg_ = -1.0;
    int divergentTicks_ = 0;
    bool deadLogged_ = false;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_AVRORJWATCHMODULEFUELMIRRORRULE_H
