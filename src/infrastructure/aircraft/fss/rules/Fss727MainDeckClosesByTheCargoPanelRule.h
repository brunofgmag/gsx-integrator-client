#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FSS727MAINDECKCLOSESBYTHECARGOPANELRULE_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FSS727MAINDECKCLOSESBYTHECARGOPANELRULE_H

#include "../../../../domain/ports/AircraftRule.h"

class Fss727;
class GsxGateway;

class Fss727MainDeckClosesByTheCargoPanelRule final : public AircraftRule
{
public:
    Fss727MainDeckClosesByTheCargoPanelRule(const Fss727& aircraft, const GsxGateway* gsxGateway);

    [[nodiscard]] const char* Name() const override;
    [[nodiscard]] RuleVerdict Evaluate(const RuleContext& context) override;
    void Act(const RuleContext& context, VariableWriter& writer) override;

private:
    [[nodiscard]] bool IsGsxWorkingTheCargoDoors() const;

    const Fss727* aircraft_;
    const GsxGateway* gsxGateway_;
    int servedRequests_ = 0;
    bool masterOn_ = false;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FSS727MAINDECKCLOSESBYTHECARGOPANELRULE_H
