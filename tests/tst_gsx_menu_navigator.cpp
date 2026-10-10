#include <algorithm>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include <QByteArray>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <QtTest/QTest>

#include "../src/domain/model/AutomationSettings.h"
#include "../src/infrastructure/commbus/CommBusPluginClient.h"
#include "../src/infrastructure/gsx/GsxRemoteApiClient.h"
#include "../src/infrastructure/gsx/GsxRemoteState.h"
#include "../src/infrastructure/gsx/GsxMenuNavigator.h"
#include "doubles/FakeCommBusBridgeGateway.h"
#include "doubles/FakeDomainLogger.h"
#include "doubles/FakeVariableGateway.h"

namespace
{
    struct Sent
    {
        QString verb;
        QJsonObject args;
    };

    class FakeRemoteClient final : public GsxRemoteApiClient
    {
    public:
        std::vector<Sent> sent;
        std::vector<Sent> refused;
        bool refusing = false;

        bool SendCommand(const QString& verb, const QJsonObject& args = {}) override
        {
            if (refusing)
            {
                refused.push_back({.verb = verb, .args = args});

                return false;
            }

            sent.push_back({.verb = verb, .args = args});

            return true;
        }

        [[nodiscard]] int Refused(const QString& verb) const
        {
            return static_cast<int>(std::ranges::count_if(refused, [&verb](const Sent& request)
            {
                return request.verb == verb;
            }));
        }

        void EmitRejection(const QString& code)
        {
            emit ResultReceived(false, code);
        }

        [[nodiscard]] const Sent* Last(const QString& verb) const
        {
            for (auto it = sent.rbegin(); it != sent.rend(); ++it)
            {
                if (it->verb == verb)
                {
                    return &*it;
                }
            }
            return nullptr;
        }

        [[nodiscard]] int Count(const QString& verb) const
        {
            int count = 0;
            for (const Sent& request : sent)
            {
                if (request.verb == verb)
                {
                    ++count;
                }
            }
            return count;
        }
    };

    void ShowMenu(GsxRemoteState& state, const std::string& title,
                  std::vector<std::string> entries, std::vector<bool> disabled = {})
    {
        state.menu.shown = true;
        state.menu.title = title;
        state.menu.entries = std::move(entries);
        state.menu.disabled = std::move(disabled);
    }

    void OfferService(GsxRemoteState& state, const std::string& id)
    {
        state.services.push_back(GsxRemoteService{.id = id, .stateRaw = 1, .canTrigger = true});
    }

    void MarkServiceTaken(GsxRemoteState& state, const std::string& id)
    {
        for (GsxRemoteService& service : state.services)
        {
            if (service.id == id)
            {
                service.stateRaw = 5;
                service.canTrigger = false;

                return;
            }
        }

        state.services.push_back(GsxRemoteService{.id = id, .stateRaw = 5, .canTrigger = false});
    }

    bool Logged(const FakeDomainLogger& logger, const std::string& needle)
    {
        return std::ranges::any_of(logger.messages, [&needle](const std::string& message)
                                   { return message.find(needle) != std::string::npos; });
    }
}

class GsxMenuNavigatorTest final : public QObject
{
    Q_OBJECT

private slots:
    static void serviceTriggersUseCanonicalVerbs();
    static void allRequestsOpensTheClosedPanelBeforeAMenuAction();
    static void allRequestsDoesNotReopenAnOpenPanel();
    static void neverSendsNoPanelCommandAtAll();
    static void allRequestsOpensThePanelOnEveryRequest();
    static void onPushbackOpensThePanelOnTheWaitAndNotOnTheRequest();
    static void aRequestWaitsForThePanelToConfirmOpen();
    static void aRequestGoesOutWhenThePanelNeverConfirms();
    static void aPanelThatConfirmedIsNeverReportedAsUnconfirmed();
    static void onPushbackClosesThePanelWhenThePushStarts();
    static void aPanelFoundOpenIsNotClosedWhenThePushStarts();
    static void closePushbackPanelClosesThePanelTheClientOpenedOnce();
    static void closePushbackPanelLeavesAPanelTheClientDidNotOpen();
    static void onPushbackIgnoresAPanelLeftOpenOutsideTheWindow();
    static void onPushbackDoesNotReopenThePanelThePilotClosed();
    static void aDroppedOpenKeepsThePushbackOpenOwed();
    static void theTurnaroundTurnRearmsThePanel();
    static void triggerServiceDoesNotOpenClosedMenu();
    static void triggerServiceDoesNotToggleOpenMenu();
    static void confirmGoodEnginesPicksWhenMenuVisible();
    static void confirmGoodEnginesOpensMenuAndDefersPick();
    static void confirmGoodEnginesOpensTheMenuWhenTheConfirmationEntriesAreLeftOver();
    static void completePushbackOpensTheMenuWhenTheInterruptEntriesAreLeftOver();
    static void completePushbackPicksEntryOnInterruptPushbackMenu();
    static void deferredConfirmEnginesPicksEntryOnInterruptPushbackMenu();
    static void completeRefuelPicksCompleteNowViaServiceMenu();
    static void completeRefuelIntentExpiresAfterTtl();
    static void completeRefuelMatchesLbsLoadedEntry();
    static void completeRefuelMatchesTheDefueledEntry();
    static void completeBoardingPicksCompleteNowViaCargoEntry();
    static void completeBoardingMatchesThePassengerEntry();
    static void completeBoardingDoesNotRepeatThePickWhenASnapshotRepeatsTheMenu();
    static void completeRefuelLeavesAServiceInProgressItDidNotOpenToThePilot();
    static void completeBoardingLeavesAServiceInProgressItDidNotOpenToThePilot();
    static void eachCompletionOnlyCompletesTheServiceItOpened();
    static void completePushbackPicksEntryWithoutInterruptTitle();
    static void staleRepositionClearedByServiceIntent();
    static void picksGsxChoiceDuringServiceIntent();
    static void gsxChoiceSurvivesDispatchDelay();
    static void gsxChoiceSurvivesTransientMenuClose();
    static void resolverDoesNotRepickSameMenu();
    static void gsxChoicePickedEvenAfterIntentTtl();
    static void gsxChoiceNotPickedWhenFlagOff();
    static void boardCrewMenuPicksBothByDefault();
    static void boardCrewMenuPicksConfiguredChoice();
    static void crewMenusPickDeclineOnBothVariantsWhenNobodyConfigured();
    static void crewMenuPickedWithoutActiveIntent();
    static void deboardCrewMenuFollowsItsOwnChoice();
    static void boardCrewMenuIgnoresTheDeboardChoice();
    static void airstairsMenuPicksAirportStairsByDefault();
    static void airstairsMenuPicksAirplaneStairsWhenEnabled();
    static void airstairsMenuYieldsToTheJetwayEvenWithOwnStairsEnabled();
    static void airstairsMenuTakesTheJetwayWhenOwnStairsAreOff();
    static void theBlockedFuelTruckGetsTheSpotYielded();
    static void theBlockedCargoLoaderGetsTheSpotYieldedToo();
    static void aStairsMenuWithoutTheBlockedSpotIsLeftAlone();
    static void theStairsAreKeptWhilePassengersAreBoarding();
    static void theStairsAreKeptWhilePassengersAreDeboarding();
    static void theStairsAreKeptWhileTheBoardingRequestIsStillOutstanding();
    static void theKeptStairsAreRememberedUntilTheNextBoardingRequest();
    static void theRemovedStairsAreNotRemembered();
    static void theTurnaroundTurnForgetsTheKeptStairs();
    static void theDepartureClearanceIsAskedForOnce();
    static void deIceMenuPicksYesWhenEnabled();
    static void deIceMenuDeclinedByDefault();
    static void theDeIceIsAcceptedOncePerDepartureWhenTheQuestionComesBack();
    static void theTurnaroundTurnRearmsTheDeIce();
    static void resetRearmsTheDeIce();
    static void picksSimbriefBlockFuelOnRefuelingLevelMenu();
    static void blockFuelNotPickedWhenFlagOff();
    static void manualMenuWithGsxChoiceIsPicked();
    static void manualMenuIsNotRepickedWhileUnchanged();
    static void manualMenuWithoutGsxChoiceIsIgnored();
    static void theGsxChoiceNeverAnswersThePilotsPullConfirmation();
    static void skipsDisabledEntryAndPicksEnabled();
    static void repositionWalksRootThenSubmenu();
    static void repositionSurvivesTransientCloseAndRootReshow();
    static void staleSelectPositionMenuClosedAfterReposition();
    static void staleSelectPositionMenuClosedAfterServiceIntentReplacedReposition();
    static void staleSelectPositionMenuIgnoredAfterIntentTtl();
    static void rejectedPickAllowsRepick();
    static void resetAllowsRepickingSameMenu();
    static void staleRefuelingLevelMenuResyncsAndPicksBlockFuel();
    static void swallowedRepositionPickRetriesAfterResyncSnapshot();
    static void stalledMenuResyncIsBounded();
    static void lateResyncSnapshotDoesNotRepickAdvancedMenu();
    static void groundServiceTriggersUseCanonicalVerbs();
    static void menuSettlesAfterQuietPeriod();
    static void pendingResyncKeepsMenuUnsettled();
    static void triggerWaitsForTheMenuToSettle();
    static void triggerRetriesWhileGsxStillOffersTheService();
    static void triggerWaitsOutASlowConfirmationInsteadOfFiringAgain();
    static void triggerStopsRetryingOnceGsxTakesIt();
    static void rearmedTriggerKeepsItsAttemptCount();
    static void rearmedTriggerIsDroppedOnceGsxTakesIt();
    static void stuckMenuIsClosedAfterResyncsAreExhausted();
    static void thePushbackDirectionMenuIsNeverDiscarded();
    static void aMenuReopenedWithAnEmptyTitleDoesNotInheritTheSpentResyncs();
    static void aMenuTheClientAskedNothingOfIsNeverDiscarded();
    static void aMenuLeftOpenForThePilotIsStillDiscardedOnceTheClientWaitsOnIt();
    static void theStuckMenuIsDiscardedEvenWithARequestStillPending();
    static void theLastAttemptGetsTheSameGraceAsTheOthers();
    static void resetAllowsClosingTheSameStuckMenuAgain();
    static void aRequestForAServiceAlreadyUnderwayIsDroppedBeforeSending();
    static void theGpuToggleStillFiresWhileTheServiceRuns();
    static void aServiceThatTogglesIsNeverSentTwice();
    static void theStairsAreNeverAskedForASecondTime();
    static void aServiceThatTogglesAndWasSentIsNotSentAgainAfterTheRestore_data();
    static void aServiceThatTogglesAndWasSentIsNotSentAgainAfterTheRestore();
    static void aRestoredGpuRequestStillFiresWhileTheServiceRuns();
    static void aDepartureClearanceNeverSentGoesOutAfterTheRestore();
    static void restoringSendsNothingAndOpensNoToolbar();
    static void aStampInTheFutureIsClampedSoTheRequestRetriesOnSchedule();
    static void aRestoredRequestIsHandledWithAnIntent();
    static void theMemoryTakenRestoredAndTakenAgainIsEqual();
    static void theToolbarTheClientOpenedIsClosedByTheRestoredNavigator();
    static void anEmptyBagRestoresToTheStateOfReset_data();
    static void anEmptyBagRestoresToTheStateOfReset();
    static void malformedQueueTextRestoresAnEmptyQueue_data();
    static void malformedQueueTextRestoresAnEmptyQueue();
    static void aCorruptQueueIsHardenedOnRestore_data();
    static void aCorruptQueueIsHardenedOnRestore();
    static void thePersistedQueueNamesWhatEachFieldMeans();
    static void thePanelLatchesSurviveTheRestore();
    static void aWaitForThePanelDoesNotSurviveTheRestore();
    static void theLabelOfARestoredRequestIsKept();
    static void anUnknownNameInTheBagIsIgnored();
    static void aRestoredRequestNeverSentRestartsItsGiveUpClock();
    static void aRefusedSendOfAServiceThatTogglesIsNotAnAttempt();
    static void aRefusedSendOfARetriedServiceIsNotAnAttempt();
    static void aRefusedSendOfARequestWithoutAServiceIsNotTakenAsSent_data();
    static void aRefusedSendOfARequestWithoutAServiceIsNotTakenAsSent();
    static void aRefusedPickDoesNotSpendTheDeIceAnswer();
    static void aRefusedPickIsNotReportedAsDone();
    static void aRefusedResyncDoesNotLeaveTheMenuUnsettled();
    static void aRefusedResyncIsNotCounted();
    static void aRefusedSendEndsThePass();
    static void aRefusedPickDoesNotFallThroughToAnotherAnswer_data();
    static void aRefusedPickDoesNotFallThroughToAnotherAnswer();
    static void aRefusedPickIsNotLoggedAsAMenuNobodyMatched_data();
    static void aRefusedPickIsNotLoggedAsAMenuNobodyMatched();
    static void aRefusedConfirmEnginesDoesNotKeepItsIntentAlive();
    static void aRequestNeverSentIsDroppedAfterTheGiveUpWindow_data();
    static void aRequestNeverSentIsDroppedAfterTheGiveUpWindow();
    static void aRequestDeliveredLateStillOpensTheServiceIntent();
    static void aRefusedCloseKeepsTheMenuTracking();
    static void aRefusedCloseOfAStuckMenuIsNotRecordedAsDone();
    static void aRefusedCloseOfAStaleMenuDoesNotSpendTheSettleWindow();
    static void aRefusedToggleDoesNotSpendTheSettleWindow();
    static void aRefusedDisableGsxMenuKeepsTheIntent();
    static void theToolbarIsNotOpenedBeforeTheBridgeReportsItsState();
    static void theToolbarIsNotClosedBeforeTheBridgeReportsItsState();
};

void GsxMenuNavigatorTest::serviceTriggersUseCanonicalVerbs()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    long long fakeNow = 0;
    nav.SetClockForTest([&fakeNow] { return fakeNow; });

    const auto settle = [&fakeNow, &nav]
    {
        fakeNow += 1500;
        nav.OnMenuChanged();
    };

    nav.RequestRefueling();
    settle();
    MarkServiceTaken(state, "Refueling");

    nav.RequestBoarding();
    settle();
    MarkServiceTaken(state, "Boarding");

    nav.RequestDeboarding();
    settle();
    MarkServiceTaken(state, "Deboarding");

    nav.RequestPushback();
    settle();
    MarkServiceTaken(state, "Departure");

    nav.CallJetway();
    settle();
    MarkServiceTaken(state, "OperateJetways");

    nav.CallStairs();
    settle();
    MarkServiceTaken(state, "OperateStairs");

    nav.RequestSimbriefLoad();
    settle();

    std::vector<QString> services;
    QString simbriefCommand;
    for (const Sent& request : client.sent)
    {
        if (request.verb == "service.trigger")
        {
            services.push_back(request.args.value("service").toString());
        }
        else if (request.verb == "command.run")
        {
            simbriefCommand = request.args.value("command").toString();
        }
    }

    const std::vector<QString> expected = {
        "Refueling", "Boarding", "Deboarding", "Departure", "OperateJetways", "OperateStairs"
    };

    QCOMPARE(services.size(), expected.size());

    for (std::size_t i = 0; i < expected.size(); ++i)
    {
        QCOMPARE(services[i], expected[i]);
    }

    QCOMPARE(simbriefCommand, QString("RELOAD_SIMBRIEF"));
}

namespace
{
    struct PanelRig
    {
        FakeRemoteClient client;
        GsxRemoteState state;
        AutomationSettings settings;
        FakeDomainLogger logger;
        FakeCommBusBridgeGateway bridge;
        CommBusPluginClient plugin{&bridge};

        explicit PanelRig(const GsxPanelMode mode)
        {
            settings.gsxPanelMode = mode;
            plugin.Setup();
        }

        void PanelIs(const char* panelState)
        {
            bridge.Deliver(IntegratorPluginCommBus::kToolbarStateChannel, panelState);
        }

        [[nodiscard]] int PanelCommands() const
        {
            return bridge.CallCount(IntegratorPluginCommBus::kToolbarCommandChannel);
        }

