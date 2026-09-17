#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FSSEJETKEEPVENDORAUTOMATIONOFFRULE_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FSSEJETKEEPVENDORAUTOMATIONOFFRULE_H

#include <array>

#include "../../../../domain/ports/AircraftRule.h"

class VariableReader;

class FssEJetKeepVendorAutomationOffRule final : public AircraftRule
{
public:
    explicit FssEJetKeepVendorAutomationOffRule(VariableReader& variables);

    [[nodiscard]] const char* Name() const override;
    [[nodiscard]] RuleVerdict Evaluate(const RuleContext& context) override;
    void Act(const RuleContext& context, VariableWriter& writer) override;

private:
    [[nodiscard]] bool IsOff(const char* lVar) const;

    VariableReader* variables_;
    std::array<int, 3> ticksSinceWrite_;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FSSEJETKEEPVENDORAUTOMATIONOFFRULE_H
