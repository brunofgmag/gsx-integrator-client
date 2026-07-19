#ifndef GSX_INTEGRATOR_CLIENT_GSXMENUNAVIGATOR_H
#define GSX_INTEGRATOR_CLIENT_GSXMENUNAVIGATOR_H

#include <functional>
#include <string>
#include <utility>
#include <QObject>
#include "GsxRemoteState.h"
#include "../../domain/ports/GsxMenuGateway.h"

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

    [[nodiscard]] bool CallJetway() override;
    [[nodiscard]] bool CallStairs() override;
    [[nodiscard]] bool RepositionAircraft() override;
    [[nodiscard]] bool RequestSimbriefLoad() override;
    [[nodiscard]] bool RequestBoarding() override;
    [[nodiscard]] bool RequestDeboarding() override;
    [[nodiscard]] bool RequestPushback() override;
    [[nodiscard]] bool RequestRefueling() override;
    [[nodiscard]] bool ConfirmGoodEngines() override;
    [[nodiscard]] bool CompletePushback() override;
    [[nodiscard]] bool CompleteRefuel() override;
    [[nodiscard]] bool ToggleGpu() override;
    [[nodiscard]] bool RequestCatering() override;
    [[nodiscard]] bool RequestLavatory() override;
    [[nodiscard]] bool RequestWater() override;
    [[nodiscard]] bool RequestCleaning() override;

    [[nodiscard]] bool IsMenuSettled() const override;

    void OpenMenu() const;
    void ShowGsxToolbar() const;
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

    bool TriggerService(const char* serviceId);
    bool PickByContains(const std::string& needle);
    bool PickNowOrArm(const char* entry, TimedIntent& intent);
    [[nodiscard]] std::string MenuSignature() const;
    void OnCommandRejected();

    void ExpireTimedIntents();
    void ExpireIntent(TimedIntent& intent, const char* name) const;
    void ClearMenuTracking();
    bool LogMenuIfNew(const std::string& sig);
    void MaybeResyncStalledMenu(const std::string& sig);
    bool MaybeCloseStaleMenu();
    bool HandleAutoPicks(const std::string& sig);
    bool HandlePendingCompletions();
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
    TimedIntent confirmingEngines_;

    Intent intent_ = Intent::None;
    long long intentSinceMs_ = 0;
    std::function<long long()> nowMs_;
    std::string lastPickedSig_;
    std::string lastDiagSig_;
    std::string watchedSig_;
    long long watchedSinceMs_ = 0;
    int resyncCount_ = 0;
    bool resyncPending_ = false;
    std::string resyncSig_;
    mutable long long lastActionMs_ = 0;

    static constexpr long long kIntentTtlMs = 60000;
    static constexpr long long kCompleteTtlMs = 20000;
    static constexpr long long kResyncDelayMs = 1500;
    static constexpr int kMaxResyncs = 3;
    static constexpr long long kMenuSettleMs = 1500;
};

#endif //GSX_INTEGRATOR_CLIENT_GSXMENUNAVIGATOR_H
