#include "GsxMenuNavigator.h"

#include <algorithm>
#include <cctype>
#include <format>
#include <QDateTime>
#include <QJsonObject>
#include <QString>
#include "GsxRemoteApiClient.h"
#include "../commbus/CommBusPluginClient.h"
#include "../../domain/model/AutomationSettings.h"
#include "../../domain/ports/DomainLogger.h"

namespace
{
    constexpr auto kGsxChoiceText = "GSX choice";
    constexpr auto kBlockFuelText = "BLOCK FUEL from Simbrief";
    constexpr auto kSelectPositionText = "Select Position at";
    constexpr auto kRepositionRootText = "Reposition Aircraft";
    constexpr auto kRepositionHereText = "Reposition here";
    constexpr auto kPushTugQuestion = "Attach Pushback Tug";
    constexpr auto kConfirmEnginesText = "Confirm good engine";
    constexpr auto kInterruptPushbackTitle = "Interrupt pushback";
    constexpr auto kCompletePushbackText = "complete pushback procedure";
    constexpr auto kServiceInProgressTitle = "Service in progress";
    constexpr auto kCompleteNowText = "Complete now";
    constexpr auto kRefuelingLoadedText = "kg loaded";

    bool Contains(const std::string& hay, const std::string& needle)
    {
        const auto it = std::ranges::search(hay, needle,
                                            [](const char x, const char y)
                                            {
                                                return std::tolower(static_cast<unsigned char>(x)) == std::tolower(
                                                    static_cast<unsigned char>(y));
                                            }).begin();
        return it != hay.end();
    }
}

GsxMenuNavigator::GsxMenuNavigator(GsxRemoteApiClient* client,
                                   GsxRemoteState* state,
                                   const AutomationSettings* settings,
                                   DomainLogger* logger,
                                   CommBusPluginClient* pluginClient,
                                   QObject* parent) : QObject(parent),
                                                      client_(client),
                                                      state_(state),
                                                      settings_(settings),
                                                      logger_(logger),
                                                      pluginClient_(pluginClient)
{
    nowMs_ = [] { return static_cast<long long>(QDateTime::currentMSecsSinceEpoch()); };

    connect(client_, &GsxRemoteApiClient::ResultReceived,
            this, [this](const bool ok, const QString&)
            {
                if (!ok)
                {
                    OnCommandRejected();
                }
            });
}

bool GsxMenuNavigator::CallJetway()
{
    return TriggerService("OperateJetways");
}

bool GsxMenuNavigator::CallStairs()
{
    return TriggerService("OperateStairs");
}

bool GsxMenuNavigator::RepositionAircraft()
{
    if (reposition_ == Reposition::Idle)
    {
        OpenIntent(Intent::Reposition);
        reposition_ = Reposition::Opening;
        OpenMenu();
    }

    return true;
}

bool GsxMenuNavigator::RequestSimbriefLoad()
{
    return client_->SendCommand("command.run", QJsonObject{{"command", "RELOAD_SIMBRIEF"}});
}

bool GsxMenuNavigator::RequestBoarding()
{
    return TriggerService("Boarding");
}

bool GsxMenuNavigator::RequestDeboarding()
{
    return TriggerService("Deboarding");
}

bool GsxMenuNavigator::RequestPushback()
{
    return TriggerService("Departure");
}

bool GsxMenuNavigator::RequestRefueling()
{
    return TriggerService("Refueling");
}

bool GsxMenuNavigator::ToggleGpu()
{
    return TriggerService("GPU");
}

bool GsxMenuNavigator::RequestCatering()
{
    return TriggerService("Catering");
}

bool GsxMenuNavigator::RequestLavatory()
{
    return TriggerService("Lavatory");
}

bool GsxMenuNavigator::RequestWater()
{
    return TriggerService("Water");
}

bool GsxMenuNavigator::RequestCleaning()
{
    return TriggerService("Cleaning");
}

bool GsxMenuNavigator::ConfirmGoodEngines()
{
    if (PickByContains(kConfirmEnginesText))
    {
        return true;
    }

    OpenIntent(Intent::Service);
    OpenMenu();

    return false;
}

bool GsxMenuNavigator::CompletePushback()
{
    if (PickByContains(kCompletePushbackText))
    {
        completingPushback_ = false;

        return true;
    }

    completingPushback_ = true;

    OpenIntent(Intent::Service);

    OpenMenu();

    return false;
}

bool GsxMenuNavigator::CompleteRefuel()
{
    completingRefuel_ = true;

    OpenIntent(Intent::Service);

    OpenMenu();

    return true;
}

void GsxMenuNavigator::DisableGsxMenu()
{
    if (state_->menu.shown)
    {
        (void)client_->SendCommand("menu.toggle");
    }

    reposition_ = Reposition::Idle;
    CloseIntent();
}

void GsxMenuNavigator::Reset()
{
    reposition_ = Reposition::Idle;
    completingPushback_ = false;
    completingRefuel_ = false;
    intent_ = Intent::None;
    intentSinceMs_ = 0;
    lastPickedSig_.clear();
    lastDiagSig_.clear();
    watchedSig_.clear();
    resyncCount_ = 0;
    resyncPending_ = false;
}

void GsxMenuNavigator::OnSnapshot()
{
    if (resyncPending_)
    {
        resyncPending_ = false;
        if (state_->menu.shown && MenuSignature() == resyncSig_)
        {
            lastPickedSig_.clear();
        }
    }

    OnMenuChanged();
}

