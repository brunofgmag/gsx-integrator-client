#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_IFLY737MAXWATCHPLANFILERULE_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_IFLY737MAXWATCHPLANFILERULE_H

#include <filesystem>
#include <optional>

#include "../../../../domain/ports/AircraftRule.h"

class IFlyPlanImport;
struct AutomationStatus;

class IFly737MaxWatchPlanFileRule final : public AircraftRule
{
public:
    IFly737MaxWatchPlanFileRule(IFlyPlanImport& planImport, const AutomationStatus& status,
                                std::optional<std::filesystem::path> appDataRoot);

    [[nodiscard]] const char* Name() const override;
    [[nodiscard]] RuleCadence Cadence() const override;
    [[nodiscard]] RuleVerdict Evaluate(const RuleContext& context) override;
    void Act(const RuleContext& context, VariableWriter& writer) override;

private:
    [[nodiscard]] std::optional<std::filesystem::path> PlanDirectory() const;

    IFlyPlanImport* planImport_;
    const AutomationStatus* status_;
    std::optional<std::filesystem::path> appDataRoot_;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_IFLY737MAXWATCHPLANFILERULE_H
