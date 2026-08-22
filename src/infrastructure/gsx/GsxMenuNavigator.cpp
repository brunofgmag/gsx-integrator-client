#include "GsxMenuNavigator.h"

#include <algorithm>
#include <cctype>
#include <format>
#include <QDateTime>
#include <QJsonObject>
#include <QString>
#include "GsxLVars.h"
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
    constexpr auto kLoadingInProgressText = "loading in progress";
    constexpr auto kBoardingPassengersText = "Boarding passengers now";
    constexpr auto kBoardCrewQuestion = "board crew";
    constexpr auto kDeIceQuestion = "de-icing";
    constexpr auto kAirstairsQuestion = "own airstairs";

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

void GsxMenuNavigator::CallJetway()
{
    TriggerService("OperateJetways");
}

void GsxMenuNavigator::CallStairs()
{
    TriggerService("OperateStairs");
}

void GsxMenuNavigator::RepositionAircraft()
{
    if (reposition_ != Reposition::Idle)
    {
        return;
    }

    OpenIntent(Intent::Reposition);
    reposition_ = Reposition::Opening;
    OpenMenu();
}

void GsxMenuNavigator::RequestSimbriefLoad()
{
    ArmRequest("command.run", QJsonObject{{"command", "RELOAD_SIMBRIEF"}}, "RELOAD_SIMBRIEF", {});
}

void GsxMenuNavigator::RequestBoarding()
{
    TriggerService("Boarding");
}

void GsxMenuNavigator::RequestDeboarding()
{
    TriggerService("Deboarding");
}

void GsxMenuNavigator::RequestPushback()
{
    TriggerService(gsx::services::Id(GroundService::Departure));
}

void GsxMenuNavigator::RequestRefueling()
{
    TriggerService("Refueling");
}

void GsxMenuNavigator::ToggleGpu()
{
    TriggerService(gsx::services::Id(GroundService::Gpu));
}

void GsxMenuNavigator::RequestCatering()
{
    TriggerService(gsx::services::Id(GroundService::Catering));
}

void GsxMenuNavigator::RequestLavatory()
{
    TriggerService(gsx::services::Id(GroundService::Lavatory));
}

void GsxMenuNavigator::RequestWater()
{
    TriggerService(gsx::services::Id(GroundService::Water));
}

