#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_TFDIMD11CARGODOORSFOLLOWLOADERRULE_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_TFDIMD11CARGODOORSFOLLOWLOADERRULE_H

#include <optional>

#include "../../../../domain/ports/AircraftRule.h"

class GsxDoorSync;
class VariableReader;

class TfdiMd11CargoDoorsFollowLoaderRule final : public AircraftRule
{
public:
    struct DoorTargets
    {
        std::optional<double> fwd;
        std::optional<double> aft;
        std::optional<double> main;
    };

    TfdiMd11CargoDoorsFollowLoaderRule(VariableReader& variables, GsxDoorSync& doors, bool cargoVariant);

    [[nodiscard]] const char* Name() const override;
    [[nodiscard]] RuleVerdict Evaluate(const RuleContext& context) override;
    void Act(const RuleContext& context, VariableWriter& writer) override;

    [[nodiscard]] DoorTargets Targets() const;
    void RestoreTargets(const DoorTargets& targets);

private:
    struct DoorLVars
    {
        const char* loaderState;
        const char* command;
        const char* reading;
    };

    void FollowLoader(VariableWriter& writer, const DoorLVars& door, std::optional<double>& lastDoorTarget) const;

    VariableReader* variables_;
    GsxDoorSync* doors_;
    bool cargoVariant_;
    DoorTargets targets_;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_TFDIMD11CARGODOORSFOLLOWLOADERRULE_H
