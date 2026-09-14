#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FSS727HOLDSCLOSEONCETHEIRLOADERLEAVESRULE_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FSS727HOLDSCLOSEONCETHEIRLOADERLEAVESRULE_H

#include <array>

#include "../../../../domain/ports/AircraftRule.h"

class Fss727;
class GsxDoorSync;
class VariableReader;

class Fss727HoldsCloseOnceTheirLoaderLeavesRule final : public AircraftRule
{
public:
    Fss727HoldsCloseOnceTheirLoaderLeavesRule(VariableReader& variables, const Fss727& aircraft,
                                              const GsxDoorSync& doors);

    [[nodiscard]] const char* Name() const override;
    [[nodiscard]] RuleVerdict Evaluate(const RuleContext& context) override;
    void Act(const RuleContext& context, VariableWriter& writer) override;

private:
    [[nodiscard]] bool HasItsLoaderLeft(const char* loaderLVar) const;

    VariableReader* variables_;
    const Fss727* aircraft_;
    const GsxDoorSync* doors_;
    std::array<int, 2> servedRequests_{};
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FSS727HOLDSCLOSEONCETHEIRLOADERLEAVESRULE_H