        [[nodiscard]] QString LastPanelPayload() const
        {
            for (auto it = bridge.calls.rbegin(); it != bridge.calls.rend(); ++it)
            {
                if (std::get<0>(*it) == IntegratorPluginCommBus::kToolbarCommandChannel)
                {
                    return QString::fromStdString(std::get<2>(*it));
                }
            }

            return {};
        }
    };
}

void GsxMenuNavigatorTest::allRequestsOpensTheClosedPanelBeforeAMenuAction()
{
    PanelRig rig(GsxPanelMode::AllRequests);
    GsxMenuNavigator nav(&rig.client, &rig.state, &rig.settings, &rig.logger, &rig.plugin);

    nav.RequestRefueling();

    QCOMPARE(rig.PanelCommands(), 1);
    QCOMPARE(rig.LastPanelPayload(), QString(IntegratorPluginCommBus::kCommandOpen));
    QCOMPARE(rig.client.Count("service.trigger"), 1);
}

void GsxMenuNavigatorTest::allRequestsDoesNotReopenAnOpenPanel()
{
    PanelRig rig(GsxPanelMode::AllRequests);
    rig.PanelIs("open");
    GsxMenuNavigator nav(&rig.client, &rig.state, &rig.settings, &rig.logger, &rig.plugin);

    nav.RequestRefueling();

    QCOMPARE(rig.PanelCommands(), 0);
    QCOMPARE(rig.client.Count("service.trigger"), 1);
}

void GsxMenuNavigatorTest::neverSendsNoPanelCommandAtAll()
{
    PanelRig rig(GsxPanelMode::Never);
    rig.PanelIs("open");
    GsxMenuNavigator nav(&rig.client, &rig.state, &rig.settings, &rig.logger, &rig.plugin);

    nav.RequestRefueling();
    nav.RequestPushback();
    nav.OnPushbackStarted();
    nav.OnTurnaroundTurned();
    nav.RequestBoarding();

    QCOMPARE(rig.PanelCommands(), 0);
}

void GsxMenuNavigatorTest::allRequestsOpensThePanelOnEveryRequest()
{
    PanelRig rig(GsxPanelMode::AllRequests);
    rig.PanelIs("closed");
    GsxMenuNavigator nav(&rig.client, &rig.state, &rig.settings, &rig.logger, &rig.plugin);

    nav.RequestRefueling();
    nav.RequestBoarding();

    QCOMPARE(rig.PanelCommands(), 2);
    QCOMPARE(rig.LastPanelPayload(), QString(IntegratorPluginCommBus::kCommandOpen));
}

void GsxMenuNavigatorTest::onPushbackOpensThePanelOnTheWaitAndNotOnTheRequest()
{
    PanelRig rig(GsxPanelMode::OnPushback);
    rig.PanelIs("closed");
    GsxMenuNavigator nav(&rig.client, &rig.state, &rig.settings, &rig.logger, &rig.plugin);

    nav.RequestRefueling();

    QCOMPARE(rig.PanelCommands(), 0);

    nav.RequestPushback();

    QCOMPARE(rig.PanelCommands(), 0);

    nav.OpenPushbackPanel();

    QCOMPARE(rig.PanelCommands(), 1);
    QCOMPARE(rig.LastPanelPayload(), QString(IntegratorPluginCommBus::kCommandOpen));
}

void GsxMenuNavigatorTest::aRequestWaitsForThePanelToConfirmOpen()
{
    PanelRig rig(GsxPanelMode::OnPushback);
    rig.PanelIs("closed");
    GsxMenuNavigator nav(&rig.client, &rig.state, &rig.settings, &rig.logger, &rig.plugin);

    nav.OpenPushbackPanel();
    nav.RequestRefueling();

    QCOMPARE(rig.client.Count("service.trigger"), 0);

    rig.PanelIs("open");
    nav.OnSnapshot();

    QCOMPARE(rig.client.Count("service.trigger"), 1);
}

void GsxMenuNavigatorTest::aRequestGoesOutWhenThePanelNeverConfirms()
{
    PanelRig rig(GsxPanelMode::OnPushback);
    rig.PanelIs("closed");
    GsxMenuNavigator nav(&rig.client, &rig.state, &rig.settings, &rig.logger, &rig.plugin);

    long long now = 1000;
    nav.SetClockForTest([&now] { return now; });

    nav.OpenPushbackPanel();
    nav.RequestRefueling();

    QCOMPARE(rig.client.Count("service.trigger"), 0);

    now += 30000;
    nav.OnSnapshot();

    QCOMPARE(rig.client.Count("service.trigger"), 1);
}

void GsxMenuNavigatorTest::aPanelThatConfirmedIsNeverReportedAsUnconfirmed()
{
    PanelRig rig(GsxPanelMode::OnPushback);
    rig.PanelIs("closed");
    GsxMenuNavigator nav(&rig.client, &rig.state, &rig.settings, &rig.logger, &rig.plugin);

    long long now = 1000;
    nav.SetClockForTest([&now] { return now; });

    nav.OpenPushbackPanel();
    nav.RequestRefueling();

    now += 2000;
    rig.PanelIs("open");
    nav.OnSnapshot();

    QCOMPARE(rig.client.Count("service.trigger"), 1);

    rig.PanelIs("closed");
    now += 30000;
    nav.OnSnapshot();

    QVERIFY(!Logged(rig.logger, "without the GSX toolbar confirming it opened"));
}

void GsxMenuNavigatorTest::onPushbackClosesThePanelWhenThePushStarts()
{
    PanelRig rig(GsxPanelMode::OnPushback);
    rig.PanelIs("closed");
    GsxMenuNavigator nav(&rig.client, &rig.state, &rig.settings, &rig.logger, &rig.plugin);

    nav.RequestPushback();
    nav.OpenPushbackPanel();
    rig.PanelIs("open");
    nav.OnPushbackStarted();

    QCOMPARE(rig.PanelCommands(), 2);
    QCOMPARE(rig.LastPanelPayload(), QString(IntegratorPluginCommBus::kCommandClose));
}

void GsxMenuNavigatorTest::closePushbackPanelClosesThePanelTheClientOpenedOnce()
{
    PanelRig rig(GsxPanelMode::OnPushback);
    rig.PanelIs("closed");
    GsxMenuNavigator nav(&rig.client, &rig.state, &rig.settings, &rig.logger, &rig.plugin);

    nav.OpenPushbackPanel();
    rig.PanelIs("open");

    QCOMPARE(rig.PanelCommands(), 1);

    nav.ClosePushbackPanel();

    QCOMPARE(rig.PanelCommands(), 2);
    QCOMPARE(rig.LastPanelPayload(), QString(IntegratorPluginCommBus::kCommandClose));
    QVERIFY(Logged(rig.logger, "closing the GSX toolbar the client opened for the pushback"));
    QVERIFY(!Logged(rig.logger, "now that the pushback has started"));

    nav.ClosePushbackPanel();
    nav.OnPushbackStarted();

    QCOMPARE(rig.PanelCommands(), 2);
}

void GsxMenuNavigatorTest::closePushbackPanelLeavesAPanelTheClientDidNotOpen()
{
    PanelRig rig(GsxPanelMode::OnPushback);
    rig.PanelIs("open");
    GsxMenuNavigator nav(&rig.client, &rig.state, &rig.settings, &rig.logger, &rig.plugin);

    nav.OpenPushbackPanel();
    nav.ClosePushbackPanel();

    QCOMPARE(rig.PanelCommands(), 0);

    nav.ClosePushbackPanel();

    QCOMPARE(rig.PanelCommands(), 0);
}

void GsxMenuNavigatorTest::aPanelFoundOpenIsNotClosedWhenThePushStarts()
{
    PanelRig rig(GsxPanelMode::OnPushback);
    rig.PanelIs("open");
    GsxMenuNavigator nav(&rig.client, &rig.state, &rig.settings, &rig.logger, &rig.plugin);

    nav.RequestPushback();

    QCOMPARE(rig.PanelCommands(), 0);
    QCOMPARE(rig.client.Count("service.trigger"), 1);

    nav.OnPushbackStarted();

    QCOMPARE(rig.PanelCommands(), 0);
}

void GsxMenuNavigatorTest::onPushbackIgnoresAPanelLeftOpenOutsideTheWindow()
{
    PanelRig rig(GsxPanelMode::OnPushback);
    rig.PanelIs("open");
    GsxMenuNavigator nav(&rig.client, &rig.state, &rig.settings, &rig.logger, &rig.plugin);

    nav.RequestRefueling();
    nav.RequestBoarding();

    QCOMPARE(rig.PanelCommands(), 0);
}

void GsxMenuNavigatorTest::onPushbackDoesNotReopenThePanelThePilotClosed()
{
    PanelRig rig(GsxPanelMode::OnPushback);
    rig.PanelIs("closed");
    GsxMenuNavigator nav(&rig.client, &rig.state, &rig.settings, &rig.logger, &rig.plugin);

    nav.RequestPushback();
    nav.OpenPushbackPanel();
    rig.PanelIs("open");
    nav.OnSnapshot();
    rig.PanelIs("closed");
    nav.OpenPushbackPanel();

    QCOMPARE(rig.PanelCommands(), 1);
}

void GsxMenuNavigatorTest::aDroppedOpenKeepsThePushbackOpenOwed()
{
    PanelRig rig(GsxPanelMode::OnPushback);
    rig.PanelIs("closed");
    rig.bridge.available = false;
    GsxMenuNavigator nav(&rig.client, &rig.state, &rig.settings, &rig.logger, &rig.plugin);

    nav.RequestPushback();
    nav.OpenPushbackPanel();

    QCOMPARE(rig.PanelCommands(), 0);
    QCOMPARE(rig.client.Count("service.trigger"), 1);

    rig.bridge.available = true;
    nav.OpenPushbackPanel();

    QCOMPARE(rig.PanelCommands(), 1);
    QCOMPARE(rig.LastPanelPayload(), QString(IntegratorPluginCommBus::kCommandOpen));
}

void GsxMenuNavigatorTest::theTurnaroundTurnRearmsThePanel()
{
    PanelRig rig(GsxPanelMode::OnPushback);
    rig.PanelIs("closed");
    GsxMenuNavigator nav(&rig.client, &rig.state, &rig.settings, &rig.logger, &rig.plugin);

    nav.RequestPushback();
    nav.OpenPushbackPanel();
    rig.PanelIs("open");
    nav.OnPushbackStarted();

    QCOMPARE(rig.PanelCommands(), 2);

    nav.OnTurnaroundTurned();
    rig.PanelIs("closed");
    nav.OpenPushbackPanel();

    QCOMPARE(rig.PanelCommands(), 3);
}

void GsxMenuNavigatorTest::triggerServiceDoesNotOpenClosedMenu()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    nav.RequestRefueling();

    QCOMPARE(client.sent.size(), static_cast<std::size_t>(1));
    QCOMPARE(client.sent[0].verb, QString("service.trigger"));
}

void GsxMenuNavigatorTest::triggerServiceDoesNotToggleOpenMenu()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    ShowMenu(state, "Activate Services at ZZZZ/Test Airport", {"Request Refueling"});

    nav.RequestRefueling();

    QCOMPARE(client.Count("menu.toggle"), 0);
    QCOMPARE(client.Count("menu.close"), 1);
    QCOMPARE(client.Count("service.trigger"), 1);
}

void GsxMenuNavigatorTest::confirmGoodEnginesPicksWhenMenuVisible()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    ShowMenu(state, "Confirm good engine start", {"Confirm good engine start", "Cancel"});

    QVERIFY(nav.ConfirmGoodEngines());

    const Sent* pick = client.Last("menu.pick");

    QVERIFY(pick != nullptr);

    QCOMPARE(pick->args.value("index").toInt(), 0);
    QCOMPARE(client.Count("menu.toggle"), 0);
}

void GsxMenuNavigatorTest::confirmGoodEnginesOpensMenuAndDefersPick()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    QVERIFY(!nav.ConfirmGoodEngines());

    QCOMPARE(client.Count("menu.pick"), 0);
    QCOMPARE(client.Count("menu.toggle"), 1);

    ShowMenu(state, "Confirm good engine start", {"Confirm good engine start", "Cancel"});
    nav.OnMenuChanged();

    const Sent* pick = client.Last("menu.pick");

    QVERIFY(pick != nullptr);

    QCOMPARE(pick->args.value("index").toInt(), 0);
}

void GsxMenuNavigatorTest::confirmGoodEnginesOpensTheMenuWhenTheConfirmationEntriesAreLeftOver()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    ShowMenu(state, "Interrupt pushback?",
             {"Confirm good engine Start", "Stop here and complete pushback procedure", "Abort pushback"});
    state.menu.shown = false;

    QVERIFY(!nav.ConfirmGoodEngines());

    QCOMPARE(client.Count("menu.pick"), 0);
    QCOMPARE(client.Count("menu.toggle"), 1);

    state.menu.shown = true;
    nav.OnMenuChanged();

    const Sent* pick = client.Last("menu.pick");

    QVERIFY(pick != nullptr);

    QCOMPARE(pick->args.value("index").toInt(), 0);
}

void GsxMenuNavigatorTest::completePushbackOpensTheMenuWhenTheInterruptEntriesAreLeftOver()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    ShowMenu(state, "Interrupt pushback?",
             {"Confirm good engine Start", "Stop here and complete pushback procedure", "Abort pushback"});
    state.menu.shown = false;

    QVERIFY(!nav.CompletePushback());

    QCOMPARE(client.Count("menu.pick"), 0);
    QCOMPARE(client.Count("menu.toggle"), 1);

    state.menu.shown = true;
    nav.OnMenuChanged();

    const Sent* pick = client.Last("menu.pick");

    QVERIFY(pick != nullptr);

    QCOMPARE(pick->args.value("index").toInt(), 1);
}

void GsxMenuNavigatorTest::completePushbackPicksEntryOnInterruptPushbackMenu()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    QVERIFY(!nav.CompletePushback());

    QCOMPARE(client.Count("menu.pick"), 0);
    QCOMPARE(client.Count("menu.toggle"), 1);

    ShowMenu(state, "Interrupt pushback?",
             {
                 "Engines not started, call you back later",
                 "Stop here and complete pushback procedure",
                 "Abort pushback",
                 "Cameras"
             });
    nav.OnMenuChanged();

    const Sent* pick = client.Last("menu.pick");

    QVERIFY(pick != nullptr);

    QCOMPARE(pick->args.value("index").toInt(), 1);
}

void GsxMenuNavigatorTest::deferredConfirmEnginesPicksEntryOnInterruptPushbackMenu()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    QVERIFY(!nav.ConfirmGoodEngines());

    QCOMPARE(client.Count("menu.pick"), 0);
    QCOMPARE(client.Count("menu.toggle"), 1);

    ShowMenu(state, "Interrupt pushback?",
             {
                 "Confirm good engine Start",
                 "Stop here and complete pushback procedure",
                 "Abort pushback",
                 "Cameras"
             });
    nav.OnMenuChanged();

    const Sent* pick = client.Last("menu.pick");

    QVERIFY(pick != nullptr);

    QCOMPARE(pick->args.value("index").toInt(), 0);
}

void GsxMenuNavigatorTest::completeRefuelPicksCompleteNowViaServiceMenu()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    nav.CompleteRefuel();

    QCOMPARE(client.Count("menu.toggle"), 1);

    ShowMenu(state, "Activate Services at ZZZZ",
             {"Request Deboarding", "Refueling: 10761 kg loaded", "Request Boarding"});
    nav.OnMenuChanged();

    const Sent* first = client.Last("menu.pick");

    QVERIFY(first != nullptr);
    QCOMPARE(first->args.value("index").toInt(), 1);

    ShowMenu(state, "Service in progress", {"Complete now", "Abort service", "Back"});
    nav.OnMenuChanged();

    const Sent* second = client.Last("menu.pick");

    QVERIFY(second != nullptr);
    QCOMPARE(second->args.value("index").toInt(), 0);
}

void GsxMenuNavigatorTest::completeBoardingPicksCompleteNowViaCargoEntry()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    nav.CompleteBoarding();

    QCOMPARE(client.Count("menu.toggle"), 1);

    ShowMenu(state, "Activate Services at ZZZZ",
             {"Request Deboarding", "Request Refueling", "Cargo loading in progress"});
    nav.OnMenuChanged();

    const Sent* first = client.Last("menu.pick");

    QVERIFY(first != nullptr);
    QCOMPARE(first->args.value("index").toInt(), 2);

    ShowMenu(state, "Service in progress", {"Complete now", "Abort service", "Back"});
    nav.OnMenuChanged();

    const Sent* second = client.Last("menu.pick");

    QVERIFY(second != nullptr);
    QCOMPARE(second->args.value("index").toInt(), 0);
}

