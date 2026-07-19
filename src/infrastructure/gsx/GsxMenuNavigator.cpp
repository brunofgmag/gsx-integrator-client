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
    constexpr auto kCompletePushbackText = "complete pushback procedure";
    constexpr auto kServiceInProgressTitle = "Service in progress";
    constexpr auto kCompleteNowText = "Complete now";
    constexpr auto kRefuelingLoadedText = "loaded";
    constexpr auto kBoardCrewQuestion = "board crew";
    constexpr auto kDeIceQuestion = "de-icing";

    const char* CrewBoardingEntry(const CrewBoarding choice)
    {
        switch (choice)
        {
        case CrewBoarding::Nobody: return "No";
        case CrewBoarding::Crew: return "Crew";
        case CrewBoarding::Pilots: return "Pilots";
        case CrewBoarding::Both: return "Both";
        }
        return "Both";
    }

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

bool GsxMenuNavigator::PickNowOrArm(const char* entry, TimedIntent& intent)
{
    if (PickByContains(entry))
    {
        intent = {};

        return true;
    }

    intent = {true, nowMs_()};

    OpenIntent(Intent::Service);
    OpenMenu();

    return false;
}

bool GsxMenuNavigator::ConfirmGoodEngines()
{
    return PickNowOrArm(kConfirmEnginesText, confirmingEngines_);
}

bool GsxMenuNavigator::CompletePushback()
{
    return PickNowOrArm(kCompletePushbackText, completingPushback_);
}

bool GsxMenuNavigator::CompleteRefuel()
{
    completingRefuel_ = {true, nowMs_()};

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
    completingPushback_ = {};
    completingRefuel_ = {};
    confirmingEngines_ = {};
    intent_ = Intent::None;
    intentSinceMs_ = 0;
    lastPickedSig_.clear();
    lastDiagSig_.clear();
    watchedSig_.clear();
    resyncCount_ = 0;
    resyncPending_ = false;
    lastActionMs_ = 0;
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
    ExpireTimedIntents();

    if (!state_->menu.shown)
    {
        ClearMenuTracking();

        return;
    }

    const std::string sig = MenuSignature();
    const bool newMenu = LogMenuIfNew(sig);
    MaybeResyncStalledMenu(sig);

    if (HandleAutoPicks(sig))
    {
        return;
    }

    if (HandlePendingCompletions())
    {
        return;
    }

    if (!HasActiveIntent() || sig == lastPickedSig_)
    {
        return;
    }

    if (HandleRepositionFlow())
    {
        return;
    }

    if (HandleIntentPrompts())
    {
        return;
    }

    if (newMenu)
    {
        logger_->LogInfo(std::format("RemoteAPI menu unmatched by intent: '{}'", state_->menu.title));
    }
}

void GsxMenuNavigator::ExpireTimedIntents()
{
    ExpireIntent(completingPushback_, "complete-pushback");
    ExpireIntent(completingRefuel_, "complete-refuel");
    ExpireIntent(confirmingEngines_, "confirm-engines");
}

void GsxMenuNavigator::ExpireIntent(TimedIntent& intent, const char* name) const
{
    if (intent.active && nowMs_() - intent.sinceMs >= kCompleteTtlMs)
    {
        intent = {};
        logger_->LogInfo(std::format("RemoteAPI {} intent expired", name));
    }
}

void GsxMenuNavigator::ClearMenuTracking()
{
    lastPickedSig_.clear();
    lastDiagSig_.clear();
    watchedSig_.clear();
}

bool GsxMenuNavigator::LogMenuIfNew(const std::string& sig)
{
    if (sig == lastDiagSig_)
    {
        return false;
    }

    lastDiagSig_ = sig;

    std::string joined;
    for (const auto& entry : state_->menu.entries)
    {
        if (!joined.empty()) joined += " | ";
        joined += entry;
    }
    logger_->LogInfo(std::format("RemoteAPI menu: '{}' -> [{}]", state_->menu.title, joined));

    return true;
}

