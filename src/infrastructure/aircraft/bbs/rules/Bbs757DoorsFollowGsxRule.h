#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_BBS757DOORSFOLLOWGSXRULE_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_BBS757DOORSFOLLOWGSXRULE_H

#include "../../../../domain/ports/AircraftRule.h"

class GsxDoorSync;

class Bbs757DoorsFollowGsxRule final : public AircraftRule
{
public:
    explicit Bbs757DoorsFollowGsxRule(GsxDoorSync& doors);

    [[nodiscard]] const char* Name() const override;
    [[nodiscard]] RuleVerdict Evaluate(const RuleContext& context) override;
    void Act(const RuleContext& context, VariableWriter& writer) override;

private:
    GsxDoorSync* doors_;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_BBS757DOORSFOLLOWGSXRULE_H