void GsxMenuNavigatorTest::completeBoardingMatchesThePassengerEntry()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    nav.CompleteBoarding();

    ShowMenu(state, "Activate Services at ZZZZ",
             {"Request Deboarding", "Boarding passengers now", "Prepare for Push-back and Departure"});
    nav.OnMenuChanged();

    const Sent* pick = client.Last("menu.pick");

    QVERIFY(pick != nullptr);
    QCOMPARE(pick->args.value("index").toInt(), 1);
}

void GsxMenuNavigatorTest::completeRefuelIntentExpiresAfterTtl()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    long long fakeNow = 0;
    nav.SetClockForTest([&fakeNow] { return fakeNow; });

    nav.CompleteRefuel();

    fakeNow = 25000;
    ShowMenu(state, "Service in progress", {"Complete now", "Abort service", "Back"});
    nav.OnMenuChanged();

    QCOMPARE(client.Count("menu.pick"), 0);
}

void GsxMenuNavigatorTest::completeRefuelMatchesLbsLoadedEntry()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    nav.CompleteRefuel();

    ShowMenu(state, "Activate Services at ZZZZ",
             {"Request Deboarding", "Refueling: 23724 lbs loaded", "Request Boarding"});
    nav.OnMenuChanged();

    const Sent* pick = client.Last("menu.pick");

    QVERIFY(pick != nullptr);
    QCOMPARE(pick->args.value("index").toInt(), 1);
}

void GsxMenuNavigatorTest::completeRefuelMatchesTheDefueledEntry()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    nav.CompleteRefuel();

    ShowMenu(state, "Activate Services at ZZZZ",
             {"Request Deboarding", "Request Catering service", "Refueling: 22718 kg defueled",
              "Request Boarding"});
    nav.OnMenuChanged();

    const Sent* pick = client.Last("menu.pick");

    QVERIFY(pick != nullptr);
    QCOMPARE(pick->args.value("index").toInt(), 2);
}

void GsxMenuNavigatorTest::completeBoardingDoesNotRepeatThePickWhenASnapshotRepeatsTheMenu()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    nav.CompleteBoarding();

    ShowMenu(state, "Activate Services at ENSB/Longyear",
             {"Request Deboarding", "Request Catering service", "Request Refueling",
              "Baggage loading in progress", "Prepare for Push-back and Departure"});
    nav.OnMenuChanged();

    QCOMPARE(client.Count("menu.pick"), 1);
    QCOMPARE(client.Last("menu.pick")->args.value("index").toInt(), 3);

    nav.OnSnapshot();
    nav.OnMenuChanged();

    QCOMPARE(client.Count("menu.pick"), 1);

    ShowMenu(state, "Service in progress", {"Complete now", "Abort service", "Back"});
    nav.OnMenuChanged();

    QCOMPARE(client.Count("menu.pick"), 2);
    QCOMPARE(client.Last("menu.pick")->args.value("index").toInt(), 0);
}

void GsxMenuNavigatorTest::completeRefuelLeavesAServiceInProgressItDidNotOpenToThePilot()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    nav.CompleteRefuel();

    ShowMenu(state, "Service in progress", {"Complete now", "Abort service", "Back"});
    nav.OnMenuChanged();

    QCOMPARE(client.Count("menu.pick"), 0);

    ShowMenu(state, "Activate Services at ZZZZ",
             {"Request Deboarding", "Refueling: 10761 kg loaded", "Boarding passengers now"});
    nav.OnMenuChanged();

    QCOMPARE(client.Last("menu.pick")->args.value("index").toInt(), 1);

    ShowMenu(state, "Service in progress", {"Complete now", "Abort service", "Back"});
    nav.OnMenuChanged();

    QCOMPARE(client.Count("menu.pick"), 2);
    QCOMPARE(client.Last("menu.pick")->args.value("index").toInt(), 0);
}

void GsxMenuNavigatorTest::completeBoardingLeavesAServiceInProgressItDidNotOpenToThePilot()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    nav.CompleteBoarding();

    ShowMenu(state, "Activate Services at ZZZZ",
             {"Request Deboarding", "Refueling: 10761 kg loaded", "Prepare for Push-back and Departure"});
    nav.OnMenuChanged();

    QCOMPARE(client.Count("menu.pick"), 0);

    ShowMenu(state, "Service in progress", {"Complete now", "Abort service", "Back"});
    nav.OnMenuChanged();

    QCOMPARE(client.Count("menu.pick"), 0);
}

void GsxMenuNavigatorTest::eachCompletionOnlyCompletesTheServiceItOpened()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    nav.CompleteRefuel();
    nav.CompleteBoarding();

    ShowMenu(state, "Activate Services at ZZZZ",
             {"Request Deboarding", "Refueling: 10761 kg loaded", "Boarding passengers now"});
    nav.OnMenuChanged();

    QCOMPARE(client.Last("menu.pick")->args.value("index").toInt(), 1);

    ShowMenu(state, "Activate Services at ZZZZ",
             {"Request Deboarding", "Request Catering service", "Boarding passengers now"});
    nav.OnMenuChanged();

    QCOMPARE(client.Last("menu.pick")->args.value("index").toInt(), 2);

    ShowMenu(state, "Service in progress", {"Complete now", "Abort service", "Back"});
    nav.OnMenuChanged();

    QCOMPARE(client.Count("menu.pick"), 3);
    QCOMPARE(client.Last("menu.pick")->args.value("index").toInt(), 0);

    state.menu.shown = false;
    nav.OnMenuChanged();

    ShowMenu(state, "Service in progress", {"Complete now", "Abort service", "Back"});
    nav.OnMenuChanged();

    QCOMPARE(client.Count("menu.pick"), 3);
}

void GsxMenuNavigatorTest::completePushbackPicksEntryWithoutInterruptTitle()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    QVERIFY(!nav.CompletePushback());

    ShowMenu(state, "Pushback in progress",
             {"Stop here and complete pushback procedure", "Abort pushback", "Cameras"});
    nav.OnMenuChanged();

    const Sent* pick = client.Last("menu.pick");

    QVERIFY(pick != nullptr);
    QCOMPARE(pick->args.value("index").toInt(), 0);
}

void GsxMenuNavigatorTest::staleRepositionClearedByServiceIntent()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    nav.RepositionAircraft();
    nav.RequestBoarding();

    ShowMenu(state, "Activate Services at ZZZZ",
             {"Request Deboarding", "Request Boarding", "Reposition Aircraft"});
    nav.OnMenuChanged();

    QCOMPARE(client.Count("menu.pick"), 0);
}

void GsxMenuNavigatorTest::picksGsxChoiceDuringServiceIntent()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    nav.RequestRefueling();

    ShowMenu(state, "Select fueltruck operator", {"Operator A [GSX choice]", "Operator B", "Back"});
    nav.OnMenuChanged();

    const Sent* pick = client.Last("menu.pick");

    QVERIFY(pick != nullptr);

    QCOMPARE(pick->args.value("index").toInt(), 0);
}

void GsxMenuNavigatorTest::gsxChoiceSurvivesDispatchDelay()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    long long fakeNow = 0;
    nav.SetClockForTest([&fakeNow] { return fakeNow; });

    nav.RequestRefueling();

    fakeNow = 30000;
    ShowMenu(state, "Select fueltruck operator", {"Operator A [GSX choice]", "Operator B"});
    nav.OnMenuChanged();

    const Sent* pick = client.Last("menu.pick");

    QVERIFY(pick != nullptr);

    QCOMPARE(pick->args.value("index").toInt(), 0);
}

void GsxMenuNavigatorTest::gsxChoiceSurvivesTransientMenuClose()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    nav.RequestRefueling();

    ShowMenu(state, "Activate Services at ZZZZ/Test Airport", {"Request Refueling", "Request Boarding"});
    nav.OnMenuChanged();

    QCOMPARE(client.Count("menu.pick"), 0);

    state.menu.shown = false;
    state.menu.title.clear();
    state.menu.entries.clear();
    nav.OnMenuChanged();

    ShowMenu(state, "Select fueltruck operator", {"Operator A [GSX choice]", "Operator B"});
    nav.OnMenuChanged();

    const Sent* pick = client.Last("menu.pick");

    QVERIFY(pick != nullptr);

    QCOMPARE(pick->args.value("index").toInt(), 0);
}

void GsxMenuNavigatorTest::resolverDoesNotRepickSameMenu()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    nav.RequestRefueling();

    ShowMenu(state, "Select fueltruck operator", {"Operator A [GSX choice]", "Operator B"});

    nav.OnMenuChanged(); // tick 1
    nav.OnMenuChanged(); // tick 2
    nav.OnMenuChanged(); // tick 3

    QCOMPARE(client.Count("menu.pick"), 1);
}

void GsxMenuNavigatorTest::gsxChoicePickedEvenAfterIntentTtl()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    long long fakeNow = 0;
    nav.SetClockForTest([&fakeNow] { return fakeNow; });

    nav.RequestRefueling();

    fakeNow = 120000;
    ShowMenu(state, "Select fueltruck operator", {"Operator A [GSX choice]", "Operator B"});
    nav.OnMenuChanged();

    const Sent* pick = client.Last("menu.pick");

    QVERIFY(pick != nullptr);

    QCOMPARE(pick->args.value("index").toInt(), 0);
}

void GsxMenuNavigatorTest::gsxChoiceNotPickedWhenFlagOff()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    AutomationSettings settings;
    settings.autoSelectGsxChoice = false;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    nav.RequestRefueling();

    ShowMenu(state, "Select fueltruck operator", {"Operator A [GSX choice]", "Operator B"});
    nav.OnMenuChanged();

    QCOMPARE(client.Count("menu.pick"), 0);
}

void GsxMenuNavigatorTest::boardCrewMenuPicksBothByDefault()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    nav.RequestBoarding();

    ShowMenu(state, "Do you want to board crew?", {"Nobody", "Crew", "Pilots", "Both"});
    nav.OnMenuChanged();

    const Sent* pick = client.Last("menu.pick");

    QVERIFY(pick != nullptr);

    QCOMPARE(pick->args.value("index").toInt(), 3);
}

void GsxMenuNavigatorTest::boardCrewMenuPicksConfiguredChoice()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    AutomationSettings settings;
    settings.crewBoarding = CrewChoice::Pilots;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    nav.RequestBoarding();

    ShowMenu(state, "Do you want to board crew?", {"Nobody", "Crew", "Pilots", "Both"});
    nav.OnMenuChanged();

    const Sent* pick = client.Last("menu.pick");

    QVERIFY(pick != nullptr);

    QCOMPARE(pick->args.value("index").toInt(), 2);
}

void GsxMenuNavigatorTest::crewMenusPickDeclineOnBothVariantsWhenNobodyConfigured()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    AutomationSettings settings;
    settings.crewBoarding = CrewChoice::Nobody;
    settings.crewDeboarding = CrewChoice::Nobody;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    nav.RequestBoarding();

    ShowMenu(state, "Do you want to board crew?", {"Nobody", "Crew", "Pilots", "Both"});
    nav.OnMenuChanged();

    const Sent* boardPick = client.Last("menu.pick");

    QVERIFY(boardPick != nullptr);
    QCOMPARE(boardPick->args.value("index").toInt(), 0);

    nav.RequestDeboarding();

    ShowMenu(state, "Do you want to deboard crew?", {"No", "Crew", "Pilots", "Both"});
    nav.OnMenuChanged();

    QCOMPARE(client.Count("menu.pick"), 2);

    const Sent* deboardPick = client.Last("menu.pick");

    QVERIFY(deboardPick != nullptr);
    QCOMPARE(deboardPick->args.value("index").toInt(), 0);
}

void GsxMenuNavigatorTest::crewMenuPickedWithoutActiveIntent()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    AutomationSettings settings;
    settings.crewDeboarding = CrewChoice::Crew;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    long long fakeNow = 0;
    nav.SetClockForTest([&fakeNow] { return fakeNow; });

    nav.RequestDeboarding();

    fakeNow = 90000;
    ShowMenu(state, "Do you want to deboard crew?", {"No", "Crew", "Pilots", "Both"});
    nav.OnMenuChanged();

    const Sent* pick = client.Last("menu.pick");

    QVERIFY(pick != nullptr);
    QCOMPARE(pick->args.value("index").toInt(), 1);
}

void GsxMenuNavigatorTest::deboardCrewMenuFollowsItsOwnChoice()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    AutomationSettings settings;
    settings.crewBoarding = CrewChoice::Nobody;
    settings.crewDeboarding = CrewChoice::Pilots;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    nav.RequestDeboarding();

    ShowMenu(state, "Do you want to deboard crew?", {"No", "Crew", "Pilots", "Both"});
    nav.OnMenuChanged();

    const Sent* pick = client.Last("menu.pick");

    QVERIFY(pick != nullptr);
    QCOMPARE(pick->args.value("index").toInt(), 2);
}

void GsxMenuNavigatorTest::boardCrewMenuIgnoresTheDeboardChoice()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    AutomationSettings settings;
    settings.crewBoarding = CrewChoice::Crew;
    settings.crewDeboarding = CrewChoice::Nobody;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    nav.RequestBoarding();

    ShowMenu(state, "Do you want to board crew?", {"Nobody", "Crew", "Pilots", "Both"});
    nav.OnMenuChanged();

    const Sent* pick = client.Last("menu.pick");

    QVERIFY(pick != nullptr);
    QCOMPARE(pick->args.value("index").toInt(), 1);
}

void GsxMenuNavigatorTest::airstairsMenuPicksAirportStairsByDefault()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    ShowMenu(state, "Use airplane's own airstairs?",
             {"Yes - Use airplane stairs", "No - Use airport stairs", "Both - Airplane stairs + airport stairs"});
    nav.OnMenuChanged();

    const Sent* pick = client.Last("menu.pick");

    QVERIFY(pick != nullptr);

    QCOMPARE(pick->args.value("index").toInt(), 1);
}

void GsxMenuNavigatorTest::airstairsMenuPicksAirplaneStairsWhenEnabled()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    AutomationSettings settings;
    settings.useAircraftStairs = true;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    ShowMenu(state, "Use airplane's own airstairs?",
             {"Yes - Use airplane stairs", "No - Use airport stairs", "Both - Airplane stairs + airport stairs"});
    nav.OnMenuChanged();

    const Sent* pick = client.Last("menu.pick");

    QVERIFY(pick != nullptr);

    QCOMPARE(pick->args.value("index").toInt(), 0);
}

void GsxMenuNavigatorTest::airstairsMenuYieldsToTheJetwayEvenWithOwnStairsEnabled()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    AutomationSettings settings;
    settings.useAircraftStairs = true;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    ShowMenu(state, "Use airplane's own airstairs?",
             {"Yes - Use airplane stairs (no jetway)", "No - Use jetway"});
    nav.OnMenuChanged();

    const Sent* pick = client.Last("menu.pick");

    QVERIFY(pick != nullptr);

    QCOMPARE(pick->args.value("index").toInt(), 1);
}

void GsxMenuNavigatorTest::airstairsMenuTakesTheJetwayWhenOwnStairsAreOff()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    ShowMenu(state, "Use airplane's own airstairs?",
             {"Yes - Use airplane stairs (no jetway)", "No - Use jetway"});
    nav.OnMenuChanged();

    const Sent* pick = client.Last("menu.pick");

    QVERIFY(pick != nullptr);

    QCOMPARE(pick->args.value("index").toInt(), 1);
}

void GsxMenuNavigatorTest::theBlockedFuelTruckGetsTheSpotYielded()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    ShowMenu(state,
             "The fuel truck is waiting for the spot where the stairs at L Entry 5 are parked. Remove the stairs?",
             {"Yes, remove the stairs", "No, keep the stairs"});
    nav.OnMenuChanged();

    const Sent* pick = client.Last("menu.pick");

    QVERIFY(pick != nullptr);

    QCOMPARE(pick->args.value("index").toInt(), 0);
}

