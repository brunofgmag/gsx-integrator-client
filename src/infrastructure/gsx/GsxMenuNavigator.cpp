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
    constexpr auto kPushbackDirectionText = "pushback direction";
    constexpr auto kConfirmEnginesText = "Confirm good engine";
    constexpr auto kCompletePushbackText = "complete pushback procedure";
    constexpr auto kServiceInProgressTitle = "Service in progress";
    constexpr auto kCompleteNowText = "Complete now";
    constexpr auto kRefuelingLoadedText = "loaded";
    constexpr auto kLoadingInProgressText = "loading in progress";
    constexpr auto kBoardingPassengersText = "Boarding passengers now";
    constexpr auto kBoardCrewQuestion = "board crew";
    constexpr auto kDeboardCrewQuestion = "deboard crew";
    constexpr auto kDeIceQuestion = "de-icing";
    constexpr auto kAirstairsQuestion = "own airstairs";
    constexpr auto kJetwayAnswerText = "use jetway";
    constexpr auto kBlockedSpotText = "waiting for the spot";
    constexpr auto kRemoveStairsText = "Remove the stairs";

    const char* CrewChoiceEntry(const CrewChoice choice)
    {
        switch (choice)
        {
        case CrewChoice::Nobody: return "No";
        case CrewChoice::Crew: return "Crew";
        case CrewChoice::Pilots: return "Pilots";
        case CrewChoice::Both: return "Both";
        }
        return "Both";
    }

    bool StartsWithFold(const std::string& hay, const std::string& needle)
    {
        if (needle.size() > hay.size())
        {
            return false;
        }

        return std::ranges::equal(hay.begin(), hay.begin() + static_cast<long long>(needle.size()),
                                  needle.begin(), needle.end(),
                                  [](const char x, const char y)
                                  {
                                      return std::tolower(static_cast<unsigned char>(x)) == std::tolower(
                                          static_cast<unsigned char>(y));
                                  });
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
    TriggerService(gsx::services::Id(GroundService::Gpu), true);
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
    discardedSig_.clear();
    resyncCount_ = 0;
    resyncPending_ = false;
    lastActionMs_ = 0;
    RearmPanelLatches();
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
    if (!automationInterested)
    {
        return;
    }

    if (resyncCount_ >= kMaxResyncs)
    {
        DiscardStuckMenu(sig);

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

void GsxMenuNavigator::DiscardStuckMenu(const std::string& sig)
{
    if (sig == discardedSig_)
    {
        return;
    }

    if (Contains(state_->menu.title, kPushbackDirectionText))
    {
        discardedSig_ = sig;
        logger_->LogInfo(std::format("RemoteAPI leaving the pushback direction menu open: '{}'",
                                     state_->menu.title));

        return;
    }

    discardedSig_ = sig;
    watchedSinceMs_ = nowMs_();
    lastActionMs_ = nowMs_();
    (void)client_->SendCommand("menu.close");
    logger_->LogInfo(std::format("RemoteAPI closing the menu the resyncs could not move: '{}'",
                                 state_->menu.title));
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
        if (ownStairs && PickByContains(kJetwayAnswerText))
        {
            return true;
        }

        if (PickByPrefix(ownStairs ? "Yes" : "No"))
        {
            return true;
        }
    }

    if (Contains(menu.title, kBlockedSpotText)
        && Contains(menu.title, kRemoveStairsText)
        && PickByPrefix("Yes"))
    {
        return true;
    }

    if ((settings_ == nullptr || settings_->autoSelectGsxChoice)
        && (PickByContains(kGsxChoiceText) || PickByContains(kBlockFuelText)))
    {
        return true;
    }

    if (Contains(menu.title, kDeboardCrewQuestion))
    {
        const CrewChoice choice = settings_ != nullptr ? settings_->crewDeboarding : CrewChoice::Both;

        return PickByContains(CrewChoiceEntry(choice));
    }

    if (Contains(menu.title, kBoardCrewQuestion))
    {
        const CrewChoice choice = settings_ != nullptr ? settings_->crewBoarding : CrewChoice::Both;

        return PickByContains(CrewChoiceEntry(choice));
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

void GsxMenuNavigator::TriggerService(const char* serviceId, const bool toggles)
{
    OpenIntent(Intent::Service);
    SyncGsxToolbar();

    ArmRequest("service.trigger",
               QJsonObject{{"service", QString::fromLatin1(serviceId)}},
               serviceId,
               serviceId,
               toggles);
}

void GsxMenuNavigator::ArmRequest(QString verb, QJsonObject args, std::string label, std::string confirmId,
                                  const bool toggles)
{
    const auto same = std::ranges::find_if(pending_, [&](const PendingRequest& request)
    {
        return request.verb == verb && request.label == label;
    });

    PendingRequest request{std::move(verb), std::move(args), std::move(label), std::move(confirmId),
                           0, 0, toggles};

    if (same != pending_.end())
    {
        request.lastSentMs = same->lastSentMs;
        request.attempts = same->attempts;
        pending_.erase(same);
    }

    pending_.push_back(std::move(request));

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

        if (IsAlreadyUnderway(*it))
        {
            logger_->LogInfo(std::format("RemoteAPI '{}' already underway; not requesting", it->label));
            it = pending_.erase(it);

            continue;
        }

        if (gsx::services::ASecondPickUndoesIt(it->confirmId) && it->attempts > 0
            && (nowMs_() - it->lastSentMs) >= kToggleGiveUpMs)
        {
            logger_->LogInfo(std::format(
                "RemoteAPI stopped waiting for '{}'; a service that toggles is never sent twice, so the request is dropped",
                it->label));
            it = pending_.erase(it);

            continue;
        }

        if (it->attempts >= kMaxTriggerAttempts && (nowMs_() - it->lastSentMs) >= kTriggerRetryMs)
        {
            logger_->LogInfo(std::format("RemoteAPI '{}' never taken by GSX after {} attempts",
                                         it->label, it->attempts));
            it = pending_.erase(it);

            continue;
        }

        ++it;
    }

    if (IsWaitingForThePanel() || !IsMenuSettled())
    {
        return;
    }

    for (PendingRequest& request : pending_)
    {
        if (request.attempts > 0
            && (gsx::services::ASecondPickUndoesIt(request.confirmId)
                || (nowMs_() - request.lastSentMs) < kTriggerRetryMs))
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

bool GsxMenuNavigator::IsAlreadyUnderway(const PendingRequest& request) const
{
    if (request.toggles || request.attempts > 0 || request.confirmId.empty())
    {
        return false;
    }

    const GsxRemoteService* service = FindService(*state_, request.confirmId);
    if (service == nullptr)
    {
        return false;
    }

    return service->stateRaw == static_cast<int>(GsxStateStatus::Requested)
        || service->stateRaw == static_cast<int>(GsxStateStatus::Active);
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

GsxPanelMode GsxMenuNavigator::PanelMode() const
{
    return settings_->gsxPanelMode;
}

void GsxMenuNavigator::SyncGsxToolbar() const
{
    if (pluginClient_ == nullptr || PanelMode() != GsxPanelMode::AllRequests)
    {
        return;
    }

    if (!pluginClient_->IsGsxToolbarActive())
    {
        logger_->LogInfo("RemoteAPI opening the GSX toolbar");
        (void)pluginClient_->OpenGsxToolbar();
    }
}

void GsxMenuNavigator::OpenPushbackPanel()
{
    if (pluginClient_ == nullptr || PanelMode() != GsxPanelMode::OnPushback || panelOpenSpent_)
    {
        return;
    }

    if (pluginClient_->IsGsxToolbarActive())
    {
        panelOpenSpent_ = true;

        return;
    }

    if (!pluginClient_->OpenGsxToolbar())
    {
        return;
    }

    panelOpenSpent_ = true;
    panelOpenedByUs_ = true;
    panelOpenSentMs_ = nowMs_();
    logger_->LogInfo("RemoteAPI opening the GSX toolbar for the pushback menu");
}

void GsxMenuNavigator::ClosePanelAfterPushback()
{
    if (pluginClient_ == nullptr || PanelMode() != GsxPanelMode::OnPushback)
    {
        return;
    }

    if (!panelOpenedByUs_ || panelCloseSpent_ || !pluginClient_->IsGsxToolbarActive())
    {
        return;
    }

    if (!pluginClient_->CloseGsxToolbar())
    {
        return;
    }

    panelCloseSpent_ = true;
    logger_->LogInfo("RemoteAPI closing the GSX toolbar now that the pushback has started");
}

bool GsxMenuNavigator::IsWaitingForThePanel()
{
    if (panelOpenSentMs_ == 0 || pluginClient_ == nullptr)
    {
        return false;
    }

    if (pluginClient_->IsGsxToolbarActive())
    {
        panelOpenSentMs_ = 0;

        return false;
    }

    if ((nowMs_() - panelOpenSentMs_) < kPanelOpenWaitMs)
    {
        return true;
    }

    panelOpenSentMs_ = 0;
    logger_->LogInfo("RemoteAPI sending the pushback request without the GSX toolbar confirming it opened");

    return false;
}

void GsxMenuNavigator::RearmPanelLatches()
{
    panelOpenSpent_ = false;
    panelCloseSpent_ = false;
    panelOpenedByUs_ = false;
    panelOpenSentMs_ = 0;
}

void GsxMenuNavigator::OnTurnaroundTurned()
{
    RearmPanelLatches();
}

void GsxMenuNavigator::OnPushbackStarted()
{
    ClosePanelAfterPushback();
}

void GsxMenuNavigator::OpenMenu() const
{
    SyncGsxToolbar();

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

bool GsxMenuNavigator::PickFirstMatching(const std::function<bool(const std::string&)>& matches)
{
    const auto& e = state_->menu.entries;
    const auto& disabled = state_->menu.disabled;
    for (std::size_t i = 0; i < e.size(); ++i)
    {
        if (i < disabled.size() && disabled[i])
        {
            continue;
        }

        if (matches(e[i]))
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

bool GsxMenuNavigator::PickByContains(const std::string& needle)
{
    return PickFirstMatching([&needle](const std::string& entry) { return Contains(entry, needle); });
}

bool GsxMenuNavigator::PickByPrefix(const std::string& needle)
{
    return PickFirstMatching([&needle](const std::string& entry) { return StartsWithFold(entry, needle); });
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
