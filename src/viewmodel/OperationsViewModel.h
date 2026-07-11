#ifndef GSX_INTEGRATOR_CLIENT_OPERATIONSVIEWMODEL_H
#define GSX_INTEGRATOR_CLIENT_OPERATIONSVIEWMODEL_H

#include <QtCore/QObject>
#include <QtCore/QString>
#include "../application/ports/IntegratorService.h"

class OperationsViewModel final : public QObject, public IntegratorServiceObserver
{
    Q_OBJECT

    Q_PROPERTY(bool connected READ IsConnected NOTIFY SnapshotChanged)
    Q_PROPERTY(bool sessionActive READ IsSessionActive NOTIFY SnapshotChanged)
    Q_PROPERTY(bool enabled READ IsEnabled WRITE SetEnabled NOTIFY SnapshotChanged)
    Q_PROPERTY(bool gsxAvailable READ IsGsxAvailable NOTIFY SnapshotChanged)
    Q_PROPERTY(bool aircraftSupported READ IsAircraftSupported NOTIFY SnapshotChanged)
    Q_PROPERTY(QString aircraftName READ GetAircraftName NOTIFY SnapshotChanged)
    Q_PROPERTY(QString stateText READ GetStateText NOTIFY SnapshotChanged)
    Q_PROPERTY(int phase READ GetPhase NOTIFY SnapshotChanged)
    Q_PROPERTY(int phaseCount READ GetPhaseCount CONSTANT)
    Q_PROPERTY(QString phaseTip READ GetPhaseTip NOTIFY SnapshotChanged)
    Q_PROPERTY(double fuelProgress READ GetFuelProgress NOTIFY SnapshotChanged)
    Q_PROPERTY(double boardingProgress READ GetBoardingProgress NOTIFY SnapshotChanged)
    Q_PROPERTY(double deboardingProgress READ GetDeboardingProgress NOTIFY SnapshotChanged)
    Q_PROPERTY(double plannedFuelKg READ GetPlannedFuelKg NOTIFY SnapshotChanged)
    Q_PROPERTY(double loadedFuelKg READ GetLoadedFuelKg NOTIFY SnapshotChanged)
    Q_PROPERTY(bool refueledExternally READ IsRefueledExternally NOTIFY SnapshotChanged)
    Q_PROPERTY(double plannedZfwKg READ GetPlannedZfwKg NOTIFY SnapshotChanged)
    Q_PROPERTY(int plannedPax READ GetPlannedPax NOTIFY SnapshotChanged)
    Q_PROPERTY(int boardedPax READ GetBoardedPax NOTIFY SnapshotChanged)
    Q_PROPERTY(QString simbriefStatusText READ GetSimbriefStatusText NOTIFY SnapshotChanged)
    Q_PROPERTY(bool simbriefReady READ IsSimbriefReady NOTIFY SnapshotChanged)
    Q_PROPERTY(bool simbriefError READ HasSimbriefError NOTIFY SnapshotChanged)
    Q_PROPERTY(bool canToggleAutomation READ CanToggleAutomation NOTIFY SnapshotChanged)
    Q_PROPERTY(bool canStartLoading READ CanStartLoading NOTIFY SnapshotChanged)
    Q_PROPERTY(bool canReloadSimbrief READ CanReloadSimbrief NOTIFY SnapshotChanged)
    Q_PROPERTY(QString commandError READ GetCommandError NOTIFY CommandErrorChanged)

public:
    explicit OperationsViewModel(IntegratorService* service, QObject* parent = nullptr);
    ~OperationsViewModel() override;

    [[nodiscard]] bool IsConnected() const;
    [[nodiscard]] bool IsSessionActive() const;
    [[nodiscard]] bool IsEnabled() const;
    void SetEnabled(bool enabled);
    [[nodiscard]] bool IsGsxAvailable() const;
    [[nodiscard]] bool IsAircraftSupported() const;
    [[nodiscard]] QString GetAircraftName() const;
    [[nodiscard]] QString GetStateText() const;
    [[nodiscard]] int GetPhase() const;
    [[nodiscard]] static int GetPhaseCount();
    [[nodiscard]] QString GetPhaseTip() const;
    Q_INVOKABLE static QString phaseLabelAt(int index);
    [[nodiscard]] double GetFuelProgress() const;
    [[nodiscard]] double GetBoardingProgress() const;
    [[nodiscard]] double GetDeboardingProgress() const;
    [[nodiscard]] double GetPlannedFuelKg() const;
    [[nodiscard]] double GetLoadedFuelKg() const;
    [[nodiscard]] bool IsRefueledExternally() const;
    [[nodiscard]] double GetPlannedZfwKg() const;
    [[nodiscard]] int GetPlannedPax() const;
    [[nodiscard]] int GetBoardedPax() const;
    [[nodiscard]] QString GetSimbriefStatusText() const;
    [[nodiscard]] bool IsSimbriefReady() const;
    [[nodiscard]] bool HasSimbriefError() const;
    [[nodiscard]] bool CanToggleAutomation() const;
    [[nodiscard]] bool CanStartLoading() const;
    [[nodiscard]] bool CanReloadSimbrief() const;
    [[nodiscard]] QString GetCommandError() const;

    Q_INVOKABLE void startFlow();
    Q_INVOKABLE void startLoading();
    Q_INVOKABLE void reloadSimbrief();

    void RetranslateUi();

    void OnIntegratorStateChanged() override;

signals:
    void SnapshotChanged();
    void CommandErrorChanged();

private:
    void Refresh();
    void SetCommandError(const CommandResult& result);

    IntegratorService* service_;
    IntegratorSnapshot snapshot_;
    QString commandError_;
};

#endif // GSX_INTEGRATOR_CLIENT_OPERATIONSVIEWMODEL_H