void GsxMenuNavigatorTest::theBlockedCargoLoaderGetsTheSpotYieldedToo()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    ShowMenu(state,
             "The front cargo loader is waiting for the spot where the stairs at AFT Pax are parked. Remove the stairs?",
             {"No, keep the stairs", "Yes, remove the stairs"});
    nav.OnMenuChanged();

    const Sent* pick = client.Last("menu.pick");

    QVERIFY(pick != nullptr);

    QCOMPARE(pick->args.value("index").toInt(), 1);
}

void GsxMenuNavigatorTest::aStairsMenuWithoutTheBlockedSpotIsLeftAlone()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    ShowMenu(state,
             "The stairs at L Entry 5 are no longer needed. Remove the stairs?",
             {"Yes, remove the stairs", "No, keep the stairs"});
    nav.OnMenuChanged();

    QVERIFY(client.Last("menu.pick") == nullptr);
}

void GsxMenuNavigatorTest::theStairsAreKeptWhilePassengersAreBoarding()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    MarkServiceTaken(state, "Boarding");
    ShowMenu(state,
             "The rear cargo loader is waiting for the spot where the stairs at L Entry FWD are parked. Remove the stairs?",
             {"Yes, remove the stairs", "No, keep the stairs"});
    nav.OnMenuChanged();

    const Sent* pick = client.Last("menu.pick");

    QVERIFY(pick != nullptr);
    QCOMPARE(pick->args.value("index").toInt(), 1);
    QVERIFY(Logged(logger, "RemoteAPI keeping the stairs: boarding or deboarding is underway"));
    QVERIFY(!Logged(logger, "passengers"));
}

void GsxMenuNavigatorTest::theStairsAreKeptWhilePassengersAreDeboarding()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    state.services.push_back(GsxRemoteService{.id = "Deboarding", .stateRaw = 4, .canTrigger = false});
    ShowMenu(state,
             "The front cargo loader is waiting for the spot where the stairs at AFT Pax are parked. Remove the stairs?",
             {"No, keep the stairs", "Yes, remove the stairs"});
    nav.OnMenuChanged();

    const Sent* pick = client.Last("menu.pick");

    QVERIFY(pick != nullptr);
    QCOMPARE(pick->args.value("index").toInt(), 0);
}

void GsxMenuNavigatorTest::theStairsAreKeptWhileTheBoardingRequestIsStillOutstanding()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    long long fakeNow = 0;
    nav.SetClockForTest([&fakeNow] { return fakeNow; });

    OfferService(state, "Boarding");
    nav.RequestBoarding();
    fakeNow += 1500;
    nav.OnMenuChanged();

    QCOMPARE(client.Count("service.trigger"), 1);

    ShowMenu(state,
             "The rear cargo loader is waiting for the spot where the stairs at L Entry FWD are parked. Remove the stairs?",
             {"Yes, remove the stairs", "No, keep the stairs"});
    nav.OnMenuChanged();

    const Sent* pick = client.Last("menu.pick");

    QVERIFY(pick != nullptr);
    QCOMPARE(pick->args.value("index").toInt(), 1);
}

void GsxMenuNavigatorTest::theKeptStairsAreRememberedUntilTheNextBoardingRequest()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    QVERIFY(!nav.WereStairsKeptInPlace());

    MarkServiceTaken(state, "Boarding");
    ShowMenu(state,
             "The rear cargo loader is waiting for the spot where the stairs at L Entry FWD are parked. Remove the stairs?",
             {"Yes, remove the stairs", "No, keep the stairs"});
    nav.OnMenuChanged();

    QVERIFY(nav.WereStairsKeptInPlace());

    nav.RequestBoarding();

    QVERIFY(!nav.WereStairsKeptInPlace());
}

void GsxMenuNavigatorTest::theRemovedStairsAreNotRemembered()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    ShowMenu(state,
             "The catering vehicle front is waiting for the spot where the stairs at L Entry AFT are parked. Remove the stairs?",
             {"Yes, remove the stairs", "No, keep the stairs"});
    nav.OnMenuChanged();

    const Sent* pick = client.Last("menu.pick");

    QVERIFY(pick != nullptr);
    QCOMPARE(pick->args.value("index").toInt(), 0);
    QVERIFY(!nav.WereStairsKeptInPlace());
}

void GsxMenuNavigatorTest::theTurnaroundTurnForgetsTheKeptStairs()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    MarkServiceTaken(state, "Boarding");
    ShowMenu(state,
             "The rear cargo loader is waiting for the spot where the stairs at L Entry FWD are parked. Remove the stairs?",
             {"Yes, remove the stairs", "No, keep the stairs"});
    nav.OnMenuChanged();

    QVERIFY(nav.WereStairsKeptInPlace());

    nav.OnTurnaroundTurned();

    QVERIFY(!nav.WereStairsKeptInPlace());
}

void GsxMenuNavigatorTest::theDepartureClearanceIsAskedForOnce()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    long long fakeNow = 0;
    nav.SetClockForTest([&fakeNow] { return fakeNow; });

    OfferService(state, "Departure");
    nav.RequestDepartureClearance();
    fakeNow += 1500;
    nav.OnMenuChanged();

    const Sent* trigger = client.Last("service.trigger");

    QVERIFY(trigger != nullptr);
    QCOMPARE(trigger->args.value("service").toString(), QStringLiteral("Departure"));
    QCOMPARE(client.Count("service.trigger"), 1);

    fakeNow += 30000;
    nav.OnMenuChanged();
    fakeNow += 30000;
    nav.OnMenuChanged();

    QCOMPARE(client.Count("service.trigger"), 1);
    QVERIFY(!Logged(logger, "never taken by GSX"));
}

void GsxMenuNavigatorTest::deIceMenuPicksYesWhenEnabled()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    AutomationSettings settings;
    settings.autoDeice = true;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    ShowMenu(state, "Ice warning: do you request the de-icing treatment?", {"Yes", "No [GSX choice]"});
    nav.OnMenuChanged();

    const Sent* pick = client.Last("menu.pick");

    QVERIFY(pick != nullptr);

    QCOMPARE(pick->args.value("index").toInt(), 0);
}

void GsxMenuNavigatorTest::deIceMenuDeclinedByDefault()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    ShowMenu(state, "Ice warning: do you request the de-icing treatment?", {"Yes", "No [GSX choice]"});
    nav.OnMenuChanged();

    const Sent* pick = client.Last("menu.pick");

    QVERIFY(pick != nullptr);

    QCOMPARE(pick->args.value("index").toInt(), 1);
}

namespace
{
    void AskForDeIceAndClose(GsxRemoteState& state, GsxMenuNavigator& nav)
    {
        ShowMenu(state, "Ice warning: do you request the de-icing treatment?", {"Yes", "No [GSX choice]"});
        nav.OnMenuChanged();

        state.menu.shown = false;
        state.menu.title.clear();
        state.menu.entries.clear();
        nav.OnMenuChanged();
    }

    int YesPicks(const FakeRemoteClient& client)
    {
        int yes = 0;
        for (const Sent& request : client.sent)
        {
            if (request.verb == "menu.pick" && request.args.value("index").toInt() == 0)
            {
                ++yes;
            }
        }

        return yes;
    }
}

void GsxMenuNavigatorTest::theDeIceIsAcceptedOncePerDepartureWhenTheQuestionComesBack()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    AutomationSettings settings;
    settings.autoDeice = true;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    AskForDeIceAndClose(state, nav);
    AskForDeIceAndClose(state, nav);

    QCOMPARE(client.Count("menu.pick"), 2);
    QCOMPARE(YesPicks(client), 1);
    QCOMPARE(client.Last("menu.pick")->args.value("index").toInt(), 1);
}

void GsxMenuNavigatorTest::theTurnaroundTurnRearmsTheDeIce()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    AutomationSettings settings;
    settings.autoDeice = true;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    AskForDeIceAndClose(state, nav);
    nav.OnTurnaroundTurned();
    AskForDeIceAndClose(state, nav);

    QCOMPARE(YesPicks(client), 2);
}

void GsxMenuNavigatorTest::resetRearmsTheDeIce()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    AutomationSettings settings;
    settings.autoDeice = true;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    AskForDeIceAndClose(state, nav);
    nav.Reset();
    AskForDeIceAndClose(state, nav);

    QCOMPARE(YesPicks(client), 2);
}

void GsxMenuNavigatorTest::picksSimbriefBlockFuelOnRefuelingLevelMenu()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    nav.RequestRefueling();
    ShowMenu(state, "Select refueling level",
             {
                 "25% - 1705 USGAL / 5182 kg",
                 "40% - 2728 USGAL / 8291 kg",
                 "50% - 3410 USGAL / 10363 kg",
                 "60% - 4092 USGAL / 12436 kg",
                 "70% - 4774 USGAL / 14509 kg",
                 "85% - 5797 USGAL / 17617 kg",
                 "100% - 6820 USGAL / 20726 kg",
                 "30% - BLOCK FUEL from Simbrief - 2027 USGAL / 6151 kg",
                 "Custom refueling using default Fuel menu"
             });
    nav.OnMenuChanged();

    const Sent* pick = client.Last("menu.pick");

    QVERIFY(pick != nullptr);

    QCOMPARE(pick->args.value("index").toInt(), 7);
}

void GsxMenuNavigatorTest::blockFuelNotPickedWhenFlagOff()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    AutomationSettings settings;
    settings.autoSelectGsxChoice = false;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    nav.RequestRefueling();

    ShowMenu(state, "Select refueling level",
             {
                 "50% - 3410 USGAL / 10363 kg",
                 "30% - BLOCK FUEL from Simbrief - 2027 USGAL / 6151 kg",
                 "Custom refueling using default Fuel menu"
             });
    nav.OnMenuChanged();

    QCOMPARE(client.Count("menu.pick"), 0);
}

void GsxMenuNavigatorTest::manualMenuWithGsxChoiceIsPicked()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    ShowMenu(state, "Select fueltruck operator", {"Operator A [GSX choice]", "Operator B"});
    nav.OnMenuChanged();

    const Sent* pick = client.Last("menu.pick");

    QVERIFY(pick != nullptr);

    QCOMPARE(pick->args.value("index").toInt(), 0);
}

void GsxMenuNavigatorTest::manualMenuIsNotRepickedWhileUnchanged()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    ShowMenu(state, "Select fueltruck operator", {"Operator A [GSX choice]", "Operator B"});

    nav.OnMenuChanged();
    nav.OnMenuChanged();
    nav.OnMenuChanged();

    QCOMPARE(client.Count("menu.pick"), 1);
}

void GsxMenuNavigatorTest::manualMenuWithoutGsxChoiceIsIgnored()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    ShowMenu(state, "Activate Services at ZZZZ/Test Airport", {"Request Refueling", "Request Boarding"});
    nav.OnMenuChanged();

    QCOMPARE(client.Count("menu.pick"), 0);
}

void GsxMenuNavigatorTest::theGsxChoiceNeverAnswersThePilotsPullConfirmation()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    long long fakeNow = 5000;
    nav.SetClockForTest([&fakeNow] { return fakeNow; });

    OfferService(state, "Departure");

    nav.RequestPushback();
    MarkServiceTaken(state, "Departure");
    nav.OnMenuChanged();

    ShowMenu(state, "Select pushback direction",
             {"Nose Right/Tail Left (LEFT)", "Nose Left/Tail Right (RIGHT)", "Straight Pull pushback (manual stop, max 100 m)"});
    nav.OnMenuChanged();

    fakeNow += 8000;
    ShowMenu(state, "Are you sure you want to Pull?", {"Yes", "No [GSX choice]"});
    nav.OnMenuChanged();

    QCOMPARE(client.Count("menu.pick"), 0);
}

void GsxMenuNavigatorTest::skipsDisabledEntryAndPicksEnabled()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    nav.RequestBoarding();

    ShowMenu(state, "Select handling operator",
             {"Operator A [GSX choice]", "Operator B [GSX choice]"}, {true, false});
    nav.OnMenuChanged();

    const Sent* pick = client.Last("menu.pick");

    QVERIFY(pick != nullptr);

    QCOMPARE(pick->args.value("index").toInt(), 1);
}

void GsxMenuNavigatorTest::repositionWalksRootThenSubmenu()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    nav.RepositionAircraft();

    QVERIFY(client.Last("menu.toggle") != nullptr);

    ShowMenu(state, "Activate Services at ZZZZ/Test Airport",
             {"Request Deboarding", "Reposition Aircraft", "Operate Stairs"});
    nav.OnMenuChanged();
    const Sent* rootPick = client.Last("menu.pick");

    QVERIFY(rootPick != nullptr);

    QCOMPARE(rootPick->args.value("index").toInt(), 1);

    ShowMenu(state, "Select Position at ZZZZ", {"Reposition here", "Cancel"});
    nav.OnMenuChanged();
    const Sent* herePick = client.Last("menu.pick");

    QVERIFY(herePick != nullptr);

    QCOMPARE(herePick->args.value("index").toInt(), 0);
}

void GsxMenuNavigatorTest::repositionSurvivesTransientCloseAndRootReshow()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    nav.RepositionAircraft();

    ShowMenu(state, "Activate Services at ZZZZ/Test Airport",
             {"Request Refueling", "Reposition Aircraft"});
    nav.OnMenuChanged();

    QCOMPARE(client.Count("menu.pick"), 1);

    state.menu.shown = false;
    state.menu.title.clear();
    state.menu.entries.clear();
    nav.OnMenuChanged();

    ShowMenu(state, "Activate Services at ZZZZ/Test Airport",
             {"Request Refueling", "Reposition Aircraft"});
    nav.OnMenuChanged();
    QCOMPARE(client.Count("menu.pick"), 2);

    ShowMenu(state, "Select Position at ZZZZ", {"Reposition here [Apron 1|Gate 1]", "Cancel"});
    nav.OnMenuChanged();

    const Sent* pick = client.Last("menu.pick");

    QVERIFY(pick != nullptr);

    QCOMPARE(pick->args.value("index").toInt(), 0);
    QCOMPARE(client.Count("menu.pick"), 3);
}

void GsxMenuNavigatorTest::staleSelectPositionMenuClosedAfterReposition()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    long long fakeNow = 0;
    nav.SetClockForTest([&fakeNow] { return fakeNow; });

    nav.RepositionAircraft();
    QCOMPARE(client.Count("menu.toggle"), 1);

    ShowMenu(state, "Select Position at ZZZZ/Test Airport",
             {"Reposition here [Gate 1]", "Cancel"});
    nav.OnMenuChanged();

    QCOMPARE(client.Count("menu.pick"), 1);

    state.menu.shown = false;
    state.menu.title.clear();
    state.menu.entries.clear();
    nav.OnMenuChanged();

    fakeNow = 5000;
    ShowMenu(state, "Select Position at ZZZZ/Test Airport",
             {"Reposition here [Gate 1]", "Cancel"});
    nav.OnMenuChanged();

    QCOMPARE(client.Count("menu.close"), 0);

    fakeNow = 7000;
    nav.OnMenuChanged();

    QCOMPARE(client.Count("menu.close"), 1);
    QCOMPARE(client.Count("menu.pick"), 1);
    QCOMPARE(client.Count("state.get"), 0);

    nav.OnMenuChanged();

    QCOMPARE(client.Count("menu.close"), 1);
}

void GsxMenuNavigatorTest::staleSelectPositionMenuClosedAfterServiceIntentReplacedReposition()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    long long fakeNow = 0;
    nav.SetClockForTest([&fakeNow] { return fakeNow; });

    nav.RepositionAircraft();

    ShowMenu(state, "Select Position at ZZZZ/Test Airport",
             {"Reposition here [Gate 1]", "Cancel"});
    nav.OnMenuChanged();

    QCOMPARE(client.Count("menu.pick"), 1);

    state.menu.shown = false;
    state.menu.title.clear();
    state.menu.entries.clear();
    nav.OnMenuChanged();

    nav.RequestCatering();

    fakeNow = 2000;
    nav.OnMenuChanged();

    fakeNow = 5000;
    ShowMenu(state, "Select Position at ZZZZ/Test Airport",
             {"Reposition here [Gate 1]", "Cancel"});
    nav.OnMenuChanged();

    QCOMPARE(client.Count("menu.close"), 0);

    fakeNow = 7000;
    nav.OnMenuChanged();

    QCOMPARE(client.Count("menu.close"), 1);
    QCOMPARE(client.Count("menu.pick"), 1);
}

