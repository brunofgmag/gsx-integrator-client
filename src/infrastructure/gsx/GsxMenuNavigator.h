#ifndef GSX_INTEGRATOR_CLIENT_GSXMENUNAVIGATOR_H
#define GSX_INTEGRATOR_CLIENT_GSXMENUNAVIGATOR_H

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include "GsxRemoteState.h"
#include "../../domain/model/MemoryBag.h"
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
    void RequestDepartureClearance() override;
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
    [[nodiscard]] bool WereStairsKeptInPlace() const override;

    [[nodiscard]] bool IsMenuSettled() const;

    void OpenMenu() const;
    void OpenPushbackPanel() override;
    void ClosePushbackPanel() override;
    void OnTurnaroundTurned() override;
    void OnPushbackStarted() override;

    void OnMenuChanged();
    void OnSnapshot();
    void DisableGsxMenu() override;

    void Reset();

    [[nodiscard]] MemoryBag TakeMemory() const;
    void RestoreMemory(const MemoryBag& memory);

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
        long long armedMs = 0;
        int attempts = 0;
        bool firesWhileUnderway = false;

        [[nodiscard]] bool WasSent() const { return attempts > 0; }
        [[nodiscard]] bool UndoneByASecondSend() const;
        [[nodiscard]] bool Matches(const QString& otherVerb, const std::string& otherLabel) const
        {
            return verb == otherVerb && label == otherLabel;
        }
        [[nodiscard]] bool RetryWindowElapsed(const long long now) const
        {
            return now - lastSentMs >= kTriggerRetryMs;
        }
        [[nodiscard]] bool GiveUpWindowElapsed(const long long now) const
        {
            return now - lastSentMs >= kToggleGiveUpMs;
        }
        [[nodiscard]] bool LeftUnsentForTooLong(const long long now) const
        {
            return !WasSent() && now - armedMs >= kToggleGiveUpMs;
        }
        [[nodiscard]] bool IsDueToSend(const long long now) const
        {
            return !WasSent() || (!UndoneByASecondSend() && RetryWindowElapsed(now));
        }
    };

    enum class DropReason : std::uint8_t { None, Taken, AlreadyUnderway, ToggleGaveUp, NeverTaken, NeverLeft };

    class PassScope;

    [[nodiscard]] static QJsonObject PendingToJson(const PendingRequest& request);
    [[nodiscard]] static std::optional<PendingRequest> PendingFromJson(const QJsonObject& object, long long now);
    [[nodiscard]] std::string PendingToText() const;
    [[nodiscard]] static std::vector<PendingRequest> PendingFromText(const std::string& text, long long now);

    [[nodiscard]] bool Send(const QString& verb, const QJsonObject& args = {}) const;
    void TriggerService(const char* serviceId, bool firesWhileUnderway = false);
    void SyncGsxToolbar() const;
    [[nodiscard]] GsxPanelMode PanelMode() const;
    void CloseThePanelWeOpened(const char* logLine);
    void RearmPanelLatches();
    [[nodiscard]] bool IsWaitingForThePanel();
    void ArmRequest(QString verb, QJsonObject args, std::string label, std::string confirmId,
                    bool firesWhileUnderway = false);
    [[nodiscard]] bool IsAlreadyUnderway(const PendingRequest& request) const;
    [[nodiscard]] bool IsServiceUnderway(const std::string& serviceId) const;
    [[nodiscard]] bool PassengersAreFlowing() const;
    [[nodiscard]] DropReason DropReasonFor(const PendingRequest& request, long long now) const;
    void LogDrop(const PendingRequest& request, DropReason reason) const;
    void PumpRequests();
    void SendRequest(PendingRequest& request);
    [[nodiscard]] bool WasTaken(const PendingRequest& request) const;
    void HandleMenu();
    bool PickFirstMatching(const std::function<bool(const std::string&)>& matches);
    bool PickByContains(const std::string& needle);
    bool PickByPrefix(const std::string& needle);
    bool PickNowOrArm(const char* entry, TimedIntent& intent);
    [[nodiscard]] std::string MenuSignature() const;
    void OnCommandRejected();

    void ExpireTimedIntents();
    void ExpireIntent(TimedIntent& intent, const char* name) const;
    void ClearMenuTracking();
    bool LogMenuIfNew(const std::string& sig);
    void MaybeResyncStalledMenu(const std::string& sig);
    void DiscardStuckMenu(const std::string& sig);
    void LogMenuLeftOpen(const std::string& sig);
    bool MaybeCloseStaleMenu();
    bool HandleAutoPicks();
    bool HandlePendingCompletions();
    bool CompleteTheServiceItOpened();
    [[nodiscard]] bool RepositionWalking() const;
    bool HandleRepositionFlow();
    bool HandleIntentPrompts();

    enum class Intent : std::uint8_t { None, Reposition, Service };

    [[nodiscard]] bool HasActiveIntent() const;
    void OpenIntent(Intent intent);
    void CloseIntent();

    GsxRemoteApiClient* client_;
    GsxRemoteState* state_;
    const AutomationSettings* settings_;
    DomainLogger* logger_;
    CommBusPluginClient* pluginClient_;

    enum class Reposition : std::uint8_t { Idle, Opening, PickingRoot, AwaitingSubmenu, Done };

    Reposition reposition_ = Reposition::Idle;
    TimedIntent completingPushback_;
    TimedIntent completingRefuel_;
    TimedIntent completingBoarding_;
    TimedIntent* serviceOpenedBy_ = nullptr;
    TimedIntent confirmingEngines_;

    Intent intent_ = Intent::None;
    long long intentSinceMs_ = 0;
    std::vector<PendingRequest> pending_;
    std::function<long long()> nowMs_;
    std::string lastPickedSig_;
    std::string lastDiagSig_;
    std::optional<std::string> watchedSig_;
    long long watchedSinceMs_ = 0;
    int resyncCount_ = 0;
    bool resyncPending_ = false;
    std::string resyncSig_;
    std::string discardedSig_;
    std::string leftOpenSig_;
    mutable long long lastActionMs_ = 0;
    mutable int passDepth_ = 0;
    mutable bool sendRefused_ = false;
    bool panelOpenSpent_ = false;
    bool panelCloseSpent_ = false;
    bool panelOpenedByUs_ = false;
    bool stairsKeptInPlace_ = false;
    bool deIceYesSpent_ = false;
    long long panelOpenSentMs_ = 0;

    static constexpr long long kIntentTtlMs = 60000;
    static constexpr long long kCompleteTtlMs = 20000;
    static constexpr long long kResyncDelayMs = 1500;
    static constexpr int kMaxResyncs = 3;
    static constexpr long long kMenuSettleMs = 1500;
    static constexpr long long kTriggerRetryMs = 20000;
    static constexpr long long kToggleGiveUpMs = 90000;
    static constexpr int kMaxTriggerAttempts = 3;
    static constexpr long long kPanelOpenWaitMs = 8000;
};

#endif //GSX_INTEGRATOR_CLIENT_GSXMENUNAVIGATOR_H
