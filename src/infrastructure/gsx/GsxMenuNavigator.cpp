#include "GsxMenuNavigator.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <format>
#include <QByteArray>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
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
    constexpr auto kConfirmationQuestion = "Are you sure";
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
    constexpr auto kRefuelingEntryText = "Refueling:";
    constexpr auto kLoadingInProgressText = "loading in progress";
    constexpr auto kBoardingPassengersText = "Boarding passengers now";
    constexpr auto kBoardCrewQuestion = "board crew";
    constexpr auto kDeboardCrewQuestion = "deboard crew";
    constexpr auto kDeIceQuestion = "de-icing";
    constexpr auto kAirstairsQuestion = "own airstairs";
    constexpr auto kJetwayAnswerText = "use jetway";
    constexpr auto kBlockedSpotText = "waiting for the spot";
    constexpr auto kRemoveStairsText = "Remove the stairs";
    constexpr auto kBoardingServiceId = "Boarding";
    constexpr auto kDeboardingServiceId = "Deboarding";
    constexpr auto kDeIceYesSpentEntry = "deIceYesSpent";
    constexpr auto kStairsKeptInPlaceEntry = "stairsKeptInPlace";
    constexpr auto kPanelOpenSpentEntry = "panelOpenSpent";
    constexpr auto kPanelCloseSpentEntry = "panelCloseSpent";
    constexpr auto kPanelOpenedByUsEntry = "panelOpenedByUs";
    constexpr auto kPendingRequestsEntry = "pendingRequests";
    constexpr auto kVerbKey = "verb";
    constexpr auto kArgsKey = "args";
    constexpr auto kLabelKey = "label";
    constexpr auto kConfirmIdKey = "confirmId";
    constexpr auto kLastSentKey = "lastSentMs";
    constexpr auto kArmedKey = "armedMs";
    constexpr auto kAttemptsKey = "attempts";
    constexpr auto kFiresWhileUnderwayKey = "firesWhileUnderway";
    constexpr auto kMenuCloseVerb = "menu.close";
    constexpr auto kMenuToggleVerb = "menu.toggle";
    constexpr auto kMenuPickVerb = "menu.pick";
    constexpr auto kStateGetVerb = "state.get";
    constexpr auto kServiceTriggerVerb = "service.trigger";
    constexpr auto kCommandRunVerb = "command.run";

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

    bool SameIgnoringCase(const char x, const char y)
    {
        return std::tolower(static_cast<unsigned char>(x)) == std::tolower(static_cast<unsigned char>(y));
    }

    bool StartsWithFold(const std::string& hay, const std::string& needle)
    {
        if (needle.size() > hay.size())
        {
            return false;
        }

        return std::ranges::equal(hay.begin(), hay.begin() + static_cast<long long>(needle.size()),
                                  needle.begin(), needle.end(), SameIgnoringCase);
    }

    bool Contains(const std::string& hay, const std::string& needle)
    {
        const auto it = std::ranges::search(hay, needle, SameIgnoringCase).begin();

        return it != hay.end();
    }

    bool IsArmedVerb(const QString& verb)
    {
        return verb == kServiceTriggerVerb || verb == kCommandRunVerb;
    }
}

bool GsxMenuNavigator::PendingRequest::UndoneByASecondSend() const
{
    return gsx::services::ASecondPickUndoesIt(confirmId);
}

class GsxMenuNavigator::PassScope
{
public:
    explicit PassScope(const GsxMenuNavigator& owner) : owner_(owner)
    {
        if (owner_.passDepth_++ == 0)
        {
            owner_.sendRefused_ = false;
        }
    }

    ~PassScope()
    {
        --owner_.passDepth_;
    }

    PassScope(const PassScope&) = delete;
    PassScope& operator=(const PassScope&) = delete;

private:
    const GsxMenuNavigator& owner_;
};

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
    ArmRequest(kCommandRunVerb, QJsonObject{{"command", "RELOAD_SIMBRIEF"}}, "RELOAD_SIMBRIEF", {});
}

