#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FSS727OWNGPUFOLLOWSTHEGSXUNITRULE_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FSS727OWNGPUFOLLOWSTHEGSXUNITRULE_H

#include "../../../../domain/ports/AircraftRule.h"

class GsxGateway;

class Fss727OwnGpuFollowsTheGsxUnitRule final : public AircraftRule
{
public:
    explicit Fss727OwnGpuFollowsTheGsxUnitRule(const GsxGateway* gsxGateway);

    [[nodiscard]] const char* Name() const override;
    [[nodiscard]] RuleVerdict Evaluate(const RuleContext& context) override;
    void Act(const RuleContext& context, VariableWriter& writer) override;

private:
    const GsxGateway* gsxGateway_;
    bool raised_ = false;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FSS727OWNGPUFOLLOWSTHEGSXUNITRULE_H
