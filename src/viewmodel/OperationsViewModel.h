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
    Q_PROPERTY(QString simLabel READ GetSimLabel NOTIFY SnapshotChanged)
    Q_PROPERTY(QString simStatusText READ GetSimStatusText NOTIFY SnapshotChanged)
    Q_PROPERTY(QString gsxLabel READ GetGsxLabel NOTIFY SnapshotChanged)
    Q_PROPERTY(QString gsxStatusText READ GetGsxStatusText NOTIFY SnapshotChanged)
    Q_PROPERTY(QString aircraftLabel READ GetAircraftLabel NOTIFY SnapshotChanged)
    Q_PROPERTY(QString turnaroundModeLabel READ GetTurnaroundModeLabel NOTIFY SnapshotChanged)
    Q_PROPERTY(QString turnaroundModeText READ GetTurnaroundModeText NOTIFY SnapshotChanged)
    Q_PROPERTY(QString loadingModeLabel READ GetLoadingModeLabel NOTIFY SnapshotChanged)
    Q_PROPERTY(QString loadingModeText READ GetLoadingModeText NOTIFY SnapshotChanged)
    Q_PROPERTY(QString turnaroundStateLabel READ GetTurnaroundStateLabel NOTIFY SnapshotChanged)
    Q_PROPERTY(QString stateText READ GetStateText NOTIFY SnapshotChanged)
    Q_PROPERTY(QString phaseCounterText READ GetPhaseCounterText NOTIFY SnapshotChanged)
    Q_PROPERTY(QString phaseTip READ GetPhaseTip NOTIFY SnapshotChanged)
    Q_PROPERTY(QString nextPhaseText READ GetNextPhaseText NOTIFY SnapshotChanged)
    Q_PROPERTY(QString holdCountdownText READ GetHoldCountdownText NOTIFY SnapshotChanged)
    Q_PROPERTY(bool inDeboardingPhase READ IsInDeboardingPhase NOTIFY SnapshotChanged)
    Q_PROPERTY(double fuelProgress READ GetFuelProgress NOTIFY SnapshotChanged)
    Q_PROPERTY(QString fuelCardLabel READ GetFuelCardLabel NOTIFY SnapshotChanged)
    Q_PROPERTY(QString fuelProgressText READ GetFuelProgressText NOTIFY SnapshotChanged)
    Q_PROPERTY(QString loadedFuelLabel READ GetLoadedFuelLabel NOTIFY SnapshotChanged)
    Q_PROPERTY(QString targetFuelLabel READ GetTargetFuelLabel NOTIFY SnapshotChanged)
    Q_PROPERTY(QString fuelRateLabel READ GetFuelRateLabel NOTIFY SnapshotChanged)
    Q_PROPERTY(QString fuelRateText READ GetFuelRateText NOTIFY SnapshotChanged)
    Q_PROPERTY(QString paxCardLabel READ GetPaxCardLabel NOTIFY SnapshotChanged)
    Q_PROPERTY(QString paxProgressText READ GetPaxProgressText NOTIFY SnapshotChanged)
    Q_PROPERTY(QString paxLabel READ GetPaxLabel NOTIFY SnapshotChanged)
    Q_PROPERTY(QString paxCountText READ GetPaxCountText NOTIFY SnapshotChanged)
    Q_PROPERTY(QString targetZfwLabel READ GetTargetZfwLabel NOTIFY SnapshotChanged)
    Q_PROPERTY(QString simbriefCardLabel READ GetSimbriefCardLabel NOTIFY SnapshotChanged)
    Q_PROPERTY(QString plannedFuelLabel READ GetPlannedFuelLabel NOTIFY SnapshotChanged)
    Q_PROPERTY(QString plannedZfwLabel READ GetPlannedZfwLabel NOTIFY SnapshotChanged)
    Q_PROPERTY(QString plannedPaxLabel READ GetPlannedPaxLabel NOTIFY SnapshotChanged)
    Q_PROPERTY(QString plannedPaxText READ GetPlannedPaxText NOTIFY SnapshotChanged)
    Q_PROPERTY(double boardingProgress READ GetBoardingProgress NOTIFY SnapshotChanged)
    Q_PROPERTY(double deboardingProgress READ GetDeboardingProgress NOTIFY SnapshotChanged)
    Q_PROPERTY(QString plannedFuelText READ GetPlannedFuelText NOTIFY SnapshotChanged)
    Q_PROPERTY(QString loadedFuelText READ GetLoadedFuelText NOTIFY SnapshotChanged)
    Q_PROPERTY(QString targetFuelText READ GetTargetFuelText NOTIFY SnapshotChanged)
    Q_PROPERTY(QString targetZfwText READ GetTargetZfwText NOTIFY SnapshotChanged)
    Q_PROPERTY(QString plannedZfwText READ GetPlannedZfwText NOTIFY SnapshotChanged)
    Q_PROPERTY(bool gsxProfileConflict READ HasGsxProfileConflict NOTIFY SnapshotChanged)
    Q_PROPERTY(QString gsxProfileAdvisoryText READ GetGsxProfileAdvisoryText NOTIFY SnapshotChanged)
    Q_PROPERTY(QString gsxProfileActionLabel READ GetGsxProfileActionLabel NOTIFY SnapshotChanged)
    Q_PROPERTY(QString pmdgOptionsAdvisoryText READ GetPmdgOptionsAdvisoryText NOTIFY SnapshotChanged)
    Q_PROPERTY(QString pmdgOptionsActionLabel READ GetPmdgOptionsActionLabel NOTIFY SnapshotChanged)
    Q_PROPERTY(QString cargoDoorAdvisoryText READ GetCargoDoorAdvisoryText NOTIFY SnapshotChanged)
    Q_PROPERTY(QString fuelRequestAdvisoryText READ GetFuelRequestAdvisoryText NOTIFY SnapshotChanged)
    Q_PROPERTY(QString fuelPlanAdvisoryText READ GetFuelPlanAdvisoryText NOTIFY SnapshotChanged)
    Q_PROPERTY(QString servicesAdvisoryText READ GetServicesAdvisoryText NOTIFY SnapshotChanged)
    Q_PROPERTY(QString openDoorAdvisoryText READ GetOpenDoorAdvisoryText NOTIFY SnapshotChanged)
    Q_PROPERTY(QString serviceInterruptedAdvisoryText READ GetServiceInterruptedAdvisoryText NOTIFY SnapshotChanged)
    Q_PROPERTY(QString commandErrorLabel READ GetCommandErrorLabel NOTIFY SnapshotChanged)
    Q_PROPERTY(bool pmdgOptionsConflict READ HasPmdgOptionsConflict NOTIFY SnapshotChanged)
    Q_PROPERTY(bool cargoDoorStuck READ IsCargoDoorStuck NOTIFY SnapshotChanged)
    Q_PROPERTY(bool fuelRequestStalled READ IsFuelRequestStalled NOTIFY SnapshotChanged)
    Q_PROPERTY(bool fuelPlanOverCapacity READ IsFuelPlanOverCapacity NOTIFY SnapshotChanged)
    Q_PROPERTY(bool servicesStalled READ AreServicesStalled NOTIFY SnapshotChanged)
    Q_PROPERTY(bool serviceInterrupted READ IsServiceInterrupted NOTIFY SnapshotChanged)
    Q_PROPERTY(bool doorsHoldingPushback READ AreDoorsHoldingPushback NOTIFY SnapshotChanged)
    Q_PROPERTY(bool cargoAircraft READ IsCargoAircraft NOTIFY SnapshotChanged)
    Q_PROPERTY(QString simbriefStatusText READ GetSimbriefStatusText NOTIFY SnapshotChanged)
    Q_PROPERTY(bool simbriefReady READ IsSimbriefReady NOTIFY SnapshotChanged)
    Q_PROPERTY(bool simbriefError READ HasSimbriefError NOTIFY SnapshotChanged)
    Q_PROPERTY(QString simbriefFailureText READ GetSimbriefFailureText NOTIFY SnapshotChanged)
    Q_PROPERTY(QString startFlowLabel READ GetStartFlowLabel NOTIFY SnapshotChanged)
    Q_PROPERTY(QString startLoadingLabel READ GetStartLoadingLabel NOTIFY SnapshotChanged)
    Q_PROPERTY(QString restartFlowLabel READ GetRestartFlowLabel NOTIFY SnapshotChanged)
    Q_PROPERTY(QString confirmRestartLabel READ GetConfirmRestartLabel NOTIFY SnapshotChanged)
    Q_PROPERTY(QString reloadSimbriefLabel READ GetReloadSimbriefLabel NOTIFY SnapshotChanged)
    Q_PROPERTY(bool canStartFlow READ CanStartFlow NOTIFY SnapshotChanged)
    Q_PROPERTY(bool canRestartFlow READ CanRestartFlow NOTIFY SnapshotChanged)
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
    [[nodiscard]] static QString GetSimLabel();
    [[nodiscard]] QString GetSimStatusText() const;
    [[nodiscard]] static QString GetGsxLabel();
    [[nodiscard]] QString GetGsxStatusText() const;
    [[nodiscard]] static QString GetAircraftLabel();
    [[nodiscard]] static QString GetTurnaroundModeLabel();
    [[nodiscard]] QString GetTurnaroundModeText() const;
    [[nodiscard]] static QString GetLoadingModeLabel();
    [[nodiscard]] QString GetLoadingModeText() const;
    [[nodiscard]] bool AutoStartsLoading() const;
    [[nodiscard]] static QString GetTurnaroundStateLabel();
    [[nodiscard]] QString GetStateText() const;
    [[nodiscard]] QString GetPhaseCounterText() const;
    [[nodiscard]] int GetPhase() const;
    [[nodiscard]] static int GetPhaseCount();
    [[nodiscard]] QString GetPhaseTip() const;
    [[nodiscard]] QString GetNextPhaseText() const;
    [[nodiscard]] QString GetHoldCountdownText() const;
    [[nodiscard]] int GetDelayTicksRemaining() const;
    [[nodiscard]] bool IsInDeboardingPhase() const;
    [[nodiscard]] double GetFuelProgress() const;
    [[nodiscard]] static QString GetFuelCardLabel();
    [[nodiscard]] QString GetFuelProgressText() const;
    [[nodiscard]] static QString GetLoadedFuelLabel();
    [[nodiscard]] static QString GetTargetFuelLabel();
    [[nodiscard]] static QString GetFuelRateLabel();
    [[nodiscard]] QString GetFuelRateText() const;
    [[nodiscard]] QString GetPaxCardLabel() const;
    [[nodiscard]] QString GetPaxProgressText() const;
    [[nodiscard]] static QString GetPaxLabel();
    [[nodiscard]] QString GetPaxCountText() const;
    [[nodiscard]] static QString GetTargetZfwLabel();
    [[nodiscard]] static QString GetSimbriefCardLabel();
    [[nodiscard]] static QString GetPlannedFuelLabel();
    [[nodiscard]] static QString GetPlannedZfwLabel();
    [[nodiscard]] static QString GetPlannedPaxLabel();
    [[nodiscard]] QString GetPlannedPaxText() const;
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
    [[nodiscard]] QString GetGsxProfileAdvisoryText() const;
    [[nodiscard]] QString GetGsxProfileActionLabel() const;
    [[nodiscard]] static QString GetPmdgOptionsAdvisoryText();
    [[nodiscard]] QString GetPmdgOptionsActionLabel() const;
    [[nodiscard]] static QString GetCargoDoorAdvisoryText();
    [[nodiscard]] static QString GetFuelRequestAdvisoryText();
    [[nodiscard]] static QString GetFuelPlanAdvisoryText();
    [[nodiscard]] QString GetServicesAdvisoryText() const;
    [[nodiscard]] static QString GetOpenDoorAdvisoryText();
    [[nodiscard]] static QString GetServiceInterruptedAdvisoryText();
    [[nodiscard]] static QString GetCommandErrorLabel();
    [[nodiscard]] bool IsGsxProfileFixable() const;
    [[nodiscard]] bool HasPmdgOptionsConflict() const;
    [[nodiscard]] bool IsCargoDoorStuck() const;
    [[nodiscard]] bool IsFuelRequestStalled() const;
    [[nodiscard]] bool IsFuelPlanOverCapacity() const;
    [[nodiscard]] bool AreServicesStalled() const;
    [[nodiscard]] bool IsServiceInterrupted() const;
    [[nodiscard]] int GetServicesWaitSeconds() const;
    [[nodiscard]] bool AreDoorsHoldingPushback() const;
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
    [[nodiscard]] QString GetSimbriefFailureText() const;
    [[nodiscard]] static QString GetStartFlowLabel();
    [[nodiscard]] static QString GetStartLoadingLabel();
    [[nodiscard]] static QString GetRestartFlowLabel();
    [[nodiscard]] static QString GetConfirmRestartLabel();
    [[nodiscard]] static QString GetReloadSimbriefLabel();
    [[nodiscard]] bool CanStartFlow() const;
    [[nodiscard]] bool CanRestartFlow() const;
    [[nodiscard]] bool AutoStartsFlow() const;
    [[nodiscard]] bool IsLoadingRunning() const;
    [[nodiscard]] bool CanStartLoading() const;
    [[nodiscard]] bool CanReloadSimbrief() const;
    [[nodiscard]] QString GetPilotTouchLabel() const;
    [[nodiscard]] bool CanPilotTouch() const;
    [[nodiscard]] QString GetCommandError() const;
    [[nodiscard]] static bool AreDebugToolsAvailable();

    Q_INVOKABLE void startFlow();
    Q_INVOKABLE void startLoading();
    Q_INVOKABLE void restartFlow();
    Q_INVOKABLE void reloadSimbrief();
    Q_INVOKABLE void fixGsxProfile();
    Q_INVOKABLE void fixPmdgOptions();
    Q_INVOKABLE void debugSkipPhase(int delta);

    void AcceptPilotTouch(TurnaroundPhase stamped);

    void RefreshDisplayText();

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
