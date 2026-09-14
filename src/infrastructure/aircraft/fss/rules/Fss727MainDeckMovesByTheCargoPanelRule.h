#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FSS727MAINDECKMOVESBYTHECARGOPANELRULE_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FSS727MAINDECKMOVESBYTHECARGOPANELRULE_H

#include "../../../../domain/ports/AircraftRule.h"

class Fss727;
class GsxDoorSync;
class GsxGateway;
enum class GsxState : int;

class Fss727MainDeckMovesByTheCargoPanelRule final : public AircraftRule
{
public:
    Fss727MainDeckMovesByTheCargoPanelRule(const Fss727& aircraft, const GsxGateway* gsxGateway,
                                           const GsxDoorSync& doors);

    [[nodiscard]] const char* Name() const override;
    [[nodiscard]] RuleVerdict Evaluate(const RuleContext& context) override;
    void Act(const RuleContext& context, VariableWriter& writer) override;

private:
    enum class Travel
    {
        None,
        Opening,
        Closing
    };

    void StartTravel(VariableWriter& writer, Travel travel);
    void FinishTravel(VariableWriter& writer);
    void TurnThePanelMasterOff(VariableWriter& writer);
    [[nodiscard]] bool IsCloseRequestServable() const;
    [[nodiscard]] bool IsTheMainLoaderWaitingForTheDeck() const;
    [[nodiscard]] bool IsGsxWorkingTheCargoDoors() const;
    [[nodiscard]] bool IsGsxUnderway(GsxState state) const;

    const Fss727* aircraft_;
    const GsxGateway* gsxGateway_;
    const GsxDoorSync* doors_;
    int servedRequests_ = 0;
    Travel travel_ = Travel::None;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FSS727MAINDECKMOVESBYTHECARGOPANELRULE_H
