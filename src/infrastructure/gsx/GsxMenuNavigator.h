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

    void OpenMenu();
    void OnMenuChanged();
    void DisableGsxMenu() override;

    void Reset();

    void SetClockForTest(std::function<long long()> clock) { nowMs_ = std::move(clock); }

private:
    bool TriggerService(const char* serviceId);
    bool PickByContains(const std::string& needle);
    [[nodiscard]] std::string MenuSignature() const;
    void OnCommandRejected();

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

    Intent intent_ = Intent::None;
    long long intentSinceMs_ = 0;
    std::function<long long()> nowMs_;
    std::string lastPickedSig_;
    std::string lastDiagSig_;

    static constexpr long long kIntentTtlMs = 60000;
};

#endif //GSX_INTEGRATOR_CLIENT_GSXMENUNAVIGATOR_H
