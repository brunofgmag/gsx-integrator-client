#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_TOLISSA340DOORSFOLLOWGSXRULE_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_TOLISSA340DOORSFOLLOWGSXRULE_H

#include "../../../../domain/ports/AircraftRule.h"

class GsxDoorSync;
class TolissA340;

class TolissA340DoorsFollowGsxRule final : public AircraftRule
{
public:
    TolissA340DoorsFollowGsxRule(const TolissA340& aircraft, GsxDoorSync& doors);

    [[nodiscard]] const char* Name() const override;
    [[nodiscard]] RuleVerdict Evaluate(const RuleContext& context) override;
    void Act(const RuleContext& context, VariableWriter& writer) override;

private:
    const TolissA340* aircraft_;
    GsxDoorSync* doors_;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_TOLISSA340DOORSFOLLOWGSXRULE_H
