#ifndef GSX_INTEGRATOR_CLIENT_INTEGRATORRUNTIME_H
#define GSX_INTEGRATOR_CLIENT_INTEGRATORRUNTIME_H

#include <filesystem>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QTimer>
#include "model/IntegratorSnapshot.h"
#include "sim/SimVersion.h"
#include "../infrastructure/commbus/CommBusBridgeClient.h"
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
#include "../infrastructure/probe/ProbeObserver.h"
#include "../infrastructure/gsx/GsxRemoteApiClient.h"
#include "../infrastructure/gsx/GsxRemoteState.h"

class Aircraft;
struct AircraftDescriptor;

class IntegratorRuntime final : public QObject
{
    Q_OBJECT

public:
    explicit IntegratorRuntime(QObject* parent = nullptr);
    ~IntegratorRuntime() override;

    void Setup();
    void Shutdown();

    [[nodiscard]] IntegratorSnapshot Snapshot() const;

    [[nodiscard]] const AutomationSettings& Settings() const { return settings_; }
    [[nodiscard]] bool IsConnected() const { return simConnect_.IsConnected(); }
    [[nodiscard]] bool IsReconnectPending() const { return reconnectTimer_.isActive(); }
    [[nodiscard]] bool IsSessionActive() const { return isSessionActive_; }
    [[nodiscard]] TurnaroundPhase GetPhase() const { return stateMachine_.GetPhase(); }
    [[nodiscard]] std::string GetAircraftProfileId() const;
    [[nodiscard]] bool HasGsxProfileConflict() const { return gsxProfile_.conflict; }
    bool FixGsxProfile();
    [[nodiscard]] bool HasPmdgOptionsConflict() const { return pmdgOptions_.conflict; }
    bool FixPmdgOptions();
    [[nodiscard]] bool IsCargoDoorStuck() const;
    [[nodiscard]] bool IsFuelRequestStalled() const;
    [[nodiscard]] bool AreServicesStalled() const;
    [[nodiscard]] bool AreDoorsHoldingPushback() const;
    void SetAutomationEnabled(bool enabled);
    void RestartFlow();
#ifndef NDEBUG
    void DebugSkipPhase(const int delta) { stateMachine_.DebugSkipPhase(delta); }
#endif
    void ConfirmLoading();
    void AcceptPilotTouch();
    void ApplySettings(const AutomationSettings& settings);
    [[nodiscard]] bool ReloadSimbrief();

    [[nodiscard]] CommBusBridgeGateway* Bridge() { return &bridgeClient_; }
    [[nodiscard]] SimVersion GetSimVersion() const { return simVersion_; }

signals:
    void Updated();
    void SimulatorQuit();

private:
    struct GsxProfileState
    {
        std::vector<std::filesystem::path> roots;
        std::vector<std::filesystem::path> cfgs;
        bool conflict = false;
        bool flagsMissing = false;

        void Reset()
        {
            roots.clear();
            cfgs.clear();
            conflict = false;
            flagsMissing = false;
        }
    };

    struct PmdgOptionsState
    {
        std::filesystem::path ini;
        bool conflict = false;

        void Reset()
        {
            ini.clear();
            conflict = false;
        }
    };

    [[nodiscard]] bool IsSessionPaused() const { return pauseFlags_ != 0; }
    [[nodiscard]] bool IsSessionReady();
    [[nodiscard]] const AutomationStatus& Status() const { return status_; }
    [[nodiscard]] int GetDelayTicksRemaining() const { return stateMachine_.GetDelayTicksRemaining(); }
    [[nodiscard]] bool IsLoadingConfirmed() const { return stateMachine_.IsLoadingConfirmed(); }
    [[nodiscard]] QString GetAircraftName() const;
    [[nodiscard]] bool IsAircraftRefuelByGsx() const;
    [[nodiscard]] bool IsAircraftRefuelBySelf() const;
    [[nodiscard]] bool IsAircraftCargoVariant() const;
    [[nodiscard]] bool IsLoadingCargoPhase() const;
    [[nodiscard]] bool AircraftRequiresEfbFlightPlan() const;
    [[nodiscard]] WeightUnit GetAutoWeightUnit() const;
    [[nodiscard]] bool CanFixGsxProfile() const;
    [[nodiscard]] bool CanFixPmdgOptions() const;

    bool IsSimOnMenu();
    void OnSimOpen(const char* appName);
    void TryConnect();
    bool SubscribeSimEvents();
    void HandleDisconnected();
    void OnDispatchTimer();
    void OnSimRunningChanged(bool running);
    void OnPauseChanged(unsigned flag);

    void ProbeGates();
    void Update();
    void UpdateSlow();
    void MaybeAutoStart();
    void ResetSession();
    void ClearFlightState();
    void OnFlightStart();
    void OnSessionEnd();
    void ResolveAircraft();
    void CheckGsxProfile();
    void CheckPmdgOptions();
    void AnnounceWireFacts();

    static constexpr int kDispatchIntervalMs = 80;
    static constexpr int kReconnectIntervalMs = 5000;

    SimConnectVariableGateway varGateway_;
    CommBusBridgeClient bridgeClient_;
    CommBusPluginClient pluginClient_;
    GsxStateService gsxService_;
    GsxRemoteApiClient gsxRemoteClient_;
    GsxRemoteState gsxRemoteState_;
    std::set<std::string> unknownPatchPaths_;
    std::string announcedHandlingOperator_;
    std::string announcedApronVerdict_;
    std::string announcedAircraftTitle_;
    int announcedSimbriefGeneration_ = 0;
    AutomationStatus status_;
    AutomationSettings settings_;
    GsxMenuNavigator gsxMenu_;
    TurnaroundStateMachine stateMachine_;
    SimbriefClient simbriefClient_;
    SimConnectSession simConnect_;
    std::unique_ptr<Aircraft> aircraft_;
    const AircraftDescriptor* aircraftDescriptor_ = nullptr;

    QTimer dispatchTimer_;
    QTimer reconnectTimer_;
    QtDomainLogger qtLogger_;
    ProbeObserver probe_;

    SimVersion simVersion_ = SimVersion::Unknown;
    bool isSessionActive_ = false;
    unsigned pauseFlags_ = 1;
    unsigned pauseEvents_ = 0;
    GsxProfileState gsxProfile_;
    PmdgOptionsState pmdgOptions_;
};

#endif // GSX_INTEGRATOR_CLIENT_INTEGRATORRUNTIME_H
