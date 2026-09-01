#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_TFDIMD11CARGODOORSFOLLOWLOADERRULE_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_TFDIMD11CARGODOORSFOLLOWLOADERRULE_H

#include "../../../../domain/ports/AircraftRule.h"

class VariableReader;

class TfdiMd11CargoDoorsFollowLoaderRule final : public AircraftRule
{
public:
    TfdiMd11CargoDoorsFollowLoaderRule(VariableReader& variables, bool cargoVariant);

    [[nodiscard]] const char* Name() const override;
    [[nodiscard]] RuleVerdict Evaluate(const RuleContext& context) override;
    void Act(const RuleContext& context, VariableWriter& writer) override;

private:
    void FollowLoader(VariableWriter& writer, const char* loaderStateLVar, const char* doorCmdLVar,
                      double& lastDoorTarget) const;

    VariableReader* variables_;
    bool cargoVariant_;
    double fwdDoorTarget_ = -1.0;
    double aftDoorTarget_ = -1.0;
    double mainDoorTarget_ = -1.0;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_TFDIMD11CARGODOORSFOLLOWLOADERRULE_H