void GsxMenuNavigator::RequestCleaning()
{
    TriggerService(gsx::services::Id(GroundService::Cleaning));
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

void GsxMenuNavigator::CompleteRefuel()
{
    completingRefuel_ = {true, nowMs_()};

    OpenIntent(Intent::Service);
    OpenMenu();
}

void GsxMenuNavigator::CompleteBoarding()
{
    completingBoarding_ = {true, nowMs_()};

    OpenIntent(Intent::Service);
    OpenMenu();
}

void GsxMenuNavigator::DisableGsxMenu()
{
    (void)client_->SendCommand("menu.close");

    reposition_ = Reposition::Idle;
    CloseIntent();
}

void GsxMenuNavigator::Reset()
{
    reposition_ = Reposition::Idle;
    completingPushback_ = {};
    completingRefuel_ = {};
    completingBoarding_ = {};
    confirmingEngines_ = {};
    intent_ = Intent::None;
    intentSinceMs_ = 0;
    lastPickedSig_.clear();
    lastDiagSig_.clear();
    watchedSig_.clear();
    resyncCount_ = 0;
    resyncPending_ = false;
    lastActionMs_ = 0;
    pending_.clear();
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
    HandleMenu();
    PumpRequests();
}

void GsxMenuNavigator::HandleMenu()
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
    ExpireIntent(completingBoarding_, "complete-boarding");
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

    if (nowMs_() - watchedSinceMs_ < kResyncDelayMs)
    {
        return;
    }

    if (MaybeCloseStaleMenu())
    {
        return;
    }

    const bool automationInterested = settings_ == nullptr || settings_->autoSelectGsxChoice
        || settings_->autoDeice || HasActiveIntent();
    if (!automationInterested || resyncCount_ >= kMaxResyncs)
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

bool GsxMenuNavigator::MaybeCloseStaleMenu()
{
    const bool repositionLeftover = !RepositionWalking() && HasActiveIntent()
        && Contains(state_->menu.title, kSelectPositionText);
    if (!repositionLeftover)
    {
        return false;
    }

    watchedSinceMs_ = nowMs_();
    lastActionMs_ = nowMs_();
    (void)client_->SendCommand("menu.close");
    logger_->LogInfo(std::format("RemoteAPI closing stale menu '{}'", state_->menu.title));

    return true;
}

bool GsxMenuNavigator::HandleAutoPicks(const std::string& sig)
{
    if (sig == lastPickedSig_)
    {
        return false;
    }

    const auto& menu = state_->menu;

    if (settings_ != nullptr && settings_->autoDeice
        && Contains(menu.title, kDeIceQuestion)
        && PickByContains("Yes"))
    {
        return true;
    }

    if (Contains(menu.title, kAirstairsQuestion))
    {
        const bool ownStairs = settings_ != nullptr && settings_->useAircraftStairs;
        if (PickByContains(ownStairs ? "Yes" : "No"))
        {
            return true;
        }
    }

    if ((settings_ == nullptr || settings_->autoSelectGsxChoice)
        && (PickByContains(kGsxChoiceText) || PickByContains(kBlockFuelText)))
    {
        return true;
    }

    if (Contains(menu.title, kBoardCrewQuestion))
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

    if (completingBoarding_.active)
    {
        if (Contains(state_->menu.title, kServiceInProgressTitle))
        {
            if (PickByContains(kCompleteNowText))
            {
                completingBoarding_ = {};
            }

            return true;
        }

        if (PickByContains(kLoadingInProgressText) || PickByContains(kBoardingPassengersText))
        {
            return true;
        }
    }

    return false;
}

bool GsxMenuNavigator::RepositionWalking() const
{
    return reposition_ == Reposition::Opening
        || reposition_ == Reposition::PickingRoot
        || reposition_ == Reposition::AwaitingSubmenu;
}

bool GsxMenuNavigator::HandleRepositionFlow()
{
    if (!RepositionWalking())
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

void GsxMenuNavigator::TriggerService(const char* serviceId)
{
    OpenIntent(Intent::Service);
    ShowGsxToolbar();

    ArmRequest("service.trigger",
               QJsonObject{{"service", QString::fromLatin1(serviceId)}},
               serviceId,
               serviceId);
}

void GsxMenuNavigator::ArmRequest(QString verb, QJsonObject args, std::string label, std::string confirmId)
{
    const auto same = std::ranges::find_if(pending_, [&](const PendingRequest& request)
    {
        return request.verb == verb && request.label == label;
    });

    if (same != pending_.end())
    {
        pending_.erase(same);
    }

    pending_.push_back({std::move(verb), std::move(args), std::move(label), std::move(confirmId)});

    PumpRequests();
}

void GsxMenuNavigator::PumpRequests()
{
    for (auto it = pending_.begin(); it != pending_.end();)
    {
        if (WasTaken(*it))
        {
            it = pending_.erase(it);

            continue;
        }

        if (it->attempts >= kMaxTriggerAttempts)
        {
            logger_->LogInfo(std::format("RemoteAPI '{}' never taken by GSX after {} attempts",
                                         it->label, it->attempts));
            it = pending_.erase(it);

            continue;
        }

        ++it;
    }

    if (!IsMenuSettled())
    {
        return;
    }

    for (PendingRequest& request : pending_)
    {
        if (request.attempts > 0 && (nowMs_() - request.lastSentMs) < kTriggerRetryMs)
        {
            continue;
        }

        SendRequest(request);

        return;
    }
}

void GsxMenuNavigator::SendRequest(PendingRequest& request)
{
    const bool closedMenu = state_->menu.shown;
    if (closedMenu)
    {
        (void)client_->SendCommand("menu.close");
        ClearMenuTracking();
    }

    lastActionMs_ = nowMs_();
    request.lastSentMs = lastActionMs_;
    ++request.attempts;

    std::string note;
    if (request.attempts > 1)
    {
        note = std::format(" (attempt {} of {})", request.attempts, kMaxTriggerAttempts);
    }
    else if (closedMenu)
    {
        note = " (closed open menu first)";
    }

    logger_->LogInfo(std::format("RemoteAPI {} '{}'{}", request.verb.toStdString(), request.label, note));

    (void)client_->SendCommand(request.verb, request.args);
}

bool GsxMenuNavigator::WasTaken(const PendingRequest& request) const
{
    if (request.attempts == 0)
    {
        return false;
    }

    if (request.confirmId.empty())
    {
        return true;
    }

    const GsxRemoteService* service = FindService(*state_, request.confirmId);
    if (service == nullptr)
    {
        return false;
    }

    return service->stateRaw != static_cast<int>(GsxStateStatus::Callable) || !service->canTrigger;
}

void GsxMenuNavigator::ShowGsxToolbar() const
{
    if (settings_->openGsxOnRequests && pluginClient_ != nullptr && !pluginClient_->IsGsxToolbarActive())
    {
        logger_->LogInfo("RemoteAPI opening the GSX toolbar");
        (void)pluginClient_->OpenGsxToolbar();
    }
}

void GsxMenuNavigator::OpenMenu() const
{
    ShowGsxToolbar();

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