void GsxMenuNavigatorTest::staleSelectPositionMenuIgnoredAfterIntentTtl()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    long long fakeNow = 0;
    nav.SetClockForTest([&fakeNow] { return fakeNow; });

    nav.RepositionAircraft();

    ShowMenu(state, "Select Position at ZZZZ/Test Airport",
             {"Reposition here [Gate 1]", "Cancel"});
    nav.OnMenuChanged();

    QCOMPARE(client.Count("menu.pick"), 1);

    state.menu.shown = false;
    state.menu.title.clear();
    state.menu.entries.clear();
    nav.OnMenuChanged();

    fakeNow = 61000;
    ShowMenu(state, "Select Position at ZZZZ/Test Airport",
             {"Reposition here [Gate 1]", "Cancel"});
    nav.OnMenuChanged();

    fakeNow = 63000;
    nav.OnMenuChanged();

    QCOMPARE(client.Count("menu.close"), 0);
}

void GsxMenuNavigatorTest::rejectedPickAllowsRepick()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    nav.RequestRefueling();

    ShowMenu(state, "Select fueltruck operator", {"Operator A [GSX choice]", "Operator B"});

    nav.OnMenuChanged();

    QCOMPARE(client.Count("menu.pick"), 1);

    nav.OnMenuChanged();

    QCOMPARE(client.Count("menu.pick"), 1);

    client.EmitRejection("not_available");
    nav.OnMenuChanged();

    QCOMPARE(client.Count("menu.pick"), 2);
}

void GsxMenuNavigatorTest::resetAllowsRepickingSameMenu()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    nav.RequestRefueling();

    ShowMenu(state, "Select fueltruck operator", {"Operator A [GSX choice]", "Operator B"});
    nav.OnMenuChanged();

    QCOMPARE(client.Count("menu.pick"), 1);

    nav.Reset();
    nav.OnMenuChanged();

    QCOMPARE(client.Count("menu.pick"), 2);
}

void GsxMenuNavigatorTest::staleRefuelingLevelMenuResyncsAndPicksBlockFuel()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    long long fakeNow = 0;
    nav.SetClockForTest([&fakeNow] { return fakeNow; });

    nav.RequestRefueling();

    ShowMenu(state, "Select refueling level",
             {"Request Deboarding", "Request Catering service", "Request Refueling", "Request Boarding"});
    nav.OnMenuChanged();

    QCOMPARE(client.Count("menu.pick"), 0);
    QCOMPARE(client.Count("state.get"), 0);

    fakeNow = 2000;
    nav.OnMenuChanged();

    QCOMPARE(client.Count("state.get"), 1);

    ShowMenu(state, "Select refueling level",
             {
                 "50% - 3410 USGAL / 10363 kg",
                 "30% - BLOCK FUEL from Simbrief - 2027 USGAL / 6151 kg",
                 "Custom refueling using default Fuel menu"
             });
    nav.OnSnapshot();

    const Sent* pick = client.Last("menu.pick");

    QVERIFY(pick != nullptr);

    QCOMPARE(pick->args.value("index").toInt(), 1);
}

void GsxMenuNavigatorTest::swallowedRepositionPickRetriesAfterResyncSnapshot()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    long long fakeNow = 0;
    nav.SetClockForTest([&fakeNow] { return fakeNow; });

    nav.RepositionAircraft();

    ShowMenu(state, "Activate Services at ZZZZ/Test Airport",
             {"Request Refueling", "Reposition Aircraft"});
    nav.OnMenuChanged();

    QCOMPARE(client.Count("menu.pick"), 1);

    fakeNow = 2000;
    nav.OnMenuChanged();

    QCOMPARE(client.Count("state.get"), 1);
    QCOMPARE(client.Count("menu.pick"), 1);

    nav.OnSnapshot();

    QCOMPARE(client.Count("menu.pick"), 2);

    ShowMenu(state, "Select Position at ZZZZ/Test Airport", {"Reposition here [Gate 1]", "Cancel"});
    nav.OnMenuChanged();

    const Sent* pick = client.Last("menu.pick");

    QVERIFY(pick != nullptr);

    QCOMPARE(pick->args.value("index").toInt(), 0);
}

void GsxMenuNavigatorTest::lateResyncSnapshotDoesNotRepickAdvancedMenu()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    long long fakeNow = 0;
    nav.SetClockForTest([&fakeNow] { return fakeNow; });

    ShowMenu(state, "", {});
    nav.OnMenuChanged();

    fakeNow = 2000;
    nav.OnMenuChanged();

    QCOMPARE(client.Count("state.get"), 1);

    ShowMenu(state, "Attach Pushback Tug now?", {"Yes", "No [GSX choice]"});
    nav.OnMenuChanged();

    QCOMPARE(client.Count("menu.pick"), 1);

    nav.OnSnapshot();

    QCOMPARE(client.Count("menu.pick"), 1);
}

void GsxMenuNavigatorTest::stalledMenuResyncIsBounded()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    long long fakeNow = 0;
    nav.SetClockForTest([&fakeNow] { return fakeNow; });

    nav.RequestRefueling();

    ShowMenu(state, "Select refueling level", {"Request Deboarding"});
    nav.OnMenuChanged();

    for (int i = 1; i <= 10; ++i)
    {
        fakeNow = static_cast<long long>(i) * 2000;
        nav.OnMenuChanged();
    }

    QCOMPARE(client.Count("state.get"), 3);
}

void GsxMenuNavigatorTest::groundServiceTriggersUseCanonicalVerbs()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator navigator(&client, &state, &settings, &logger);

    long long fakeNow = 0;
    navigator.SetClockForTest([&fakeNow] { return fakeNow; });

    const auto settle = [&fakeNow, &navigator]
    {
        fakeNow += 1500;
        navigator.OnMenuChanged();
    };

    navigator.ToggleGpu();
    settle();
    QCOMPARE(client.Last("service.trigger")->args.value("service").toString(), QStringLiteral("GPU"));
    MarkServiceTaken(state, "GPU");

    navigator.RequestCatering();
    settle();
    QCOMPARE(client.Last("service.trigger")->args.value("service").toString(), QStringLiteral("Catering"));
    MarkServiceTaken(state, "Catering");

    navigator.RequestLavatory();
    settle();
    QCOMPARE(client.Last("service.trigger")->args.value("service").toString(), QStringLiteral("Lavatory"));
    MarkServiceTaken(state, "Lavatory");

    navigator.RequestWater();
    settle();
    QCOMPARE(client.Last("service.trigger")->args.value("service").toString(), QStringLiteral("Water"));
    MarkServiceTaken(state, "Water");

    navigator.RequestCleaning();
    settle();
    QCOMPARE(client.Last("service.trigger")->args.value("service").toString(), QStringLiteral("Cleaning"));
}

void GsxMenuNavigatorTest::menuSettlesAfterQuietPeriod()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    long long fakeNow = 5000;
    nav.SetClockForTest([&fakeNow] { return fakeNow; });

    QVERIFY(nav.IsMenuSettled());

    nav.RequestRefueling();
    QVERIFY(!nav.IsMenuSettled());

    fakeNow = 6499;

    QVERIFY(!nav.IsMenuSettled());

    fakeNow = 6500;

    QVERIFY(nav.IsMenuSettled());
}

void GsxMenuNavigatorTest::pendingResyncKeepsMenuUnsettled()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    long long fakeNow = 0;
    nav.SetClockForTest([&fakeNow] { return fakeNow; });

    nav.RequestRefueling();

    ShowMenu(state, "Select refueling level", {"Request Refueling"});
    nav.OnMenuChanged();

    fakeNow = 2000;
    nav.OnMenuChanged();

    QCOMPARE(client.Count("state.get"), 1);

    fakeNow = 4000;

    QVERIFY(!nav.IsMenuSettled());
}

void GsxMenuNavigatorTest::triggerWaitsForTheMenuToSettle()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    long long fakeNow = 5000;
    nav.SetClockForTest([&fakeNow] { return fakeNow; });

    ShowMenu(state, "Select refueling level", {"GSX choice"});
    nav.OnMenuChanged();

    QCOMPARE(client.Count("menu.pick"), 1);

    state.menu.shown = false;
    state.menu.title.clear();
    state.menu.entries.clear();
    nav.OnMenuChanged();

    nav.RequestBoarding();

    QCOMPARE(client.Count("service.trigger"), 0);

    fakeNow = 6499;
    nav.OnMenuChanged();

    QCOMPARE(client.Count("service.trigger"), 0);

    fakeNow = 6500;
    nav.OnMenuChanged();

    QCOMPARE(client.Count("service.trigger"), 1);
}

void GsxMenuNavigatorTest::triggerRetriesWhileGsxStillOffersTheService()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    long long fakeNow = 5000;
    nav.SetClockForTest([&fakeNow] { return fakeNow; });

    OfferService(state, "Boarding");

    nav.RequestBoarding();

    QCOMPARE(client.Count("service.trigger"), 1);

    fakeNow = 24999;
    nav.OnMenuChanged();

    QCOMPARE(client.Count("service.trigger"), 1);

    fakeNow = 25000;
    nav.OnMenuChanged();

    QCOMPARE(client.Count("service.trigger"), 2);

    fakeNow = 45000;
    nav.OnMenuChanged();

    QCOMPARE(client.Count("service.trigger"), 3);

    fakeNow = 65000;
    nav.OnMenuChanged();

    QCOMPARE(client.Count("service.trigger"), 3);
    QVERIFY(Logged(logger, "never taken by GSX"));
}

void GsxMenuNavigatorTest::triggerWaitsOutASlowConfirmationInsteadOfFiringAgain()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    long long fakeNow = 5000;
    nav.SetClockForTest([&fakeNow] { return fakeNow; });

    OfferService(state, "OperateStairs");

    nav.CallStairs();

    QCOMPARE(client.Count("service.trigger"), 1);

    fakeNow = 15990;
    nav.OnMenuChanged();

    QCOMPARE(client.Count("service.trigger"), 1);

    MarkServiceTaken(state, "OperateStairs");

    fakeNow = 25000;
    nav.OnMenuChanged();

    QCOMPARE(client.Count("service.trigger"), 1);
}

void GsxMenuNavigatorTest::triggerStopsRetryingOnceGsxTakesIt()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    long long fakeNow = 5000;
    nav.SetClockForTest([&fakeNow] { return fakeNow; });

    OfferService(state, "Boarding");

    nav.RequestBoarding();

    QCOMPARE(client.Count("service.trigger"), 1);

    MarkServiceTaken(state, "Boarding");

    fakeNow = 15000;
    nav.OnMenuChanged();

    QCOMPARE(client.Count("service.trigger"), 1);

    fakeNow = 25000;
    nav.OnMenuChanged();

    QCOMPARE(client.Count("service.trigger"), 1);
    QVERIFY(!Logged(logger, "never taken by GSX"));
}

void GsxMenuNavigatorTest::rearmedTriggerKeepsItsAttemptCount()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    long long fakeNow = 5000;
    nav.SetClockForTest([&fakeNow] { return fakeNow; });

    OfferService(state, "Boarding");

    nav.RequestBoarding();

    QCOMPARE(client.Count("service.trigger"), 1);

    fakeNow = 25000;
    nav.OnMenuChanged();

    QCOMPARE(client.Count("service.trigger"), 2);

    nav.RequestBoarding();

    QCOMPARE(client.Count("service.trigger"), 2);

    fakeNow = 45000;
    nav.OnMenuChanged();

    QCOMPARE(client.Count("service.trigger"), 3);

    fakeNow = 65000;
    nav.OnMenuChanged();

    QCOMPARE(client.Count("service.trigger"), 3);
    QVERIFY(Logged(logger, "never taken by GSX"));
}

void GsxMenuNavigatorTest::rearmedTriggerIsDroppedOnceGsxTakesIt()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    long long fakeNow = 5000;
    nav.SetClockForTest([&fakeNow] { return fakeNow; });

    OfferService(state, "Boarding");

    nav.RequestBoarding();

    QCOMPARE(client.Count("service.trigger"), 1);

    MarkServiceTaken(state, "Boarding");

    nav.RequestBoarding();

    QCOMPARE(client.Count("service.trigger"), 1);

    fakeNow = 25000;
    nav.OnMenuChanged();

    QCOMPARE(client.Count("service.trigger"), 1);
}

void GsxMenuNavigatorTest::stuckMenuIsClosedAfterResyncsAreExhausted()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    long long fakeNow = 5000;
    nav.SetClockForTest([&fakeNow] { return fakeNow; });

    OfferService(state, "Boarding");

    nav.RequestBoarding();
    MarkServiceTaken(state, "Boarding");
    nav.OnMenuChanged();

    ShowMenu(state, "Service in progress", {"Complete now", "Abort service", "Back"});
    nav.OnMenuChanged();

    for (int resync = 0; resync < 3; ++resync)
    {
        fakeNow += 2000;
        nav.OnMenuChanged();
    }

    QCOMPARE(client.Count("state.get"), 3);
    QCOMPARE(client.Count("menu.close"), 0);

    fakeNow += 2000;
    nav.OnMenuChanged();

    QCOMPARE(client.Count("menu.close"), 1);
    QVERIFY(Logged(logger, "the resyncs could not move"));

    fakeNow += 2000;
    nav.OnMenuChanged();

    QCOMPARE(client.Count("menu.close"), 1);
}

void GsxMenuNavigatorTest::theStuckMenuIsDiscardedEvenWithARequestStillPending()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    long long fakeNow = 5000;
    nav.SetClockForTest([&fakeNow] { return fakeNow; });

    OfferService(state, "Boarding");

    nav.RequestBoarding();
    nav.OnMenuChanged();

    ShowMenu(state, "Service in progress", {"Complete now", "Abort service", "Back"});
    nav.OnMenuChanged();

    for (int resync = 0; resync < 3; ++resync)
    {
        fakeNow += 2000;
        nav.OnMenuChanged();
    }

    QCOMPARE(client.Count("menu.close"), 0);

    fakeNow += 2000;
    nav.OnMenuChanged();

    QCOMPARE(client.Count("menu.close"), 1);
    QVERIFY(Logged(logger, "the resyncs could not move"));
}

void GsxMenuNavigatorTest::theLastAttemptGetsTheSameGraceAsTheOthers()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    long long fakeNow = 5000;
    nav.SetClockForTest([&fakeNow] { return fakeNow; });

    OfferService(state, "Boarding");

    nav.RequestBoarding();

    fakeNow = 25000;
    nav.OnMenuChanged();

    fakeNow = 45000;
    nav.OnMenuChanged();

    QCOMPARE(client.Count("service.trigger"), 3);

    fakeNow = 64999;
    nav.OnMenuChanged();

    QVERIFY(!Logged(logger, "never taken by GSX"));

    fakeNow = 65000;
    nav.OnMenuChanged();

    QVERIFY(Logged(logger, "never taken by GSX"));
}

void GsxMenuNavigatorTest::thePushbackDirectionMenuIsNeverDiscarded()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    long long fakeNow = 5000;
    nav.SetClockForTest([&fakeNow] { return fakeNow; });

    OfferService(state, "Departure");

    nav.RequestPushback();
    MarkServiceTaken(state, "Departure");
    nav.OnMenuChanged();

    ShowMenu(state, "Select pushback direction",
             {"Nose Right/Tail Left (LEFT)", "Nose Left/Tail Right (RIGHT)", "QuickEdit Pushback"});
    nav.OnMenuChanged();

    for (int tick = 0; tick < 10; ++tick)
    {
        fakeNow += 2000;
        nav.OnMenuChanged();
    }

    QCOMPARE(client.Count("menu.close"), 0);
    QVERIFY(!Logged(logger, "the resyncs could not move"));
}