void GsxMenuNavigator::MaybeResyncStalledMenu(const std::string& sig)
{
    if (sig != watchedSig_)
    {
        watchedSig_ = sig;
        watchedSinceMs_ = nowMs_();
        resyncCount_ = 0;

        return;
    }

    const bool automationInterested = settings_ == nullptr || settings_->autoSelectGsxChoice
        || settings_->autoDeice || HasActiveIntent();
    if (!automationInterested || resyncCount_ >= kMaxResyncs || nowMs_() - watchedSinceMs_ < kResyncDelayMs)
    {
        return;
    }

    ++resyncCount_;
    watchedSinceMs_ = nowMs_();
    resyncPending_ = true;
    resyncSig_ = sig;
    lastActionMs_ = nowMs_();
    (void)client_->SendCommand("state.get");
    logger_->LogInfo(std::format("RemoteAPI menu stalled: requesting snapshot resync {}/{} ('{}')",
                                 resyncCount_, kMaxResyncs, state_->menu.title));
}

bool GsxMenuNavigator::HandleAutoPicks(const std::string& sig)
{
    const auto& menu = state_->menu;

    if (settings_ != nullptr && settings_->autoDeice
        && sig != lastPickedSig_
        && Contains(menu.title, kDeIceQuestion)
        && PickByContains("Yes"))
    {
        return true;
    }

    if ((settings_ == nullptr || settings_->autoSelectGsxChoice)
        && sig != lastPickedSig_
        && (PickByContains(kGsxChoiceText) || PickByContains(kBlockFuelText)))
    {
        return true;
    }

    if (sig != lastPickedSig_ && Contains(menu.title, kBoardCrewQuestion))
    {
        const auto choice = settings_ != nullptr ? settings_->crewBoarding : CrewBoarding::Both;
        if (PickByContains(CrewBoardingEntry(choice)))
        {
            return true;
        }
    }

    return false;
}

bool GsxMenuNavigator::HandlePendingCompletions()
{
    if (completingPushback_.active && PickByContains(kCompletePushbackText))
    {
        completingPushback_ = {};

        return true;
    }

    if (confirmingEngines_.active && PickByContains(kConfirmEnginesText))
    {
        confirmingEngines_ = {};

        return true;
    }

    if (completingRefuel_.active)
    {
        if (Contains(state_->menu.title, kServiceInProgressTitle))
        {
            if (PickByContains(kCompleteNowText))
            {
                completingRefuel_ = {};
            }

            return true;
        }

        if (PickByContains(kRefuelingLoadedText))
        {
            return true;
        }
    }

    return false;
}

bool GsxMenuNavigator::HandleRepositionFlow()
{
    if (reposition_ != Reposition::Opening
        && reposition_ != Reposition::PickingRoot
        && reposition_ != Reposition::AwaitingSubmenu)
    {
        return false;
    }

    if (Contains(state_->menu.title, kSelectPositionText))
    {
        if (PickByContains(kRepositionHereText))
        {
            reposition_ = Reposition::Done;
        }

        return true;
    }

    if (PickByContains(kRepositionRootText))
    {
        reposition_ = Reposition::AwaitingSubmenu;

        return true;
    }

    reposition_ = Reposition::PickingRoot;

    return false;
}

bool GsxMenuNavigator::HandleIntentPrompts()
{
    if (Contains(state_->menu.title, kPushTugQuestion))
    {
        (void)PickByContains("No");

        return true;
    }

    if (Contains(state_->menu.title, kConfirmEnginesText))
    {
        (void)ConfirmGoodEngines();

        return true;
    }

    return false;
}

bool GsxMenuNavigator::TriggerService(const char* serviceId)
{
    OpenIntent(Intent::Service);
    OpenMenu();
    lastActionMs_ = nowMs_();

    return client_->SendCommand("service.trigger", QJsonObject{{"service", QString::fromLatin1(serviceId)}});
}

void GsxMenuNavigator::OpenMenu() const
{
    if (settings_->openGsxOnRequests && pluginClient_ != nullptr && !pluginClient_->IsGsxToolbarActive())
    {
        (void)pluginClient_->OpenGsxToolbar();
    }

    if (!state_->menu.shown)
    {
        lastActionMs_ = nowMs_();
        (void)client_->SendCommand("menu.toggle");
    }
}

bool GsxMenuNavigator::IsMenuSettled() const
{
    return !resyncPending_ && (nowMs_() - lastActionMs_) >= kMenuSettleMs;
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
            lastActionMs_ = nowMs_();
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
    if (intent != Intent::Reposition)
    {
        reposition_ = Reposition::Idle;
    }

    intent_ = intent;
    intentSinceMs_ = nowMs_();
}

void GsxMenuNavigator::CloseIntent()
{
    intent_ = Intent::None;
}