void GsxMenuNavigator::RequestBoarding()
{
    stairsKeptInPlace_ = false;
    TriggerService(kBoardingServiceId);
}

void GsxMenuNavigator::RequestDeboarding()
{
    stairsKeptInPlace_ = false;
    TriggerService(kDeboardingServiceId);
}

void GsxMenuNavigator::RequestPushback()
{
    TriggerService(gsx::services::Id(GroundService::Departure));
}

void GsxMenuNavigator::RequestDepartureClearance()
{
    OpenIntent(Intent::Service);
    SyncGsxToolbar();

    const char* const departure = gsx::services::Id(GroundService::Departure);
    ArmRequest(kServiceTriggerVerb,
               QJsonObject{{"service", QString::fromLatin1(departure)}},
               departure,
               {});
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

bool GsxMenuNavigator::PickNowOrArm(const char* const entry, TimedIntent& intent)
{
    const PassScope pass(*this);

    if (state_->menu.shown && PickByContains(entry))
    {
        intent = {};

        return true;
    }

    if (sendRefused_)
    {
        return false;
    }

    intent = {.active = true, .sinceMs = nowMs_()};

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
    completingRefuel_ = {.active = true, .sinceMs = nowMs_()};

    OpenIntent(Intent::Service);
    OpenMenu();
}

void GsxMenuNavigator::CompleteBoarding()
{
    completingBoarding_ = {.active = true, .sinceMs = nowMs_()};

    OpenIntent(Intent::Service);
    OpenMenu();
}

void GsxMenuNavigator::DisableGsxMenu()
{
    const PassScope pass(*this);

    if (!Send(kMenuCloseVerb))
    {
        return;
    }

    reposition_ = Reposition::Idle;
    CloseIntent();
}

void GsxMenuNavigator::Reset()
{
    reposition_ = Reposition::Idle;
    completingPushback_ = {};
    completingRefuel_ = {};
    completingBoarding_ = {};
    serviceOpenedBy_ = nullptr;
    confirmingEngines_ = {};
    stairsKeptInPlace_ = false;
    deIceYesSpent_ = false;
    intent_ = Intent::None;
    intentSinceMs_ = 0;
    lastPickedSig_.clear();
    lastDiagSig_.clear();
    watchedSig_.reset();
    discardedSig_.clear();
    leftOpenSig_.clear();
    resyncSig_.clear();
    resyncCount_ = 0;
    resyncPending_ = false;
    lastActionMs_ = 0;
    RearmPanelLatches();
    pending_.clear();
}

MemoryBag GsxMenuNavigator::TakeMemory() const
{
    MemoryBag memory;
    memory.PutFlag(kDeIceYesSpentEntry, deIceYesSpent_);
    memory.PutFlag(kStairsKeptInPlaceEntry, stairsKeptInPlace_);
    memory.PutFlag(kPanelOpenSpentEntry, panelOpenSpent_);
    memory.PutFlag(kPanelCloseSpentEntry, panelCloseSpent_);
    memory.PutFlag(kPanelOpenedByUsEntry, panelOpenedByUs_);
    memory.PutText(kPendingRequestsEntry, PendingToText());

    return memory;
}

void GsxMenuNavigator::RestoreMemory(const MemoryBag& memory)
{
    Reset();

    deIceYesSpent_ = memory.Flag(kDeIceYesSpentEntry, false);
    stairsKeptInPlace_ = memory.Flag(kStairsKeptInPlaceEntry, false);
    panelOpenSpent_ = memory.Flag(kPanelOpenSpentEntry, false);
    panelCloseSpent_ = memory.Flag(kPanelCloseSpentEntry, false);
    panelOpenedByUs_ = memory.Flag(kPanelOpenedByUsEntry, false);

    const long long now = nowMs_();
    pending_ = PendingFromText(memory.Text(kPendingRequestsEntry, {}), now);
    std::ranges::for_each(pending_, [now](PendingRequest& request)
    {
        if (!request.WasSent())
        {
            request.armedMs = now;
        }
    });

    if (!pending_.empty())
    {
        OpenIntent(Intent::Service);
    }
}

QJsonObject GsxMenuNavigator::PendingToJson(const PendingRequest& request)
{
    return QJsonObject{{kVerbKey, request.verb},
                       {kArgsKey, request.args},
                       {kLabelKey, QString::fromStdString(request.label)},
                       {kConfirmIdKey, QString::fromStdString(request.confirmId)},
                       {kLastSentKey, request.lastSentMs},
                       {kArmedKey, request.armedMs},
                       {kAttemptsKey, request.attempts},
                       {kFiresWhileUnderwayKey, request.firesWhileUnderway}};
}

std::optional<GsxMenuNavigator::PendingRequest> GsxMenuNavigator::PendingFromJson(const QJsonObject& object,
                                                                                  const long long now)
{
    QString verb = object.value(kVerbKey).toString();
    if (!IsArmedVerb(verb))
    {
        return std::nullopt;
    }

    const long long newest = std::max(now, 0LL);

    return PendingRequest{.verb = std::move(verb),
                          .args = object.value(kArgsKey).toObject(),
                          .label = object.value(kLabelKey).toString().toStdString(),
                          .confirmId = object.value(kConfirmIdKey).toString().toStdString(),
                          .lastSentMs = std::clamp(object.value(kLastSentKey).toInteger(), 0LL, newest),
                          .armedMs = std::clamp(object.value(kArmedKey).toInteger(), 0LL, newest),
                          .attempts = std::clamp(object.value(kAttemptsKey).toInt(), 0, kMaxTriggerAttempts),
                          .firesWhileUnderway = object.value(kFiresWhileUnderwayKey).toBool()};
}

std::string GsxMenuNavigator::PendingToText() const
{
    QJsonArray queue;
    std::ranges::for_each(pending_, [&queue](const PendingRequest& request)
    {
        queue.append(PendingToJson(request));
    });

    const QByteArray text = QJsonDocument(queue).toJson(QJsonDocument::Compact);

    return text.toStdString();
}

std::vector<GsxMenuNavigator::PendingRequest> GsxMenuNavigator::PendingFromText(const std::string& text,
                                                                                const long long now)
{
    const QJsonDocument document = QJsonDocument::fromJson(QByteArray::fromStdString(text));

    std::vector<PendingRequest> queue;
    for (const QJsonValue& value : document.array())
    {
        auto request = PendingFromJson(value.toObject(), now);
        if (!request)
        {
            continue;
        }

        const bool alreadyQueued = std::ranges::any_of(queue, [&request](const PendingRequest& queued)
        {
            return queued.Matches(request->verb, request->label);
        });
        if (!alreadyQueued)
        {
            queue.push_back(std::move(*request));
        }
    }

    return queue;
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
    const PassScope pass(*this);

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

    if (sig == lastPickedSig_)
    {
        return;
    }

    if (HandleAutoPicks())
    {
        serviceOpenedBy_ = nullptr;

        return;
    }

    if (sendRefused_ || HandlePendingCompletions())
    {
        return;
    }

    if (!HasActiveIntent())
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

void GsxMenuNavigator::ExpireIntent(TimedIntent& intent, const char* const name) const
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
    watchedSig_.reset();
    leftOpenSig_.clear();
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
        if (!joined.empty())
        {
            joined += " | ";
        }
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

    const bool automationInterested = settings_->autoSelectGsxChoice || settings_->autoDeice
        || HasActiveIntent();
    if (!automationInterested)
    {
        return;
    }

    if (resyncCount_ >= kMaxResyncs)
    {
        DiscardStuckMenu(sig);

        return;
    }

    watchedSinceMs_ = nowMs_();
    if (!Send(kStateGetVerb))
    {
        return;
    }

    ++resyncCount_;
    resyncPending_ = true;
    resyncSig_ = sig;
    lastActionMs_ = nowMs_();
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

    if (!HasActiveIntent())
    {
        LogMenuLeftOpen(sig);

        return;
    }

    watchedSinceMs_ = nowMs_();
    if (!Send(kMenuCloseVerb))
    {
        return;
    }

    discardedSig_ = sig;
    lastActionMs_ = nowMs_();
    logger_->LogInfo(std::format("RemoteAPI closing the menu the resyncs could not move: '{}'",
                                 state_->menu.title));
}

void GsxMenuNavigator::LogMenuLeftOpen(const std::string& sig)
{
    if (sig == leftOpenSig_)
    {
        return;
    }

    leftOpenSig_ = sig;
    logger_->LogInfo(std::format("RemoteAPI leaving the menu open: the client asked for nothing on it: '{}'",
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
    if (Send(kMenuCloseVerb))
    {
        lastActionMs_ = nowMs_();
        logger_->LogInfo(std::format("RemoteAPI closing stale menu '{}'", state_->menu.title));
    }

    return true;
}

bool GsxMenuNavigator::HandleAutoPicks()
{
    const auto& menu = state_->menu;

    if (settings_->autoDeice && !deIceYesSpent_
        && Contains(menu.title, kDeIceQuestion)
        && PickByContains("Yes"))
    {
        deIceYesSpent_ = true;

        return true;
    }

    if (Contains(menu.title, kAirstairsQuestion))
    {
        const bool ownStairs = settings_->useAircraftStairs;
        if (ownStairs && PickByContains(kJetwayAnswerText))
        {
            return true;
        }

        if (PickByPrefix(ownStairs ? "Yes" : "No"))
        {
            return true;
        }
    }

    if (Contains(menu.title, kBlockedSpotText) && Contains(menu.title, kRemoveStairsText))
    {
        const bool passengersNeedThem = PassengersAreFlowing();
        if (PickByPrefix(passengersNeedThem ? "No" : "Yes"))
        {
            if (passengersNeedThem)
            {
                stairsKeptInPlace_ = true;
                logger_->LogInfo("RemoteAPI keeping the stairs: boarding or deboarding is underway");
            }

            return true;
        }
    }

    if (settings_->autoSelectGsxChoice
        && !Contains(menu.title, kConfirmationQuestion)
        && (PickByContains(kGsxChoiceText) || PickByContains(kBlockFuelText)))
    {
        return true;
    }

    if (Contains(menu.title, kDeboardCrewQuestion))
    {
        return PickByContains(CrewChoiceEntry(settings_->crewDeboarding));
    }

    if (Contains(menu.title, kBoardCrewQuestion))
    {
        return PickByContains(CrewChoiceEntry(settings_->crewBoarding));
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

    if (!completingRefuel_.active && !completingBoarding_.active)
    {
        return false;
    }

    if (Contains(state_->menu.title, kServiceInProgressTitle))
    {
        return CompleteTheServiceItOpened();
    }

    serviceOpenedBy_ = nullptr;

    if (completingRefuel_.active && PickByContains(kRefuelingEntryText))
    {
        serviceOpenedBy_ = &completingRefuel_;

        return true;
    }

    if (completingBoarding_.active
        && (PickByContains(kLoadingInProgressText) || PickByContains(kBoardingPassengersText)))
    {
        serviceOpenedBy_ = &completingBoarding_;

        return true;
    }

    return false;
}

bool GsxMenuNavigator::CompleteTheServiceItOpened()
{
    TimedIntent* const opener = serviceOpenedBy_;
    if (opener == nullptr || !opener->active)
    {
        serviceOpenedBy_ = nullptr;

        return true;
    }

    if (PickByContains(kCompleteNowText))
    {
        *opener = {};
        serviceOpenedBy_ = nullptr;
    }

    return true;
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

    if (sendRefused_)
    {
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

void GsxMenuNavigator::TriggerService(const char* const serviceId, const bool firesWhileUnderway)
{
    OpenIntent(Intent::Service);
    SyncGsxToolbar();

    ArmRequest(kServiceTriggerVerb,
               QJsonObject{{"service", QString::fromLatin1(serviceId)}},
               serviceId,
               serviceId,
               firesWhileUnderway);
}

void GsxMenuNavigator::ArmRequest(QString verb, QJsonObject args, std::string label, std::string confirmId,
                                  const bool firesWhileUnderway)
{
    const PassScope pass(*this);

    const auto same = std::ranges::find_if(pending_, [&](const PendingRequest& request)
    {
        return request.Matches(verb, label);
    });

    PendingRequest request{.verb = std::move(verb),
                           .args = std::move(args),
                           .label = std::move(label),
                           .confirmId = std::move(confirmId),
                           .armedMs = nowMs_(),
                           .firesWhileUnderway = firesWhileUnderway};

    if (same != pending_.end())
    {
        request.lastSentMs = same->lastSentMs;
        request.attempts = same->attempts;
        pending_.erase(same);
    }

    pending_.push_back(std::move(request));

    PumpRequests();
}

GsxMenuNavigator::DropReason GsxMenuNavigator::DropReasonFor(const PendingRequest& request, const long long now) const
{
    if (WasTaken(request))
    {
        return DropReason::Taken;
    }

    if (IsAlreadyUnderway(request))
    {
        return DropReason::AlreadyUnderway;
    }

    if (request.WasSent() && request.UndoneByASecondSend() && request.GiveUpWindowElapsed(now))
    {
        return DropReason::ToggleGaveUp;
    }

    if (request.attempts >= kMaxTriggerAttempts && request.RetryWindowElapsed(now))
    {
        return DropReason::NeverTaken;
    }

    if (request.LeftUnsentForTooLong(now))
    {
        return DropReason::NeverLeft;
    }

    return DropReason::None;
}

void GsxMenuNavigator::LogDrop(const PendingRequest& request, const DropReason reason) const
{
    switch (reason)
    {
    case DropReason::None:
    case DropReason::Taken:
        break;
    case DropReason::AlreadyUnderway:
        logger_->LogInfo(std::format("RemoteAPI '{}' already underway; not requesting", request.label));
        break;
    case DropReason::ToggleGaveUp:
        logger_->LogInfo(std::format(
            "RemoteAPI stopped waiting for '{}'; a service that toggles is never sent twice, so the request is dropped",
            request.label));
        break;
    case DropReason::NeverTaken:
        logger_->LogInfo(std::format("RemoteAPI '{}' never taken by GSX after {} attempts",
                                     request.label, request.attempts));
        break;
    case DropReason::NeverLeft:
        logger_->LogInfo(std::format(
            "RemoteAPI stopped waiting for '{}'; the request never left the client, so it is dropped",
            request.label));
        break;
    }
}

void GsxMenuNavigator::PumpRequests()
{
    const long long now = nowMs_();

    std::erase_if(pending_, [this, now](const PendingRequest& request)
    {
        const DropReason reason = DropReasonFor(request, now);
        LogDrop(request, reason);

        return reason != DropReason::None;
    });

    if (IsWaitingForThePanel() || !IsMenuSettled())
    {
        return;
    }

    const auto due = std::ranges::find_if(pending_, [now](const PendingRequest& request)
    {
        return request.IsDueToSend(now);
    });
    if (due != pending_.end())
    {
        SendRequest(*due);
    }
}

void GsxMenuNavigator::SendRequest(PendingRequest& request)
{
    const bool closedMenu = state_->menu.shown;
    if (closedMenu)
    {
        if (!Send(kMenuCloseVerb))
        {
            return;
        }

        ClearMenuTracking();
    }

    if (!Send(request.verb, request.args))
    {
        return;
    }

    lastActionMs_ = nowMs_();
    request.lastSentMs = lastActionMs_;
    ++request.attempts;

    const bool isTheFirstServiceDelivery = request.attempts == 1 && request.verb == kServiceTriggerVerb;
    if (isTheFirstServiceDelivery && !RepositionWalking())
    {
        OpenIntent(Intent::Service);
    }

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
}

bool GsxMenuNavigator::IsAlreadyUnderway(const PendingRequest& request) const
{
    if (request.firesWhileUnderway || request.WasSent() || request.confirmId.empty())
    {
        return false;
    }

    return IsServiceUnderway(request.confirmId);
}

bool GsxMenuNavigator::IsServiceUnderway(const std::string& serviceId) const
{
    const GsxRemoteService* const service = FindService(*state_, serviceId);
    if (service == nullptr)
    {
        return false;
    }

    return service->stateRaw == static_cast<int>(GsxStateStatus::Requested)
        || service->stateRaw == static_cast<int>(GsxStateStatus::Active);
}

bool GsxMenuNavigator::PassengersAreFlowing() const
{
    return std::ranges::any_of(std::array{kBoardingServiceId, kDeboardingServiceId},
                               [this](const char* const serviceId)
                               {
                                   return IsServiceUnderway(serviceId)
                                       || std::ranges::any_of(pending_, [serviceId](const PendingRequest& request)
                                       {
                                           return request.label == serviceId && request.WasSent();
                                       });
                               });
}

bool GsxMenuNavigator::WasTaken(const PendingRequest& request) const
{
    if (!request.WasSent())
    {
        return false;
    }

    if (request.confirmId.empty())
    {
        return true;
    }

    const GsxRemoteService* const service = FindService(*state_, request.confirmId);
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
    if (pluginClient_ == nullptr || PanelMode() != GsxPanelMode::OnPushback || panelOpenSpent_
        || !pluginClient_->IsBridgeReady())
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

void GsxMenuNavigator::CloseThePanelWeOpened(const char* const logLine)
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
    logger_->LogInfo(logLine);
}

void GsxMenuNavigator::ClosePushbackPanel()
{
    CloseThePanelWeOpened("RemoteAPI closing the GSX toolbar the client opened for the pushback");
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
    stairsKeptInPlace_ = false;
    deIceYesSpent_ = false;
    RearmPanelLatches();
}

bool GsxMenuNavigator::WereStairsKeptInPlace() const
{
    return stairsKeptInPlace_;
}

void GsxMenuNavigator::OnPushbackStarted()
{
    CloseThePanelWeOpened("RemoteAPI closing the GSX toolbar now that the pushback has started");
}

void GsxMenuNavigator::OpenMenu() const
{
    const PassScope pass(*this);

    SyncGsxToolbar();

    if (!state_->menu.shown && Send(kMenuToggleVerb))
    {
        lastActionMs_ = nowMs_();
    }
}

bool GsxMenuNavigator::Send(const QString& verb, const QJsonObject& args) const
{
    if (sendRefused_)
    {
        return false;
    }

    sendRefused_ = !client_->SendCommand(verb, args);

    return !sendRefused_;
}

bool GsxMenuNavigator::IsMenuSettled() const
{
    return !resyncPending_ && (nowMs_() - lastActionMs_) >= kMenuSettleMs;
}

bool GsxMenuNavigator::PickFirstMatching(const std::function<bool(const std::string&)>& matches)
{
    const auto& entries = state_->menu.entries;
    const auto& disabled = state_->menu.disabled;
    for (std::size_t i = 0; i < entries.size(); ++i)
    {
        const bool isDisabled = i < disabled.size() && disabled[i];
        if (isDisabled || !matches(entries[i]))
        {
            continue;
        }

        if (!Send(kMenuPickVerb, QJsonObject{{"index", static_cast<int>(i)}}))
        {
            return false;
        }

        lastActionMs_ = nowMs_();
        logger_->LogInfo(std::format("RemoteAPI menu.pick {} ({})", i, entries[i]));
        lastPickedSig_ = MenuSignature();

        return true;
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