void GsxMenuNavigator::OnMenuChanged()
{
    const auto& menu = state_->menu;
    if (!menu.shown)
    {
        lastPickedSig_.clear();
        lastDiagSig_.clear();
        watchedSig_.clear();
        return;
    }

    const std::string sig = MenuSignature();
    const bool newMenu = sig != lastDiagSig_;

    if (newMenu)
    {
        lastDiagSig_ = sig;
        std::string joined;
        for (const auto& entry : menu.entries)
        {
            if (!joined.empty()) joined += " | ";
            joined += entry;
        }
        logger_->LogInfo(std::format("RemoteAPI menu: '{}' -> [{}]", menu.title, joined));
    }

    if (sig != watchedSig_)
    {
        watchedSig_ = sig;
        watchedSinceMs_ = nowMs_();
        resyncCount_ = 0;
    }
    else if ((settings_ == nullptr || settings_->autoSelectGsxChoice || HasActiveIntent())
        && resyncCount_ < kMaxResyncs
        && (nowMs_() - watchedSinceMs_) >= kResyncDelayMs)
    {
        ++resyncCount_;
        watchedSinceMs_ = nowMs_();
        resyncPending_ = true;
        resyncSig_ = sig;
        (void)client_->SendCommand("state.get");
        logger_->LogInfo(std::format("RemoteAPI menu stalled: requesting snapshot resync {}/{} ('{}')",
                                     resyncCount_, kMaxResyncs, menu.title));
    }

    if ((settings_ == nullptr || settings_->autoSelectGsxChoice)
        && sig != lastPickedSig_
        && (PickByContains(kGsxChoiceText) || PickByContains(kBlockFuelText)))
    {
        return;
    }

    if (completingPushback_ && Contains(menu.title, kInterruptPushbackTitle))
    {
        if (PickByContains(kCompletePushbackText))
        {
            completingPushback_ = false;
        }

        return;
    }

    if (completingRefuel_)
    {
        if (Contains(menu.title, kServiceInProgressTitle))
        {
            if (PickByContains(kCompleteNowText))
            {
                completingRefuel_ = false;
            }

            return;
        }

        if (PickByContains(kRefuelingLoadedText))
        {
            return;
        }
    }

    if (!HasActiveIntent())
    {
        return;
    }

    if (sig == lastPickedSig_)
    {
        return;
    }

    if (reposition_ == Reposition::Opening
        || reposition_ == Reposition::PickingRoot
        || reposition_ == Reposition::AwaitingSubmenu)
    {
        if (Contains(menu.title, kSelectPositionText))
        {
            if (PickByContains(kRepositionHereText))
            {
                reposition_ = Reposition::Done;
            }

            return;
        }

        if (PickByContains(kRepositionRootText))
        {
            reposition_ = Reposition::AwaitingSubmenu;

            return;
        }
        reposition_ = Reposition::PickingRoot;
    }

    if (Contains(menu.title, kPushTugQuestion))
    {
        (void)PickByContains("No");

        return;
    }

    if (Contains(menu.title, kConfirmEnginesText))
    {
        (void)ConfirmGoodEngines();

        return;
    }

    if (newMenu)
    {
        logger_->LogInfo(std::format("RemoteAPI menu unmatched by intent: '{}'", menu.title));
    }
}

bool GsxMenuNavigator::TriggerService(const char* serviceId)
{
    OpenIntent(Intent::Service);
    OpenMenu();
    return client_->SendCommand("service.trigger", QJsonObject{{"service", QString::fromLatin1(serviceId)}});
}

void GsxMenuNavigator::OpenMenu() const
{
    if (pluginClient_ != nullptr && !pluginClient_->IsGsxToolbarActive())
    {
        (void)pluginClient_->OpenGsxToolbar();
    }

    if (!state_->menu.shown)
    {
        (void)client_->SendCommand("menu.toggle");
    }
}

bool GsxMenuNavigator::PickByContains(const std::string& needle)
{
    const auto& e = state_->menu.entries;
    const auto& disabled = state_->menu.disabled;
    for (std::size_t i = 0; i < e.size(); ++i)
    {
        if (i < disabled.size() && disabled[i])
        {
            continue;
        }
        if (Contains(e[i], needle))
        {
            client_->SendCommand("menu.pick", QJsonObject{{"index", static_cast<int>(i)}});
            logger_->LogInfo(std::format("RemoteAPI menu.pick {} ({})", i, e[i]));
            lastPickedSig_ = MenuSignature();

            return true;
        }
    }

    return false;
}

std::string GsxMenuNavigator::MenuSignature() const
{
    std::string sig = state_->menu.title;
    for (const auto& entry : state_->menu.entries)
    {
        sig += '\n';
        sig += entry;
    }
    return sig;
}

void GsxMenuNavigator::OnCommandRejected()
{
    lastPickedSig_.clear();
}

bool GsxMenuNavigator::HasActiveIntent() const
{
    if (intent_ == Intent::None)
    {
        return false;
    }

    return (nowMs_() - intentSinceMs_) < kIntentTtlMs;
}

void GsxMenuNavigator::OpenIntent(const Intent intent)
{
    intent_ = intent;
    intentSinceMs_ = nowMs_();
}

void GsxMenuNavigator::CloseIntent()
{
    intent_ = Intent::None;
}
