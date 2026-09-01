#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDGRETRYGROUNDCONNUNTILSETRULE_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDGRETRYGROUNDCONNUNTILSETRULE_H

#include "../../../../domain/ports/AircraftRule.h"

class PmdgDataGateway;
class PmdgGroundConnReconciler;

class PmdgRetryGroundConnUntilSetRule final : public AircraftRule
{
public:
    PmdgRetryGroundConnUntilSetRule(const PmdgDataGateway& data, PmdgGroundConnReconciler& groundConn);

    [[nodiscard]] const char* Name() const override;
    [[nodiscard]] RuleVerdict Evaluate(const RuleContext& context) override;
    void Act(const RuleContext& context, VariableWriter& writer) override;

private:
    const PmdgDataGateway* data_;
    PmdgGroundConnReconciler* groundConn_;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDGRETRYGROUNDCONNUNTILSETRULE_H
