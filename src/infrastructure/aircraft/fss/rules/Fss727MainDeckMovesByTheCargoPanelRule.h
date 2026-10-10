#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FSS727MAINDECKMOVESBYTHECARGOPANELRULE_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FSS727MAINDECKMOVESBYTHECARGOPANELRULE_H

#include <cstdint>
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

    [[nodiscard]] bool HasUnservedClose() const;
    [[nodiscard]] bool IsLoaderDepartureCloseUnserved() const;
    void RestoreUnservedLoaderDepartureClose();
    [[nodiscard]] bool IsDeboardingAtWork() const;
    [[nodiscard]] bool HasSeenTheMainLoaderAtTheDeck() const;
    void RestoreTheEdges(bool deboardingAtWork, bool mainLoaderSeenAtTheDeck, bool gsxRestartedSinceSave);
    void CheckThePanelMasterLeftOn();
    void ForgetTheResume();

private:
    enum class Travel : std::uint8_t
    {
        None,
        Opening,
        Closing
    };

    void StartTravel(VariableWriter& writer, Travel travel);
    void FinishTravel(VariableWriter& writer);
    void ResumeTravel(double position);
    void DropTheTravelThatNeverMoved();
    void GuardThePanelMasterCut(VariableWriter& writer);
    void TurnOffThePanelMasterLeftOn(VariableWriter& writer);
    [[nodiscard]] bool HasComeToRest() const;
    void TurnThePanelMasterOff(VariableWriter& writer, double position);
    void ServeThePendingClose(VariableWriter& writer, bool closed);
    void AskForTheDeckClosedOnceTheDeboardingCompletes();
    void AskForTheDeckClosedOnceTheMainLoaderLeaves();
    [[nodiscard]] int CloseRequests() const;
    [[nodiscard]] bool IsCloseRequestPending() const;
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
    int deboardingCloseRequests_ = 0;
    bool deboardingAtWork_ = false;
    bool deboardingReadAwaited_ = false;
    bool deboardingMayHaveEndedWhileAway_ = false;
    int loaderDepartureCloseRequests_ = 0;
    bool loaderDepartureCloseUnserved_ = false;
    bool mainLoaderSeenAtTheDeck_ = false;
    Travel travel_ = Travel::None;
    Travel cutTravel_ = Travel::None;
    Fss727DoorRest rest_;
    int unmovedTicks_ = 0;
    int masterCutGuardTicks_ = 0;
    bool mayResumeTravel_ = false;
    bool panelMasterLeftOnToCheck_ = false;
    int loaderHoldTicks_ = 0;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FSS727MAINDECKMOVESBYTHECARGOPANELRULE_H
