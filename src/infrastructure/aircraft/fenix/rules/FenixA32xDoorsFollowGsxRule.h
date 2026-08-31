#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FENIXA32XDOORSFOLLOWGSXRULE_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FENIXA32XDOORSFOLLOWGSXRULE_H

#include "../../../../domain/ports/AircraftRule.h"

class FenixA32x;
class FenixEfbGateway;
class GsxDoorSync;
enum class FenixVariant;

class FenixA32xDoorsFollowGsxRule final : public AircraftRule
{
public:
    FenixA32xDoorsFollowGsxRule(const FenixA32x& aircraft, FenixEfbGateway& efb, GsxDoorSync& doors,
                                FenixVariant variant);

    [[nodiscard]] const char* Name() const override;
    [[nodiscard]] RuleVerdict Evaluate(const RuleContext& context) override;
    void Act(const RuleContext& context, VariableWriter& writer) override;

private:
    const FenixA32x* aircraft_;
    FenixEfbGateway* efb_;
    GsxDoorSync* doors_;
    FenixVariant variant_;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FENIXA32XDOORSFOLLOWGSXRULE_H