void GsxMenuNavigatorTest::aMenuReopenedWithAnEmptyTitleDoesNotInheritTheSpentResyncs()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    long long fakeNow = 5000;
    nav.SetClockForTest([&fakeNow] { return fakeNow; });

    OfferService(state, "Departure");

    nav.RequestPushback();
    MarkServiceTaken(state, "Departure");
    nav.OnMenuChanged();

    ShowMenu(state, "Select pushback direction",
             {"Nose Right/Tail Left (LEFT)", "Nose Left/Tail Right (RIGHT)", "QuickEdit Pushback"});
    nav.OnMenuChanged();

    for (int tick = 0; tick < 4; ++tick)
    {
        fakeNow += 2000;
        nav.OnMenuChanged();
    }

    QVERIFY(Logged(logger, "leaving the pushback direction menu open"));

    state.menu.shown = false;
    state.menu.title.clear();
    state.menu.entries.clear();
    nav.OnMenuChanged();

    const int resyncsBeforeTheReopen = client.Count("state.get");

    fakeNow += 700;
    ShowMenu(state, "", {});
    nav.OnMenuChanged();

    QCOMPARE(client.Count("menu.close"), 0);
    QCOMPARE(client.Count("state.get"), resyncsBeforeTheReopen);
}

void GsxMenuNavigatorTest::aMenuTheClientAskedNothingOfIsNeverDiscarded()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    long long fakeNow = 5000;
    nav.SetClockForTest([&fakeNow] { return fakeNow; });

    OfferService(state, "Departure");

    nav.RequestPushback();
    MarkServiceTaken(state, "Departure");
    nav.OnMenuChanged();

    fakeNow += 300000;
    ShowMenu(state, "Interrupt pushback?", {"Yes", "No", "Select pushback direction", "Cameras"});
    nav.OnMenuChanged();

    for (int tick = 0; tick < 10; ++tick)
    {
        fakeNow += 2000;
        nav.OnMenuChanged();
    }

    QCOMPARE(client.Count("state.get"), 3);
    QCOMPARE(client.Count("menu.close"), 0);
    QVERIFY(!Logged(logger, "the resyncs could not move"));
    QVERIFY(Logged(logger, "RemoteAPI leaving the menu open: the client asked for nothing on it: 'Interrupt pushback?'"));
}

void GsxMenuNavigatorTest::aMenuLeftOpenForThePilotIsStillDiscardedOnceTheClientWaitsOnIt()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    long long fakeNow = 5000;
    nav.SetClockForTest([&fakeNow] { return fakeNow; });

    const auto stallTheServicesMenu = [&]
    {
        ShowMenu(state, "Activate Services at ZZZZ/Test Airport", {"Request Boarding", "Operate Stairs"});
        nav.OnMenuChanged();

        for (int tick = 0; tick < 4; ++tick)
        {
            fakeNow += 2000;
            nav.OnMenuChanged();
        }
    };

    stallTheServicesMenu();

    QCOMPARE(client.Count("menu.close"), 0);

    state.menu.shown = false;
    state.menu.title.clear();
    state.menu.entries.clear();
    nav.OnSnapshot();

    OfferService(state, "OperateStairs");
    nav.CallStairs();

    QCOMPARE(client.Count("service.trigger"), 1);

    stallTheServicesMenu();

    QCOMPARE(client.Count("menu.close"), 1);
    QVERIFY(Logged(logger, "the resyncs could not move"));
}

void GsxMenuNavigatorTest::resetAllowsClosingTheSameStuckMenuAgain()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    long long fakeNow = 5000;
    nav.SetClockForTest([&fakeNow] { return fakeNow; });

    const auto driveToStuckMenu = [&]
    {
        state.menu.shown = false;
        nav.RequestBoarding();

        ShowMenu(state, "Service in progress", {"Complete now", "Abort service", "Back"});
        nav.OnMenuChanged();

        for (int tick = 0; tick < 4; ++tick)
        {
            fakeNow += 2000;
            nav.OnMenuChanged();
        }
    };

    driveToStuckMenu();

    QCOMPARE(client.Count("menu.close"), 1);

    nav.Reset();
    driveToStuckMenu();

    QCOMPARE(client.Count("menu.close"), 2);
}

void GsxMenuNavigatorTest::aRequestForAServiceAlreadyUnderwayIsDroppedBeforeSending()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    long long fakeNow = 0;
    nav.SetClockForTest([&fakeNow] { return fakeNow; });

    MarkServiceTaken(state, "OperateStairs");

    nav.CallStairs();
    fakeNow += 1500;
    nav.OnMenuChanged();

    QCOMPARE(client.Count("service.trigger"), 0);
    QVERIFY(Logged(logger, "already underway"));
}

void GsxMenuNavigatorTest::theGpuToggleStillFiresWhileTheServiceRuns()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    long long fakeNow = 0;
    nav.SetClockForTest([&fakeNow] { return fakeNow; });

    MarkServiceTaken(state, "GPU");

    nav.ToggleGpu();
    fakeNow += 1500;
    nav.OnMenuChanged();

    QCOMPARE(client.Count("service.trigger"), 1);
}

void GsxMenuNavigatorTest::aServiceThatTogglesIsNeverSentTwice()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    long long fakeNow = 5000;
    nav.SetClockForTest([&fakeNow] { return fakeNow; });

    OfferService(state, "GPU");

    nav.ToggleGpu();

    QCOMPARE(client.Count("service.trigger"), 1);

    fakeNow = 30000;
    nav.OnMenuChanged();

    QCOMPARE(client.Count("service.trigger"), 1);
    QVERIFY(!Logged(logger, "stopped waiting"));

    fakeNow = 60000;
    nav.OnMenuChanged();

    QCOMPARE(client.Count("service.trigger"), 1);
    QVERIFY(!Logged(logger, "stopped waiting"));

    fakeNow = 100000;
    nav.OnMenuChanged();

    QCOMPARE(client.Count("service.trigger"), 1);
    QVERIFY(Logged(logger, "stopped waiting"));
}

void GsxMenuNavigatorTest::theStairsAreNeverAskedForASecondTime()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    long long fakeNow = 5000;
    nav.SetClockForTest([&fakeNow] { return fakeNow; });

    OfferService(state, "OperateStairs");

    nav.CallStairs();

    QCOMPARE(client.Count("service.trigger"), 1);

    fakeNow = 30000;
    nav.OnMenuChanged();

    QCOMPARE(client.Count("service.trigger"), 1);
    QVERIFY(!Logged(logger, "stopped waiting"));

    fakeNow = 60000;
    nav.OnMenuChanged();

    QCOMPARE(client.Count("service.trigger"), 1);
    QVERIFY(!Logged(logger, "stopped waiting"));

    fakeNow = 100000;
    nav.OnMenuChanged();

    QCOMPARE(client.Count("service.trigger"), 1);
    QVERIFY(Logged(logger, "stopped waiting"));
}

namespace
{
    constexpr auto kDeIceQuestionTitle = "Ice warning: do you request the de-icing treatment?";
    constexpr auto kBlockedSpotTitle =
        "The rear cargo loader is waiting for the spot where the stairs at L Entry FWD are parked. Remove the stairs?";

    struct NavigatorRig
    {
        FakeRemoteClient client;
        GsxRemoteState state;
        AutomationSettings settings;
        FakeDomainLogger logger;
        GsxMenuNavigator nav{&client, &state, &settings, &logger};
        long long now = 5000;

        NavigatorRig()
        {
            settings.autoDeice = true;
            nav.SetClockForTest([this] { return now; });
        }
    };

    QJsonArray QueueOf(const GsxMenuNavigator& nav)
    {
        const std::string text = nav.TakeMemory().Text("pendingRequests", "[]");

        return QJsonDocument::fromJson(QByteArray::fromStdString(text)).array();
    }

    QJsonObject EntryFor(const GsxMenuNavigator& nav, const QString& label)
    {
        for (const QJsonValueConstRef& value : QueueOf(nav))
        {
            if (value.toObject().value("label").toString() == label)
            {
                return value.toObject();
            }
        }

        return {};
    }

    int TriggersFor(const FakeRemoteClient& client, const QString& serviceId)
    {
        return static_cast<int>(std::ranges::count_if(client.sent, [&serviceId](const Sent& request)
        {
            return request.verb == "service.trigger" && request.args.value("service").toString() == serviceId;
        }));
    }

    void RequestToggling(GsxMenuNavigator& nav, const QString& serviceId)
    {
        if (serviceId == "GPU")
        {
            nav.ToggleGpu();

            return;
        }

        if (serviceId == "OperateStairs")
        {
            nav.CallStairs();

            return;
        }

        nav.CallJetway();
    }

    void DirtyTheMemory(NavigatorRig& rig)
    {
        OfferService(rig.state, "Boarding");
        rig.nav.RequestBoarding();
        rig.nav.RequestCatering();
        rig.nav.RequestDepartureClearance();

        ShowMenu(rig.state, kDeIceQuestionTitle, {"Yes", "No [GSX choice]"});
        rig.nav.OnMenuChanged();

        ShowMenu(rig.state, kBlockedSpotTitle, {"Yes, remove the stairs", "No, keep the stairs"});
        rig.nav.OnMenuChanged();
    }
}

void GsxMenuNavigatorTest::aServiceThatTogglesAndWasSentIsNotSentAgainAfterTheRestore_data()
{
    QTest::addColumn<QString>("serviceId");

    QTest::newRow("gpu") << QStringLiteral("GPU");
    QTest::newRow("stairs") << QStringLiteral("OperateStairs");
    QTest::newRow("jetways") << QStringLiteral("OperateJetways");
}

void GsxMenuNavigatorTest::aServiceThatTogglesAndWasSentIsNotSentAgainAfterTheRestore()
{
    QFETCH(QString, serviceId);

    NavigatorRig previous;
    OfferService(previous.state, serviceId.toStdString());
    RequestToggling(previous.nav, serviceId);
    previous.nav.RequestCatering();

    QCOMPARE(TriggersFor(previous.client, serviceId), 1);
    QCOMPARE(TriggersFor(previous.client, QStringLiteral("Catering")), 0);

    NavigatorRig resumed;
    resumed.now = 30000;
    OfferService(resumed.state, serviceId.toStdString());
    OfferService(resumed.state, "Catering");
    resumed.nav.RestoreMemory(previous.nav.TakeMemory());
    resumed.nav.OnMenuChanged();

    QCOMPARE(TriggersFor(resumed.client, QStringLiteral("Catering")), 1);
    QCOMPARE(TriggersFor(resumed.client, serviceId), 0);
}

void GsxMenuNavigatorTest::aRestoredGpuRequestStillFiresWhileTheServiceRuns()
{
    NavigatorRig previous;
    previous.now = 0;
    previous.nav.ToggleGpu();

    QVERIFY(previous.client.sent.empty());

    NavigatorRig resumed;
    MarkServiceTaken(resumed.state, "GPU");
    resumed.nav.RestoreMemory(previous.nav.TakeMemory());
    resumed.nav.OnMenuChanged();

    QCOMPARE(TriggersFor(resumed.client, QStringLiteral("GPU")), 1);
}

void GsxMenuNavigatorTest::aDepartureClearanceNeverSentGoesOutAfterTheRestore()
{
    NavigatorRig previous;
    previous.now = 0;
    previous.nav.RequestDepartureClearance();

    QVERIFY(previous.client.sent.empty());

    NavigatorRig resumed;
    OfferService(resumed.state, "Departure");
    resumed.nav.RestoreMemory(previous.nav.TakeMemory());
    resumed.nav.OnMenuChanged();

    QCOMPARE(TriggersFor(resumed.client, QStringLiteral("Departure")), 1);

    resumed.now += 30000;
    resumed.nav.OnMenuChanged();

    QCOMPARE(TriggersFor(resumed.client, QStringLiteral("Departure")), 1);
    QVERIFY(!Logged(resumed.logger, "never taken by GSX"));
}

void GsxMenuNavigatorTest::restoringSendsNothingAndOpensNoToolbar()
{
    NavigatorRig previous;
    previous.now = 0;
    previous.nav.RequestCatering();
    previous.nav.RequestDepartureClearance();

    PanelRig rig(GsxPanelMode::AllRequests);
    rig.PanelIs("closed");
    rig.client.refusing = true;
    GsxMenuNavigator resumed(&rig.client, &rig.state, &rig.settings, &rig.logger, &rig.plugin);
    resumed.SetClockForTest([] { return 9000LL; });
    resumed.RestoreMemory(previous.nav.TakeMemory());

    QVERIFY(rig.client.refused.empty());
    QCOMPARE(rig.PanelCommands(), 0);
}

void GsxMenuNavigatorTest::aStampInTheFutureIsClampedSoTheRequestRetriesOnSchedule()
{
    NavigatorRig previous;
    previous.now = 50000;
    OfferService(previous.state, "Boarding");
    previous.nav.RequestBoarding();

    QCOMPARE(TriggersFor(previous.client, QStringLiteral("Boarding")), 1);

    NavigatorRig resumed;
    resumed.now = 10000;
    OfferService(resumed.state, "Boarding");
    resumed.nav.RestoreMemory(previous.nav.TakeMemory());
    resumed.nav.OnMenuChanged();

    QVERIFY(resumed.client.sent.empty());

    resumed.now = 29999;
    resumed.nav.OnMenuChanged();

    QVERIFY(resumed.client.sent.empty());

    resumed.now = 30000;
    resumed.nav.OnMenuChanged();

    QCOMPARE(TriggersFor(resumed.client, QStringLiteral("Boarding")), 1);
}

void GsxMenuNavigatorTest::aRestoredRequestIsHandledWithAnIntent()
{
    NavigatorRig previous;
    OfferService(previous.state, "Departure");
    previous.nav.RequestPushback();

    QCOMPARE(TriggersFor(previous.client, QStringLiteral("Departure")), 1);

    NavigatorRig resumed;
    resumed.now = 6000;
    resumed.nav.RestoreMemory(previous.nav.TakeMemory());
    ShowMenu(resumed.state, "Attach Pushback Tug?", {"Yes", "No"});
    resumed.nav.OnMenuChanged();

    const Sent* pick = resumed.client.Last("menu.pick");

    QVERIFY(pick != nullptr);
    QCOMPARE(pick->args.value("index").toInt(), 1);

    NavigatorRig bare;
    bare.now = 6000;
    bare.nav.RestoreMemory(MemoryBag{});
    ShowMenu(bare.state, "Attach Pushback Tug?", {"Yes", "No"});
    bare.nav.OnMenuChanged();

    QCOMPARE(bare.client.Count("menu.pick"), 0);
}

void GsxMenuNavigatorTest::theMemoryTakenRestoredAndTakenAgainIsEqual()
{
    NavigatorRig previous;
    DirtyTheMemory(previous);

    const MemoryBag taken = previous.nav.TakeMemory();

    NavigatorRig resumed;
    resumed.nav.RestoreMemory(taken);

    QVERIFY(resumed.nav.TakeMemory() == taken);
    QVERIFY(resumed.nav.WereStairsKeptInPlace());

    ShowMenu(resumed.state, kDeIceQuestionTitle, {"Yes", "No [GSX choice]"});
    resumed.nav.OnMenuChanged();

    const Sent* pick = resumed.client.Last("menu.pick");

    QVERIFY(pick != nullptr);
    QCOMPARE(pick->args.value("index").toInt(), 1);
}

void GsxMenuNavigatorTest::theToolbarTheClientOpenedIsClosedByTheRestoredNavigator()
{
    PanelRig before(GsxPanelMode::OnPushback);
    before.PanelIs("closed");
    GsxMenuNavigator previous(&before.client, &before.state, &before.settings, &before.logger, &before.plugin);
    previous.OpenPushbackPanel();

    QCOMPARE(before.PanelCommands(), 1);

    PanelRig after(GsxPanelMode::OnPushback);
    after.PanelIs("open");
    GsxMenuNavigator resumed(&after.client, &after.state, &after.settings, &after.logger, &after.plugin);
    resumed.RestoreMemory(previous.TakeMemory());
    resumed.OnPushbackStarted();

    QCOMPARE(after.PanelCommands(), 1);
    QCOMPARE(after.LastPanelPayload(), QString(IntegratorPluginCommBus::kCommandClose));
}

namespace
{
    enum class Dirt : std::uint8_t
    {
        ServiceIntent,
        Reposition,
        PendingResync,
        RefuelCompletion,
        RememberedPick
    };

    constexpr auto kChoiceTitle = "Pick an option";
    constexpr auto kTugTitle = "Attach Pushback Tug?";
    constexpr auto kPositionTitle = "Select Position at ZZZZ/Test Airport";
    constexpr auto kServiceTitle = "Ground services";

    void ShowTheChoiceMenu(NavigatorRig& rig)
    {
        ShowMenu(rig.state, kChoiceTitle, {"Option A", "Option B [GSX choice]"});
    }

