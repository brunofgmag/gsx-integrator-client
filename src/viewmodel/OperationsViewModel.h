#ifndef GSX_INTEGRATOR_CLIENT_OPERATIONSVIEWMODEL_H
#define GSX_INTEGRATOR_CLIENT_OPERATIONSVIEWMODEL_H

#include <QtCore/QObject>
#include <QtCore/QString>
#include "../application/ports/IntegratorService.h"
#include "OperationsDisplaySettings.h"

class OperationsViewModel final : public QObject, public IntegratorServiceObserver
{
    Q_OBJECT

    Q_PROPERTY(bool connected READ IsConnected NOTIFY SnapshotChanged)
    Q_PROPERTY(bool enabled READ IsEnabled WRITE SetEnabled NOTIFY SnapshotChanged)
    Q_PROPERTY(bool gsxAvailable READ IsGsxAvailable NOTIFY SnapshotChanged)
    Q_PROPERTY(bool aircraftSupported READ IsAircraftSupported NOTIFY SnapshotChanged)
    Q_PROPERTY(QString aircraftNameText READ GetAircraftNameText NOTIFY SnapshotChanged)
    Q_PROPERTY(QString stateText READ GetStateText NOTIFY SnapshotChanged)
    Q_PROPERTY(int phase READ GetPhase NOTIFY SnapshotChanged)
    Q_PROPERTY(int phaseCount READ GetPhaseCount CONSTANT)
    Q_PROPERTY(QString phaseTip READ GetPhaseTip NOTIFY SnapshotChanged)
    Q_PROPERTY(QString nextPhaseText READ GetNextPhaseText NOTIFY SnapshotChanged)
    Q_PROPERTY(QString holdCountdownText READ GetHoldCountdownText NOTIFY SnapshotChanged)
    Q_PROPERTY(bool advancedByPilot READ AdvancedByPilot NOTIFY SnapshotChanged)
    Q_PROPERTY(bool inDeboardingPhase READ IsInDeboardingPhase NOTIFY SnapshotChanged)
    Q_PROPERTY(double fuelProgress READ GetFuelProgress NOTIFY SnapshotChanged)
    Q_PROPERTY(double boardingProgress READ GetBoardingProgress NOTIFY SnapshotChanged)
    Q_PROPERTY(double deboardingProgress READ GetDeboardingProgress NOTIFY SnapshotChanged)
    Q_PROPERTY(QString plannedFuelText READ GetPlannedFuelText NOTIFY SnapshotChanged)
    Q_PROPERTY(QString loadedFuelText READ GetLoadedFuelText NOTIFY SnapshotChanged)
    Q_PROPERTY(QString targetFuelText READ GetTargetFuelText NOTIFY SnapshotChanged)
    Q_PROPERTY(QString targetZfwText READ GetTargetZfwText NOTIFY SnapshotChanged)
    Q_PROPERTY(QString plannedZfwText READ GetPlannedZfwText NOTIFY SnapshotChanged)
    Q_PROPERTY(bool refuelByGsx READ RefuelByGsx NOTIFY SnapshotChanged)
    Q_PROPERTY(bool refuelBySelf READ RefuelBySelf NOTIFY SnapshotChanged)
    Q_PROPERTY(bool gsxProfileConflict READ HasGsxProfileConflict NOTIFY SnapshotChanged)
    Q_PROPERTY(bool gsxProfileFixable READ IsGsxProfileFixable NOTIFY SnapshotChanged)
    Q_PROPERTY(bool pmdgOptionsConflict READ HasPmdgOptionsConflict NOTIFY SnapshotChanged)
    Q_PROPERTY(bool cargoDoorStuck READ IsCargoDoorStuck NOTIFY SnapshotChanged)
    Q_PROPERTY(bool fuelRequestStalled READ IsFuelRequestStalled NOTIFY SnapshotChanged)
    Q_PROPERTY(bool pmdgOptionsFixable READ IsPmdgOptionsFixable NOTIFY SnapshotChanged)
    Q_PROPERTY(int plannedPax READ GetPlannedPax NOTIFY SnapshotChanged)
    Q_PROPERTY(int boardedPax READ GetBoardedPax NOTIFY SnapshotChanged)
    Q_PROPERTY(int deboardedPax READ GetDeboardedPax NOTIFY SnapshotChanged)
    Q_PROPERTY(int targetPax READ GetTargetPax NOTIFY SnapshotChanged)
    Q_PROPERTY(bool cargoAircraft READ IsCargoAircraft NOTIFY SnapshotChanged)
    Q_PROPERTY(QString simbriefStatusText READ GetSimbriefStatusText NOTIFY SnapshotChanged)
    Q_PROPERTY(bool simbriefReady READ IsSimbriefReady NOTIFY SnapshotChanged)
    Q_PROPERTY(bool simbriefError READ HasSimbriefError NOTIFY SnapshotChanged)
    Q_PROPERTY(QString simbriefRefusal READ GetSimbriefRefusal NOTIFY SnapshotChanged)
    Q_PROPERTY(bool canToggleAutomation READ CanToggleAutomation NOTIFY SnapshotChanged)
    Q_PROPERTY(bool canStartLoading READ CanStartLoading NOTIFY SnapshotChanged)
    Q_PROPERTY(bool canReloadSimbrief READ CanReloadSimbrief NOTIFY SnapshotChanged)
    Q_PROPERTY(QString commandError READ GetCommandError NOTIFY CommandErrorChanged)
    Q_PROPERTY(bool debugToolsAvailable READ AreDebugToolsAvailable CONSTANT)

public:
    OperationsViewModel(IntegratorService* service, const OperationsDisplaySettings* display, QObject* parent = nullptr);
    ~OperationsViewModel() override;

