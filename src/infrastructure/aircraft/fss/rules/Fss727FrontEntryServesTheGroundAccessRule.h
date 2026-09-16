#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FSS727FRONTENTRYSERVESTHEGROUNDACCESSRULE_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FSS727FRONTENTRYSERVESTHEGROUNDACCESSRULE_H

#include "../../../../domain/ports/AircraftRule.h"

class Fss727;
class GsxDoorSync;
class VariableReader;

class Fss727FrontEntryServesTheGroundAccessRule final : public AircraftRule
{
public:
    Fss727FrontEntryServesTheGroundAccessRule(VariableReader& variables, const Fss727& aircraft,
                                              GsxDoorSync& doors);

    [[nodiscard]] const char* Name() const override;
    [[nodiscard]] RuleVerdict Evaluate(const RuleContext& context) override;
    void Act(const RuleContext& context, VariableWriter& writer) override;

private:
    void Command(VariableWriter& writer, double target);
    [[nodiscard]] bool IsFrontEntryWanted() const;

    VariableReader* variables_;
    const Fss727* aircraft_;
    GsxDoorSync* doors_;
    double lastTarget_ = -1.0;
    int servedCloseRequests_ = 0;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FSS727FRONTENTRYSERVESTHEGROUNDACCESSRULE_H