    void Dirty(NavigatorRig& rig, const Dirt dirt)
    {
        switch (dirt)
        {
        case Dirt::ServiceIntent:
            rig.nav.RequestDepartureClearance();
            break;
        case Dirt::Reposition:
            rig.nav.RepositionAircraft();
            break;
        case Dirt::PendingResync:
            ShowMenu(rig.state, "Stalled menu", {"Option"});
            rig.nav.OnMenuChanged();
            rig.now += 2000;
            rig.nav.OnMenuChanged();
            break;
        case Dirt::RefuelCompletion:
            rig.nav.CompleteRefuel();
            break;
        case Dirt::RememberedPick:
            ShowTheChoiceMenu(rig);
            rig.nav.OnMenuChanged();
            break;
        }
    }

    QStringList Observe(NavigatorRig& rig)
    {
        QStringList seen;
        std::size_t recorded = rig.client.sent.size();

        const auto step = [&](const QString& name)
        {
            rig.now += 100;
            rig.nav.OnMenuChanged();

            for (; recorded < rig.client.sent.size(); ++recorded)
            {
                const Sent& request = rig.client.sent[recorded];
                seen.append(QStringLiteral("%1:%2:%3").arg(name, request.verb).arg(request.args.value("index").toInt(-1)));
            }

            rig.state.menu.shown = false;
            rig.state.menu.title.clear();
            rig.state.menu.entries.clear();
            rig.nav.OnMenuChanged();
            recorded = rig.client.sent.size();
        };

        rig.now += 2000;
        seen.append(rig.nav.IsMenuSettled() ? QStringLiteral("settled") : QStringLiteral("unsettled"));

        ShowTheChoiceMenu(rig);
        step(QStringLiteral("choice"));

        ShowMenu(rig.state, kTugTitle, {"Yes", "No"});
        step(QStringLiteral("tug"));

        ShowMenu(rig.state, kPositionTitle, {"Reposition here [Gate 1]", "Cancel"});
        step(QStringLiteral("position"));

        ShowMenu(rig.state, kServiceTitle, {"Refueling: 50%", "Back"});
        step(QStringLiteral("refuel"));

        return seen;
    }
}

void GsxMenuNavigatorTest::anEmptyBagRestoresToTheStateOfReset_data()
{
    QTest::addColumn<int>("dirt");

    QTest::newRow("a service intent") << static_cast<int>(Dirt::ServiceIntent);
    QTest::newRow("a reposition under way") << static_cast<int>(Dirt::Reposition);
    QTest::newRow("a pending resync") << static_cast<int>(Dirt::PendingResync);
    QTest::newRow("a refuel completion") << static_cast<int>(Dirt::RefuelCompletion);
    QTest::newRow("a remembered pick") << static_cast<int>(Dirt::RememberedPick);
}

void GsxMenuNavigatorTest::anEmptyBagRestoresToTheStateOfReset()
{
    QFETCH(int, dirt);

    NavigatorRig fresh;
    const QStringList expected = Observe(fresh);

    NavigatorRig dirtied;
    Dirty(dirtied, static_cast<Dirt>(dirt));

    QVERIFY(Observe(dirtied) != expected);

    NavigatorRig restored;
    Dirty(restored, static_cast<Dirt>(dirt));
    restored.nav.RestoreMemory(MemoryBag{});

    QCOMPARE(Observe(restored), expected);
    QVERIFY(restored.nav.TakeMemory() == fresh.nav.TakeMemory());
    QVERIFY(!restored.nav.WereStairsKeptInPlace());
}

void GsxMenuNavigatorTest::malformedQueueTextRestoresAnEmptyQueue_data()
{
    QTest::addColumn<QString>("text");

    QTest::newRow("not json") << QStringLiteral("{not json");
    QTest::newRow("an object") << QStringLiteral("{\"verb\":\"service.trigger\"}");
    QTest::newRow("a scalar") << QStringLiteral("42");
    QTest::newRow("empty text") << QString();
    QTest::newRow("elements without a verb") << QStringLiteral("[1,\"x\",{},{\"label\":\"a\"}]");
}

void GsxMenuNavigatorTest::malformedQueueTextRestoresAnEmptyQueue()
{
    QFETCH(QString, text);

    MemoryBag memory;
    memory.PutText("pendingRequests", text.toStdString());

    NavigatorRig resumed;
    NavigatorRig fresh;
    resumed.nav.RestoreMemory(memory);

    QVERIFY(resumed.nav.TakeMemory() == fresh.nav.TakeMemory());

    OfferService(resumed.state, "Catering");
    resumed.nav.OnMenuChanged();

    QVERIFY(resumed.client.sent.empty());
}

void GsxMenuNavigatorTest::aRefusedSendOfAServiceThatTogglesIsNotAnAttempt()
{
    NavigatorRig rig;
    rig.client.refusing = true;
    OfferService(rig.state, "GPU");

    rig.nav.ToggleGpu();

    QCOMPARE(rig.client.Refused("service.trigger"), 1);

    rig.now += 30000;
    rig.nav.OnMenuChanged();
    rig.now += 40000;
    rig.nav.OnMenuChanged();

    QVERIFY(!Logged(rig.logger, "stopped waiting"));

    rig.client.refusing = false;
    rig.now += 1500;
    rig.nav.OnMenuChanged();

    QCOMPARE(TriggersFor(rig.client, QStringLiteral("GPU")), 1);
}

void GsxMenuNavigatorTest::aRefusedSendOfARetriedServiceIsNotAnAttempt()
{
    NavigatorRig rig;
    rig.client.refusing = true;
    OfferService(rig.state, "Catering");

    rig.nav.RequestCatering();

    for (int i = 0; i < 4; ++i)
    {
        rig.now += 20000;
        rig.nav.OnMenuChanged();
    }

    const QJsonObject entry = EntryFor(rig.nav, QStringLiteral("Catering"));

    QVERIFY(!Logged(rig.logger, "never taken by GSX"));
    QCOMPARE(entry.value("attempts").toInt(-1), 0);
    QCOMPARE(entry.value("lastSentMs").toInteger(-1), 0LL);
    QCOMPARE(rig.client.Refused("service.trigger"), 5);

    rig.client.refusing = false;
    rig.now += 1500;
    rig.nav.OnMenuChanged();

    QCOMPARE(TriggersFor(rig.client, QStringLiteral("Catering")), 1);
}

void GsxMenuNavigatorTest::aRefusedSendOfARequestWithoutAServiceIsNotTakenAsSent_data()
{
    QTest::addColumn<QString>("verb");

    QTest::newRow("departure clearance") << QStringLiteral("service.trigger");
    QTest::newRow("simbrief reload") << QStringLiteral("command.run");
}

void GsxMenuNavigatorTest::aRefusedSendOfARequestWithoutAServiceIsNotTakenAsSent()
{
    QFETCH(QString, verb);

    NavigatorRig rig;
    rig.client.refusing = true;

    if (verb == "service.trigger")
    {
        rig.nav.RequestDepartureClearance();
    }
    else
    {
        rig.nav.RequestSimbriefLoad();
    }

    QCOMPARE(rig.client.Refused(verb), 1);

    rig.now += 1500;
    rig.nav.OnMenuChanged();

    rig.client.refusing = false;
    rig.now += 1500;
    rig.nav.OnMenuChanged();

    QCOMPARE(rig.client.Count(verb), 1);
}

void GsxMenuNavigatorTest::aRefusedPickDoesNotSpendTheDeIceAnswer()
{
    NavigatorRig rig;
    rig.client.refusing = true;

    ShowMenu(rig.state, kDeIceQuestionTitle, {"Yes", "No [GSX choice]"});
    rig.nav.OnMenuChanged();

    QCOMPARE(rig.client.Refused("menu.pick"), 1);
    QCOMPARE(rig.client.refused.front().args.value("index").toInt(-1), 0);
    QVERIFY(!rig.nav.TakeMemory().Flag("deIceYesSpent", true));

    rig.client.refusing = false;
    rig.nav.OnMenuChanged();

    QCOMPARE(YesPicks(rig.client), 1);
    QVERIFY(rig.nav.TakeMemory().Flag("deIceYesSpent", false));
}

void GsxMenuNavigatorTest::aRefusedPickIsNotReportedAsDone()
{
    NavigatorRig rig;
    rig.client.refusing = true;

    ShowMenu(rig.state, "Confirm good engine start", {"Confirm good engine start", "Cancel"});

    QVERIFY(!rig.nav.ConfirmGoodEngines());
    QCOMPARE(rig.client.Refused("menu.pick"), 1);

    rig.client.refusing = false;
    rig.now += 1500;
    rig.nav.OnMenuChanged();

    QCOMPARE(rig.client.Count("menu.pick"), 0);

    QVERIFY(rig.nav.ConfirmGoodEngines());
    QCOMPARE(rig.client.Count("menu.pick"), 1);
    QCOMPARE(rig.client.Last("menu.pick")->args.value("index").toInt(-1), 0);
}

void GsxMenuNavigatorTest::aRefusedResyncDoesNotLeaveTheMenuUnsettled()
{
    NavigatorRig rig;
    rig.now = 0;
    rig.client.refusing = true;

    rig.nav.RequestRefueling();

    ShowMenu(rig.state, "Select refueling level", {"Request Refueling"});
    rig.nav.OnMenuChanged();

    rig.now = 2000;
    rig.nav.OnMenuChanged();

    QCOMPARE(rig.client.Refused("state.get"), 1);

    rig.now = 4000;

    QVERIFY(rig.nav.IsMenuSettled());
}

namespace
{
    constexpr auto kServiceTrigger = "service.trigger";

    QString Element(const QString& verb, const QString& label, const int attempts, const qint64 lastSentMs,
                    const qint64 armedMs)
    {
        const QJsonObject element{{"verb", verb},
                                  {"args", QJsonObject{{"service", label}}},
                                  {"label", label},
                                  {"confirmId", label},
                                  {"lastSentMs", lastSentMs},
                                  {"attempts", attempts},
                                  {"firesWhileUnderway", false},
                                  {"armedMs", armedMs}};

        return QString::fromUtf8(QJsonDocument(element).toJson(QJsonDocument::Compact));
    }

    QString Summary(const GsxMenuNavigator& nav)
    {
        QStringList parts;
        for (const QJsonValueConstRef& value : QueueOf(nav))
        {
            const QJsonObject entry = value.toObject();
            parts.append(QStringLiteral("%1:%2:%3:%4")
                         .arg(entry.value("label").toString())
                         .arg(entry.value("attempts").toInt(-1))
                         .arg(entry.value("lastSentMs").toInteger(-1))
                         .arg(entry.value("armedMs").toInteger(-1)));
        }

        return parts.join(QLatin1Char('|'));
    }

    MemoryBag BagWithQueue(const QString& text)
    {
        MemoryBag memory;
        memory.PutText("pendingRequests", text.toStdString());

        return memory;
    }

    int MessagesWith(const FakeDomainLogger& logger, const std::string& needle)
    {
        return static_cast<int>(std::ranges::count_if(logger.messages, [&needle](const std::string& message)
        {
            return message.find(needle) != std::string::npos;
        }));
    }
}

void GsxMenuNavigatorTest::aCorruptQueueIsHardenedOnRestore_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("expected");

    const QString trigger = QString::fromLatin1(kServiceTrigger);

    QTest::newRow("a negative attempt count")
        << QStringLiteral("[%1]").arg(Element(trigger, "Catering", -5, 100, 100))
        << QStringLiteral("Catering:0:100:5000");
    QTest::newRow("an attempt count above the cap")
        << QStringLiteral("[%1]").arg(Element(trigger, "Catering", 99, 100, 100))
        << QStringLiteral("Catering:3:100:100");
    QTest::newRow("negative stamps")
        << QStringLiteral("[%1]").arg(Element(trigger, "Catering", 1, -50, -50))
        << QStringLiteral("Catering:1:0:0");
    QTest::newRow("stamps in the future")
        << QStringLiteral("[%1]").arg(Element(trigger, "Catering", 1, 9000000, 9000000))
        << QStringLiteral("Catering:1:5000:5000");
    QTest::newRow("the same verb and label twice")
        << QStringLiteral("[%1,%2]").arg(Element(trigger, "Catering", 1, 100, 100),
                                         Element(trigger, "Catering", 2, 200, 200))
        << QStringLiteral("Catering:1:100:100");
    QTest::newRow("verbs the navigator never arms")
        << QStringLiteral("[%1,%2]").arg(Element("menu.pick", "a", 0, 0, 0), Element("menu.close", "b", 0, 0, 0))
        << QString();
    QTest::newRow("the simbrief command")
        << QStringLiteral("[%1]").arg(Element("command.run", "RELOAD_SIMBRIEF", 1, 100, 100))
        << QStringLiteral("RELOAD_SIMBRIEF:1:100:100");
    QTest::newRow("a valid element among malformed ones")
        << QStringLiteral("[1,\"x\",{},{\"label\":\"a\"},%1,{\"verb\":\"menu.pick\",\"label\":\"p\"},%2,null]")
           .arg(Element(trigger, "Catering", 1, 100, 100), Element(trigger, "Departure", 0, 0, 0))
        << QStringLiteral("Catering:1:100:100|Departure:0:0:5000");
}

void GsxMenuNavigatorTest::aCorruptQueueIsHardenedOnRestore()
{
    QFETCH(QString, text);
    QFETCH(QString, expected);

    NavigatorRig resumed;
    resumed.nav.RestoreMemory(BagWithQueue(text));

    QCOMPARE(Summary(resumed.nav), expected);
}

void GsxMenuNavigatorTest::thePersistedQueueNamesWhatEachFieldMeans()
{
    NavigatorRig rig;
    OfferService(rig.state, "GPU");
    OfferService(rig.state, "OperateStairs");

    rig.nav.ToggleGpu();
    rig.nav.CallStairs();

    const QJsonObject gpu = EntryFor(rig.nav, QStringLiteral("GPU"));
    const QJsonObject stairs = EntryFor(rig.nav, QStringLiteral("OperateStairs"));

    QStringList keys = gpu.keys();
    keys.sort();

    QCOMPARE(keys, (QStringList{"args", "armedMs", "attempts", "confirmId", "firesWhileUnderway", "label",
                                "lastSentMs", "verb"}));
    QCOMPARE(gpu.value("attempts").toInt(-1), 1);
    QCOMPARE(stairs.value("attempts").toInt(-1), 0);
    QVERIFY(gpu.value("firesWhileUnderway").toBool(false));
    QVERIFY(stairs.contains("firesWhileUnderway"));
    QVERIFY(!stairs.value("firesWhileUnderway").toBool(true));
    QCOMPARE(gpu.value("armedMs").toInteger(-1), 5000LL);
}

void GsxMenuNavigatorTest::thePanelLatchesSurviveTheRestore()
{
    PanelRig before(GsxPanelMode::OnPushback);
    before.PanelIs("closed");
    GsxMenuNavigator previous(&before.client, &before.state, &before.settings, &before.logger, &before.plugin);
    previous.OpenPushbackPanel();
    before.PanelIs("open");
    previous.ClosePushbackPanel();

    const MemoryBag taken = previous.TakeMemory();

    QVERIFY(taken.Flag("panelOpenSpent", false));
    QVERIFY(taken.Flag("panelCloseSpent", false));
    QVERIFY(taken.Flag("panelOpenedByUs", false));

    PanelRig after(GsxPanelMode::OnPushback);
    after.PanelIs("closed");
    GsxMenuNavigator resumed(&after.client, &after.state, &after.settings, &after.logger, &after.plugin);
    resumed.RestoreMemory(taken);

    QVERIFY(resumed.TakeMemory().Flag("panelOpenedByUs", false));

    resumed.OpenPushbackPanel();

    QCOMPARE(after.PanelCommands(), 0);

    after.PanelIs("open");
    resumed.ClosePushbackPanel();

    QCOMPARE(after.PanelCommands(), 0);
}

void GsxMenuNavigatorTest::aWaitForThePanelDoesNotSurviveTheRestore()
{
    PanelRig rig(GsxPanelMode::OnPushback);
    rig.PanelIs("closed");
    GsxMenuNavigator nav(&rig.client, &rig.state, &rig.settings, &rig.logger, &rig.plugin);

    long long now = 5000;
    nav.SetClockForTest([&now] { return now; });

    nav.OpenPushbackPanel();
    nav.RestoreMemory(BagWithQueue(QStringLiteral("[%1]").arg(
        Element(QString::fromLatin1(kServiceTrigger), "Catering", 0, 0, 0))));
    OfferService(rig.state, "Catering");
    nav.OnMenuChanged();

    QCOMPARE(rig.client.Count(kServiceTrigger), 1);
}

