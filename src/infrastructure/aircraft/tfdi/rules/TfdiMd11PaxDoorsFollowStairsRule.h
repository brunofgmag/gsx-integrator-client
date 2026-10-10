#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_TFDIMD11PAXDOORSFOLLOWSTAIRSRULE_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_TFDIMD11PAXDOORSFOLLOWSTAIRSRULE_H

#include <optional>

#include "../../../../domain/ports/AircraftRule.h"

class TfdiMd11;
class VariableReader;

class TfdiMd11PaxDoorsFollowStairsRule final : public AircraftRule
{
public:
    struct DoorTargets
    {
        std::optional<double> fwd;
        std::optional<double> mid;
        std::optional<double> aft;
    };

    TfdiMd11PaxDoorsFollowStairsRule(VariableReader& variables, const TfdiMd11& aircraft);

    [[nodiscard]] const char* Name() const override;
    [[nodiscard]] RuleVerdict Evaluate(const RuleContext& context) override;
    void Act(const RuleContext& context, VariableWriter& writer) override;

    [[nodiscard]] DoorTargets Targets() const;
    void RestoreTargets(const DoorTargets& targets);

private:
    struct DoorLVars
    {
        const char* stairsState;
        const char* command;
    };

    void FollowStairs(VariableWriter& writer, const DoorLVars& door, std::optional<double>& lastDoorTarget) const;
    static void CloseWhileHeld(VariableWriter& writer, const DoorLVars& door, std::optional<double>& lastDoorTarget);

    VariableReader* variables_;
    const TfdiMd11* aircraft_;
    DoorTargets targets_;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_TFDIMD11PAXDOORSFOLLOWSTAIRSRULE_H
