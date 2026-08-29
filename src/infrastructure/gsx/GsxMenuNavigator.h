#ifndef GSX_INTEGRATOR_CLIENT_GSXMENUNAVIGATOR_H
#define GSX_INTEGRATOR_CLIENT_GSXMENUNAVIGATOR_H

#include <functional>
#include <string>
#include <utility>
#include <vector>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include "GsxRemoteState.h"
#include "../../domain/ports/GsxMenuGateway.h"

enum class GsxPanelMode;
struct AutomationSettings;
class CommBusPluginClient;
class DomainLogger;
class GsxRemoteApiClient;

class GsxMenuNavigator : public QObject, public GsxMenuGateway
{
    Q_OBJECT

public:
    GsxMenuNavigator(GsxRemoteApiClient* client,
                     GsxRemoteState* state,
                     const AutomationSettings* settings,
                     DomainLogger* logger,
                     CommBusPluginClient* pluginClient = nullptr,
                     QObject* parent = nullptr);

    void CallJetway() override;
    void CallStairs() override;
    void RepositionAircraft() override;
    void RequestSimbriefLoad() override;
    void RequestBoarding() override;
    void RequestDeboarding() override;
    void RequestPushback() override;
    void RequestRefueling() override;
    void CompleteRefuel() override;
    void CompleteBoarding() override;
    void ToggleGpu() override;
    void RequestCatering() override;
    void RequestLavatory() override;
    void RequestWater() override;
    void RequestCleaning() override;

    [[nodiscard]] bool ConfirmGoodEngines() override;
    [[nodiscard]] bool CompletePushback() override;

    [[nodiscard]] bool IsMenuSettled() const;

    void OpenMenu() const;
    void OpenPushbackPanel() override;
    void OnTurnaroundTurned() override;
    void OnPushbackStarted() override;

    void OnMenuChanged();
    void OnSnapshot();
    void DisableGsxMenu() override;

    void Reset();

    void SetClockForTest(std::function<long long()> clock) { nowMs_ = std::move(clock); }

private:
    struct TimedIntent
    {
        bool active = false;
        long long sinceMs = 0;
    };

    struct PendingRequest
    {
        QString verb;
        QJsonObject args;
        std::string label;
        std::string confirmId;
        long long lastSentMs = 0;
        int attempts = 0;
        bool toggles = false;
    };

    void TriggerService(const char* serviceId, bool toggles = false);
    void SyncGsxToolbar() const;
    [[nodiscard]] GsxPanelMode PanelMode() const;
    void ClosePanelAfterPushback();
    void RearmPanelLatches();
    [[nodiscard]] bool IsWaitingForThePanel();
    void ArmRequest(QString verb, QJsonObject args, std::string label, std::string confirmId,
                    bool toggles = false);
    [[nodiscard]] bool IsAlreadyUnderway(const PendingRequest& request) const;
    void PumpRequests();
    void SendRequest(PendingRequest& request);
    [[nodiscard]] bool WasTaken(const PendingRequest& request) const;
    void HandleMenu();
    bool PickByContains(const std::string& needle);
    bool PickNowOrArm(const char* entry, TimedIntent& intent);
    [[nodiscard]] std::string MenuSignature() const;
    void OnCommandRejected();

    void ExpireTimedIntents();
    void ExpireIntent(TimedIntent& intent, const char* name) const;
    void ClearMenuTracking();
    bool LogMenuIfNew(const std::string& sig);
    void MaybeResyncStalledMenu(const std::string& sig);
    void DiscardStuckMenu(const std::string& sig);
    bool MaybeCloseStaleMenu();
    bool HandleAutoPicks(const std::string& sig);
    bool HandlePendingCompletions();
    [[nodiscard]] bool RepositionWalking() const;
    bool HandleRepositionFlow();
    bool HandleIntentPrompts();

    enum class Intent { None, Reposition, Service };

    [[nodiscard]] bool HasActiveIntent() const;
    void OpenIntent(Intent intent);
    void CloseIntent();

    GsxRemoteApiClient* client_;
    GsxRemoteState* state_;
    const AutomationSettings* settings_;
    DomainLogger* logger_;
    CommBusPluginClient* pluginClient_;

    enum class Reposition { Idle, Opening, PickingRoot, AwaitingSubmenu, Done };

    Reposition reposition_ = Reposition::Idle;
    TimedIntent completingPushback_;
    TimedIntent completingRefuel_;
    TimedIntent completingBoarding_;
    TimedIntent confirmingEngines_;

    Intent intent_ = Intent::None;
    long long intentSinceMs_ = 0;
    std::vector<PendingRequest> pending_;
    std::function<long long()> nowMs_;
    std::string lastPickedSig_;
    std::string lastDiagSig_;
    std::string watchedSig_;
    long long watchedSinceMs_ = 0;
    int resyncCount_ = 0;
    bool resyncPending_ = false;
    std::string resyncSig_;
    std::string discardedSig_;
    mutable long long lastActionMs_ = 0;
    bool panelOpenSpent_ = false;
    bool panelCloseSpent_ = false;
    bool panelOpenedByUs_ = false;
    long long panelOpenSentMs_ = 0;

    static constexpr long long kIntentTtlMs = 60000;
    static constexpr long long kCompleteTtlMs = 20000;
    static constexpr long long kResyncDelayMs = 1500;
    static constexpr int kMaxResyncs = 3;
    static constexpr long long kMenuSettleMs = 1500;
    static constexpr long long kTriggerRetryMs = 10000;
    static constexpr int kMaxTriggerAttempts = 3;
    static constexpr long long kPanelOpenWaitMs = 8000;
};

#endif //GSX_INTEGRATOR_CLIENT_GSXMENUNAVIGATOR_H