void GsxMenuNavigatorTest::theLabelOfARestoredRequestIsKept()
{
    NavigatorRig previous;
    OfferService(previous.state, "Boarding");
    previous.nav.RequestBoarding();

    QCOMPARE(TriggersFor(previous.client, QStringLiteral("Boarding")), 1);

    NavigatorRig resumed;
    resumed.now = 6000;
    resumed.nav.RestoreMemory(previous.nav.TakeMemory());
    ShowMenu(resumed.state, kBlockedSpotTitle, {"Yes, remove the stairs", "No, keep the stairs"});
    resumed.nav.OnMenuChanged();

    const Sent* pick = resumed.client.Last("menu.pick");

    QVERIFY(pick != nullptr);
    QCOMPARE(pick->args.value("index").toInt(-1), 1);
    QVERIFY(resumed.nav.WereStairsKeptInPlace());
}

void GsxMenuNavigatorTest::anUnknownNameInTheBagIsIgnored()
{
    NavigatorRig previous;
    DirtyTheMemory(previous);

    const MemoryBag taken = previous.nav.TakeMemory();

    MemoryBag noisy = taken;
    noisy.PutText("someFutureEntry", "x");
    noisy.PutFlag("anotherFutureEntry", true);

    NavigatorRig resumed;
    resumed.nav.RestoreMemory(noisy);

    QVERIFY(resumed.nav.TakeMemory() == taken);
}

void GsxMenuNavigatorTest::aRestoredRequestNeverSentRestartsItsGiveUpClock()
{
    NavigatorRig previous;
    previous.now = 0;
    previous.nav.RequestDepartureClearance();

    NavigatorRig resumed;
    resumed.now = 1000000;
    OfferService(resumed.state, "Departure");
    resumed.nav.RestoreMemory(previous.nav.TakeMemory());

    QCOMPARE(EntryFor(resumed.nav, QStringLiteral("Departure")).value("armedMs").toInteger(-1), 1000000LL);

    resumed.nav.OnMenuChanged();

    QCOMPARE(TriggersFor(resumed.client, QStringLiteral("Departure")), 1);
    QVERIFY(!Logged(resumed.logger, "never left the client"));
}

void GsxMenuNavigatorTest::aRefusedResyncIsNotCounted()
{
    NavigatorRig rig;
    rig.now = 0;
    rig.client.refusing = true;

    ShowMenu(rig.state, "Stalled menu", {"Option"});
    rig.nav.OnMenuChanged();

    for (int pass = 0; pass < 4; ++pass)
    {
        rig.now += 2000;
        rig.nav.OnMenuChanged();
    }

    QVERIFY(!Logged(rig.logger, "requesting snapshot resync"));
    QCOMPARE(rig.client.Refused("state.get"), 4);

    rig.client.refusing = false;
    rig.now += 2000;
    rig.nav.OnMenuChanged();

    QVERIFY(Logged(rig.logger, "snapshot resync 1/3"));
}

void GsxMenuNavigatorTest::aRefusedSendEndsThePass()
{
    NavigatorRig rig;
    rig.client.refusing = true;
    ShowMenu(rig.state, "Some menu", {"Option"});

    rig.nav.RequestCatering();

    QCOMPARE(static_cast<int>(rig.client.refused.size()), 1);
    QCOMPARE(rig.client.Refused("menu.close"), 1);

    for (int pass = 0; pass < 5; ++pass)
    {
        rig.now += 1500;
        rig.nav.OnMenuChanged();
    }

    QCOMPARE(static_cast<int>(rig.client.refused.size()), 6);
    QCOMPARE(EntryFor(rig.nav, QStringLiteral("Catering")).value("attempts").toInt(-1), 0);
}

void GsxMenuNavigatorTest::aRefusedPickDoesNotFallThroughToAnotherAnswer_data()
{
    QTest::addColumn<QString>("title");
    QTest::addColumn<QStringList>("entries");
    QTest::addColumn<bool>("ownStairs");
    QTest::addColumn<int>("attempted");

    QTest::newRow("the de-ice yes does not become the gsx choice")
        << QString::fromLatin1(kDeIceQuestionTitle) << QStringList{"Yes", "No [GSX choice]"} << false << 0;
    QTest::newRow("the jetway does not become the own airstairs")
        << QStringLiteral("Do you want to use your own airstairs?") << QStringList{"No", "Use jetway", "Yes"}
        << true << 1;
}

void GsxMenuNavigatorTest::aRefusedPickDoesNotFallThroughToAnotherAnswer()
{
    QFETCH(QString, title);
    QFETCH(QStringList, entries);
    QFETCH(bool, ownStairs);
    QFETCH(int, attempted);

    NavigatorRig rig;
    rig.settings.useAircraftStairs = ownStairs;
    rig.client.refusing = true;

    std::vector<std::string> shown;
    for (const QString& entry : entries)
    {
        shown.push_back(entry.toStdString());
    }

    ShowMenu(rig.state, title.toStdString(), shown);
    rig.nav.OnMenuChanged();

    QCOMPARE(static_cast<int>(rig.client.refused.size()), 1);
    QCOMPARE(rig.client.refused.front().args.value("index").toInt(-1), attempted);

    rig.now += 1500;
    rig.nav.OnMenuChanged();

    const bool onlyTheFirstAnswer = std::ranges::all_of(rig.client.refused, [attempted](const Sent& request)
    {
        return request.verb != "menu.pick" || request.args.value("index").toInt(-1) == attempted;
    });

    QCOMPARE(static_cast<int>(rig.client.refused.size()), 2);
    QVERIFY(onlyTheFirstAnswer);

    rig.client.refusing = false;
    rig.now += 1500;
    rig.nav.OnMenuChanged();

    QCOMPARE(rig.client.Count("menu.pick"), 1);
    QCOMPARE(rig.client.Last("menu.pick")->args.value("index").toInt(-1), attempted);
}

void GsxMenuNavigatorTest::aRefusedPickIsNotLoggedAsAMenuNobodyMatched_data()
{
    QTest::addColumn<bool>("repositioning");
    QTest::addColumn<QString>("title");
    QTest::addColumn<QStringList>("entries");

    QTest::newRow("the crew question")
        << false << QStringLiteral("Do you want to board crew?") << QStringList{"No", "Crew", "Pilots", "Both"};
    QTest::newRow("the reposition root")
        << true << QStringLiteral("Ground services") << QStringList{"Reposition Aircraft", "Back"};
}

void GsxMenuNavigatorTest::aRefusedPickIsNotLoggedAsAMenuNobodyMatched()
{
    QFETCH(bool, repositioning);
    QFETCH(QString, title);
    QFETCH(QStringList, entries);

    NavigatorRig rig;

    if (repositioning)
    {
        rig.nav.RepositionAircraft();
    }
    else
    {
        rig.nav.RequestCatering();
    }

    rig.client.refusing = true;

    std::vector<std::string> shown;
    for (const QString& entry : entries)
    {
        shown.push_back(entry.toStdString());
    }

    ShowMenu(rig.state, title.toStdString(), shown);
    rig.nav.OnMenuChanged();

    QCOMPARE(rig.client.Refused("menu.pick"), 1);
    QVERIFY(!Logged(rig.logger, "unmatched by intent"));
}

void GsxMenuNavigatorTest::aRefusedConfirmEnginesDoesNotKeepItsIntentAlive()
{
    NavigatorRig rig;

    QVERIFY(!rig.nav.ConfirmGoodEngines());

    rig.client.refusing = true;
    ShowMenu(rig.state, "Confirm good engine start", {"Confirm good engine start", "Cancel"});

    for (int pass = 0; pass < 65; ++pass)
    {
        rig.now += 1000;
        rig.nav.OnMenuChanged();
    }

    QCOMPARE(MessagesWith(rig.logger, "confirm-engines intent expired"), 1);

    rig.client.refusing = false;
    ShowMenu(rig.state, kTugTitle, {"Yes", "No"});
    rig.now += 1000;
    rig.nav.OnMenuChanged();

    QCOMPARE(rig.client.Count("menu.pick"), 0);
}

void GsxMenuNavigatorTest::aRequestNeverSentIsDroppedAfterTheGiveUpWindow_data()
{
    QTest::addColumn<QString>("serviceId");

    QTest::newRow("gpu") << QStringLiteral("GPU");
    QTest::newRow("stairs") << QStringLiteral("OperateStairs");
    QTest::newRow("catering") << QStringLiteral("Catering");
}

void GsxMenuNavigatorTest::aRequestNeverSentIsDroppedAfterTheGiveUpWindow()
{
    QFETCH(QString, serviceId);

    NavigatorRig rig;
    rig.client.refusing = true;
    OfferService(rig.state, serviceId.toStdString());

    if (serviceId == "Catering")
    {
        rig.nav.RequestCatering();
    }
    else
    {
        RequestToggling(rig.nav, serviceId);
    }

    rig.now = 5000 + 89999;
    rig.nav.OnMenuChanged();

    QVERIFY(!Logged(rig.logger, "never left the client"));
    QCOMPARE(static_cast<int>(QueueOf(rig.nav).size()), 1);

    rig.now = 5000 + 90000;
    rig.nav.OnMenuChanged();

    QVERIFY(Logged(rig.logger, "never left the client"));
    QCOMPARE(static_cast<int>(QueueOf(rig.nav).size()), 0);

    rig.client.refusing = false;
    rig.now += 1500;
    rig.nav.OnMenuChanged();

    QCOMPARE(TriggersFor(rig.client, serviceId), 0);
}

void GsxMenuNavigatorTest::aRequestDeliveredLateStillOpensTheServiceIntent()
{
    NavigatorRig rig;
    rig.client.refusing = true;
    OfferService(rig.state, "Departure");

    rig.nav.RequestPushback();

    QCOMPARE(rig.client.Refused(kServiceTrigger), 1);

    rig.client.refusing = false;
    rig.now = 5000 + 70000;
    rig.nav.OnMenuChanged();

    QCOMPARE(TriggersFor(rig.client, QStringLiteral("Departure")), 1);

    ShowMenu(rig.state, kTugTitle, {"Yes", "No"});
    rig.nav.OnMenuChanged();

    const Sent* pick = rig.client.Last("menu.pick");

    QVERIFY(pick != nullptr);
    QCOMPARE(pick->args.value("index").toInt(-1), 1);
}

void GsxMenuNavigatorTest::aRefusedCloseKeepsTheMenuTracking()
{
    NavigatorRig rig;
    rig.client.refusing = true;
    ShowMenu(rig.state, "Some menu", {"Option"});

    rig.nav.RequestCatering();

    for (int pass = 0; pass < 4; ++pass)
    {
        rig.now += 1500;
        rig.nav.OnMenuChanged();
    }

    QCOMPARE(MessagesWith(rig.logger, "RemoteAPI menu:"), 1);
}

void GsxMenuNavigatorTest::aRefusedCloseOfAStuckMenuIsNotRecordedAsDone()
{
    NavigatorRig rig;
    OfferService(rig.state, "Boarding");

    rig.nav.RequestBoarding();
    MarkServiceTaken(rig.state, "Boarding");
    rig.nav.OnMenuChanged();

    ShowMenu(rig.state, "Service in progress", {"Complete now", "Abort service", "Back"});
    rig.nav.OnMenuChanged();

    for (int resync = 0; resync < 3; ++resync)
    {
        rig.now += 2000;
        rig.nav.OnMenuChanged();
    }

    rig.client.refusing = true;
    rig.now += 2000;
    rig.nav.OnMenuChanged();

    QCOMPARE(rig.client.Refused("menu.close"), 1);
    QVERIFY(!Logged(rig.logger, "the resyncs could not move"));

    rig.client.refusing = false;
    rig.now += 2000;
    rig.nav.OnMenuChanged();

    QCOMPARE(rig.client.Count("menu.close"), 1);
    QVERIFY(Logged(rig.logger, "the resyncs could not move"));
}

void GsxMenuNavigatorTest::aRefusedCloseOfAStaleMenuDoesNotSpendTheSettleWindow()
{
    NavigatorRig rig;
    rig.now = 0;

    rig.nav.RepositionAircraft();
    ShowMenu(rig.state, kPositionTitle, {"Reposition here [Gate 1]", "Cancel"});
    rig.nav.OnMenuChanged();

    rig.state.menu.shown = false;
    rig.nav.OnMenuChanged();

    rig.now = 5000;
    ShowMenu(rig.state, kPositionTitle, {"Reposition here [Gate 1]", "Cancel"});
    rig.nav.OnMenuChanged();

    rig.client.refusing = true;
    rig.now = 7000;
    rig.nav.OnMenuChanged();

    QCOMPARE(rig.client.Refused("menu.close"), 1);
    QVERIFY(rig.nav.IsMenuSettled());
    QVERIFY(!Logged(rig.logger, "closing stale menu"));

    rig.client.refusing = false;
    rig.now = 8600;
    rig.nav.OnMenuChanged();

    QCOMPARE(rig.client.Count("menu.close"), 1);
    QVERIFY(Logged(rig.logger, "closing stale menu"));
}

void GsxMenuNavigatorTest::aRefusedToggleDoesNotSpendTheSettleWindow()
{
    NavigatorRig rig;
    rig.client.refusing = true;

    rig.nav.RepositionAircraft();

    QCOMPARE(rig.client.Refused("menu.toggle"), 1);
    QVERIFY(rig.nav.IsMenuSettled());
}

void GsxMenuNavigatorTest::aRefusedDisableGsxMenuKeepsTheIntent()
{
    const auto tugPicks = [](const bool refuseTheClose)
    {
        NavigatorRig rig;
        OfferService(rig.state, "Departure");
        rig.nav.RequestPushback();

        rig.client.refusing = refuseTheClose;
        rig.nav.DisableGsxMenu();
        rig.client.refusing = false;

        ShowMenu(rig.state, kTugTitle, {"Yes", "No"});
        rig.now += 100;
        rig.nav.OnMenuChanged();

        return rig.client.Count("menu.pick");
    };

    QCOMPARE(tugPicks(false), 0);
    QCOMPARE(tugPicks(true), 1);
}

void GsxMenuNavigatorTest::theToolbarIsNotOpenedBeforeTheBridgeReportsItsState()
{
    PanelRig rig(GsxPanelMode::OnPushback);
    GsxMenuNavigator nav(&rig.client, &rig.state, &rig.settings, &rig.logger, &rig.plugin);

    nav.OpenPushbackPanel();

    QCOMPARE(rig.PanelCommands(), 0);
    QVERIFY(!nav.TakeMemory().Flag("panelOpenSpent", true));
    QVERIFY(!nav.TakeMemory().Flag("panelOpenedByUs", true));

    rig.PanelIs("closed");
    nav.OpenPushbackPanel();

    QCOMPARE(rig.PanelCommands(), 1);
    QCOMPARE(rig.LastPanelPayload(), QString(IntegratorPluginCommBus::kCommandOpen));
}

void GsxMenuNavigatorTest::theToolbarIsNotClosedBeforeTheBridgeReportsItsState()
{
    PanelRig before(GsxPanelMode::OnPushback);
    before.PanelIs("closed");
    GsxMenuNavigator previous(&before.client, &before.state, &before.settings, &before.logger, &before.plugin);
    previous.OpenPushbackPanel();

    PanelRig after(GsxPanelMode::OnPushback);
    GsxMenuNavigator resumed(&after.client, &after.state, &after.settings, &after.logger, &after.plugin);
    resumed.RestoreMemory(previous.TakeMemory());
    resumed.OnPushbackStarted();

    QCOMPARE(after.PanelCommands(), 0);

    after.PanelIs("open");
    resumed.OnPushbackStarted();

    QCOMPARE(after.PanelCommands(), 1);
    QCOMPARE(after.LastPanelPayload(), QString(IntegratorPluginCommBus::kCommandClose));
}

QTEST_GUILESS_MAIN(GsxMenuNavigatorTest)

#include "tst_gsx_menu_navigator.moc"
