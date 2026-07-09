#ifndef GSX_INTEGRATOR_CLIENT_INTEGRATORRUNTIME_H
#define GSX_INTEGRATOR_CLIENT_INTEGRATORRUNTIME_H

#include <memory>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QTimer>
#include "sim/SimVersion.h"
#include "../infrastructure/commbus/CommBusPluginClient.h"
#include "../infrastructure/gsx/GsxStateService.h"
#include "../infrastructure/simbrief/SimbriefClient.h"
#include "../infrastructure/simconnect/SimConnectSession.h"
#include "../infrastructure/simconnect/SimConnectVariableGateway.h"
#include "../domain/model/AutomationStatus.h"
#include "../domain/model/AutomationSettings.h"
#include "../domain/turnaround/TurnaroundStateMachine.h"
#include "../infrastructure/gsx/GsxMenuNavigator.h"
#include "../infrastructure/logging/QtDomainLogger.h"
#include "../infrastructure/gsx/GsxRemoteApiClient.h"
#include "../infrastructure/gsx/GsxRemoteState.h"

class Aircraft;

class IntegratorRuntime final : public QObject
{
    Q_OBJECT

public:
    explicit IntegratorRuntime(QObject* parent = nullptr);
    ~IntegratorRuntime() override;

    void Setup();
    void Shutdown();

    [[nodiscard]] const AutomationStatus& Status() const { return status_; }
    [[nodiscard]] const AutomationSettings& Settings() const { return settings_; }

    [[nodiscard]] bool IsConnected() const { return simConnect_.IsConnected(); }
    [[nodiscard]] bool IsSessionActive() const { return isSessionActive_; }
    [[nodiscard]] bool IsSessionPaused() const { return pauseFlags_ != 0; }
    [[nodiscard]] bool IsSessionReady();
    [[nodiscard]] TurnaroundPhase GetPhase() const { return stateMachine_.GetPhase(); }
    [[nodiscard]] QString GetAircraftName() const;
    void SetAutomationEnabled(bool enabled);
    void ApplySettings(const AutomationSettings& settings);
    [[nodiscard]] bool ReloadSimbrief();

signals:
    void Updated();
    void SimulatorQuit();

private:
    bool IsSimOnMenu();
    void OnSimOpen(const char* appName);
    void TryConnect();
    void HandleDisconnected();
    void OnDispatchTimer();
    void OnSimRunningChanged(bool running);
    void OnPauseChanged(unsigned flag);

    void Update();
    void UpdateSlow();
    void MaybeAutoStart();
    void ResetSession();
    void OnFlightStart();
    void OnSessionEnd();
    void ResolveAircraft();

    static constexpr int kDispatchIntervalMs = 80;
    static constexpr int kReconnectIntervalMs = 5000;

    SimConnectVariableGateway varGateway_;
    CommBusPluginClient pluginClient_;
    GsxStateService gsxService_;
    GsxRemoteApiClient gsxRemoteClient_;
    GsxRemoteState gsxRemoteState_;
    AutomationStatus status_;
    AutomationSettings settings_;
    GsxMenuNavigator gsxMenu_;
    TurnaroundStateMachine stateMachine_;
    SimbriefClient simbriefClient_;
    SimConnectSession simConnect_;
    std::unique_ptr<Aircraft> aircraft_;

    QTimer dispatchTimer_;
    QTimer reconnectTimer_;
    QtDomainLogger qtLogger_;

    SimVersion simVersion_ = SimVersion::Unknown;
    bool isSessionActive_ = false;
    unsigned pauseFlags_ = 1;
};

#endif // GSX_INTEGRATOR_CLIENT_INTEGRATORRUNTIME_H
