#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FSS727MAINDECKMOVESBYTHECARGOPANELRULE_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FSS727MAINDECKMOVESBYTHECARGOPANELRULE_H

#include <optional>

#include "../../../../domain/ports/AircraftRule.h"

class Fss727;
class GsxDoorSync;
class GsxGateway;
class VariableReader;
enum class GsxState : int;
enum class GsxStateStatus : int;

struct Fss727DoorRest
{
    void Follow(double position);
    [[nodiscard]] bool HasMoved() const;
    [[nodiscard]] bool IsStill() const;

    std::optional<double> lastPosition;
    bool moved = false;
    int stillTicks = 0;
};

class Fss727MainDeckMovesByTheCargoPanelRule final : public AircraftRule
{
public:
    Fss727MainDeckMovesByTheCargoPanelRule(VariableReader& variables, const Fss727& aircraft,
                                           const GsxGateway* gsxGateway, const GsxDoorSync& doors);

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
    void GuardThePanelMasterCut(VariableWriter& writer);
    [[nodiscard]] bool HasComeToRest() const;
    void TurnThePanelMasterOff(VariableWriter& writer, double position);
    [[nodiscard]] bool IsCloseRequestServable() const;
    [[nodiscard]] bool HasTheMainLoaderLeft() const;
    [[nodiscard]] bool IsTheMainLoaderWaitingForTheDeck() const;
    [[nodiscard]] bool IsGsxWorkingTheCargoDoors() const;
    [[nodiscard]] bool IsGsxUnderway(GsxState state) const;
    [[nodiscard]] bool IsGsxWorkingTheDoors(GsxState state) const;
    [[nodiscard]] GsxStateStatus GsxStatusOf(GsxState state) const;

    VariableReader* variables_;
    const Fss727* aircraft_;
    const GsxGateway* gsxGateway_;
    const GsxDoorSync* doors_;
    int servedRequests_ = 0;
    Travel travel_ = Travel::None;
    Fss727DoorRest rest_;
    int masterCutGuardTicks_ = 0;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FSS727MAINDECKMOVESBYTHECARGOPANELRULE_H