    [[nodiscard]] bool IsConnected() const;
    [[nodiscard]] bool IsSessionActive() const;
    [[nodiscard]] bool IsEnabled() const;
    void SetEnabled(bool enabled);
    [[nodiscard]] bool IsGsxAvailable() const;
    [[nodiscard]] bool IsAircraftSupported() const;
    [[nodiscard]] QString GetAircraftName() const;
    [[nodiscard]] QString GetAircraftNameText() const;
    [[nodiscard]] QString GetStateText() const;
    [[nodiscard]] int GetPhase() const;
    [[nodiscard]] static int GetPhaseCount();
    [[nodiscard]] QString GetPhaseTip() const;
    [[nodiscard]] QString GetNextPhaseText() const;
    [[nodiscard]] QString GetHoldCountdownText() const;
    [[nodiscard]] int GetDelayTicksRemaining() const;
    [[nodiscard]] bool AdvancedByPilot() const;
    [[nodiscard]] bool IsInDeboardingPhase() const;
    [[nodiscard]] double GetFuelProgress() const;
    [[nodiscard]] double GetBoardingProgress() const;
    [[nodiscard]] double GetDeboardingProgress() const;
    [[nodiscard]] double GetPlannedFuelKg() const;
    [[nodiscard]] QString GetPlannedFuelText() const;
    [[nodiscard]] QString GetLoadedFuelText() const;
    [[nodiscard]] QString GetTargetFuelText() const;
    [[nodiscard]] QString GetTargetZfwText() const;
    [[nodiscard]] QString GetPlannedZfwText() const;
    [[nodiscard]] double GetLoadedFuelKg() const;
    [[nodiscard]] bool RefuelByGsx() const;
    [[nodiscard]] bool RefuelBySelf() const;
    [[nodiscard]] bool HasGsxProfileConflict() const;
    [[nodiscard]] bool IsGsxProfileFixable() const;
    [[nodiscard]] bool HasPmdgOptionsConflict() const;
    [[nodiscard]] bool IsCargoDoorStuck() const;
    [[nodiscard]] bool IsFuelRequestStalled() const;
    [[nodiscard]] bool IsPmdgOptionsFixable() const;
    [[nodiscard]] double GetPlannedZfwKg() const;
    [[nodiscard]] int GetPlannedPax() const;
    [[nodiscard]] int GetBoardedPax() const;
    [[nodiscard]] int GetDeboardedPax() const;
    [[nodiscard]] double GetTargetFuelKg() const;
    [[nodiscard]] double GetTargetZfwKg() const;
    [[nodiscard]] int GetTargetPax() const;
    [[nodiscard]] int GetAutoWeightUnit() const;
    [[nodiscard]] bool IsCargoAircraft() const;
    [[nodiscard]] QString GetSimbriefStatusText() const;
    [[nodiscard]] bool IsSimbriefReady() const;
    [[nodiscard]] bool HasSimbriefError() const;
    [[nodiscard]] QString GetSimbriefRefusal() const;
    [[nodiscard]] bool CanToggleAutomation() const;
    [[nodiscard]] bool CanStartLoading() const;
    [[nodiscard]] bool CanReloadSimbrief() const;
    [[nodiscard]] QString GetCommandError() const;
    [[nodiscard]] static bool AreDebugToolsAvailable();

    Q_INVOKABLE void startFlow();
    Q_INVOKABLE void startLoading();
    Q_INVOKABLE void restartFlow();
    Q_INVOKABLE void reloadSimbrief();
    Q_INVOKABLE void fixGsxProfile();
    Q_INVOKABLE void fixPmdgOptions();
    Q_INVOKABLE void debugSkipPhase(int delta);

    void RetranslateUi();

    void OnIntegratorStateChanged() override;

signals:
    void SnapshotChanged();
    void CommandErrorChanged();

private:
    [[nodiscard]] bool IsAwaitingStartLoading() const;
    [[nodiscard]] QString WeightText(double kilograms) const;
    void Refresh();
    void SetCommandError(const CommandResult& result);

    IntegratorService* service_;
    const OperationsDisplaySettings* display_;
    IntegratorSnapshot snapshot_;
    QString commandError_;
};

#endif // GSX_INTEGRATOR_CLIENT_OPERATIONSVIEWMODEL_H
