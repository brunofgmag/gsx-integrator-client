#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_AVRORJWATCHMODULEFUELMIRRORRULE_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_AVRORJWATCHMODULEFUELMIRRORRULE_H

#include "../../../../domain/ports/AircraftRule.h"

class VariableReader;
struct AvroRjModuleState;

class AvroRjWatchModuleFuelMirrorRule final : public AircraftRule
{
public:
    AvroRjWatchModuleFuelMirrorRule(VariableReader& variables, AvroRjModuleState& module);

    [[nodiscard]] const char* Name() const override;
    [[nodiscard]] RuleVerdict Evaluate(const RuleContext& context) override;
    void Act(const RuleContext& context, VariableWriter& writer) override;

private:
    VariableReader* variables_;
    AvroRjModuleState* module_;
    double lastSimFuelKg_ = -1.0;
    int divergentTicks_ = 0;
    bool deadLogged_ = false;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_AVRORJWATCHMODULEFUELMIRRORRULE_H
