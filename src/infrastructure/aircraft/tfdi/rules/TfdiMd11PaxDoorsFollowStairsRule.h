#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_TFDIMD11PAXDOORSFOLLOWSTAIRSRULE_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_TFDIMD11PAXDOORSFOLLOWSTAIRSRULE_H

#include "../../../../domain/ports/AircraftRule.h"

class VariableReader;

class TfdiMd11PaxDoorsFollowStairsRule final : public AircraftRule
{
public:
    explicit TfdiMd11PaxDoorsFollowStairsRule(VariableReader& variables);

    [[nodiscard]] const char* Name() const override;
    [[nodiscard]] RuleVerdict Evaluate(const RuleContext& context) override;
    void Act(const RuleContext& context, VariableWriter& writer) override;

private:
    void FollowStairs(VariableWriter& writer, const char* stairsStateLVar, const char* doorCmdLVar,
                      double& lastDoorTarget) const;

    VariableReader* variables_;
    double fwdDoorTarget_ = -1.0;
    double midDoorTarget_ = -1.0;
    double aftDoorTarget_ = -1.0;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_TFDIMD11PAXDOORSFOLLOWSTAIRSRULE_H
