#include <string>
#include <utility>
#include <vector>

#include <QJsonObject>
#include <QString>
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

        bool SendCommand(const QString& verb, const QJsonObject& args = {}) override
        {
            sent.push_back({verb, args});
            return true;
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
            int n = 0;
            for (const Sent& s : sent)
            {
                if (s.verb == verb)
                {
                    ++n;
                }
            }
            return n;
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
        state.services.push_back(GsxRemoteService{id, 1, true});
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

        state.services.push_back(GsxRemoteService{id, 5, false});
    }

    bool Logged(const FakeDomainLogger& logger, const std::string& needle)
    {
        for (const std::string& message : logger.messages)
        {
            if (message.find(needle) != std::string::npos)
            {
                return true;
            }
        }

        return false;
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
    static void onPushbackIgnoresAPanelLeftOpenOutsideTheWindow();
    static void onPushbackDoesNotReopenThePanelThePilotClosed();
    static void aDroppedOpenKeepsThePushbackOpenOwed();
    static void theTurnaroundTurnRearmsThePanel();
    static void triggerServiceDoesNotOpenClosedMenu();
    static void triggerServiceDoesNotToggleOpenMenu();
    static void confirmGoodEnginesPicksWhenMenuVisible();
    static void confirmGoodEnginesOpensMenuAndDefersPick();
    static void completePushbackPicksEntryOnInterruptPushbackMenu();
    static void deferredConfirmEnginesPicksEntryOnInterruptPushbackMenu();
    static void completeRefuelPicksCompleteNowViaServiceMenu();
    static void completeRefuelIntentExpiresAfterTtl();
    static void completeRefuelMatchesLbsLoadedEntry();
    static void completeBoardingPicksCompleteNowViaCargoEntry();
    static void completeBoardingMatchesThePassengerEntry();
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
    static void deIceMenuPicksYesWhenEnabled();
    static void deIceMenuDeclinedByDefault();
    static void picksSimbriefBlockFuelOnRefuelingLevelMenu();
    static void blockFuelNotPickedWhenFlagOff();
    static void manualMenuWithGsxChoiceIsPicked();
    static void manualMenuIsNotRepickedWhileUnchanged();
    static void manualMenuWithoutGsxChoiceIsIgnored();
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
    static void triggerStopsRetryingOnceGsxTakesIt();
    static void rearmedTriggerKeepsItsAttemptCount();
    static void rearmedTriggerIsDroppedOnceGsxTakesIt();
    static void stuckMenuIsClosedAfterResyncsAreExhausted();
    static void thePushbackDirectionMenuIsNeverDiscarded();
    static void resetAllowsClosingTheSameStuckMenuAgain();
    static void aRequestForAServiceAlreadyUnderwayIsDroppedBeforeSending();
    static void theGpuToggleStillFiresWhileTheServiceRuns();
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
    for (const Sent& s : client.sent)
    {
        if (s.verb == "service.trigger")
        {
            services.push_back(s.args.value("service").toString());
        }
        else if (s.verb == "command.run")
        {
            simbriefCommand = s.args.value("command").toString();
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
        fakeNow = i * 2000;
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

    fakeNow = 14999;
    nav.OnMenuChanged();

    QCOMPARE(client.Count("service.trigger"), 1);

    fakeNow = 15000;
    nav.OnMenuChanged();

    QCOMPARE(client.Count("service.trigger"), 2);

    fakeNow = 25000;
    nav.OnMenuChanged();

    QCOMPARE(client.Count("service.trigger"), 3);

    fakeNow = 35000;
    nav.OnMenuChanged();

    QCOMPARE(client.Count("service.trigger"), 3);
    QVERIFY(Logged(logger, "never taken by GSX"));
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

    fakeNow = 15000;
    nav.OnMenuChanged();

    QCOMPARE(client.Count("service.trigger"), 2);

    nav.RequestBoarding();

    QCOMPARE(client.Count("service.trigger"), 2);

    fakeNow = 25000;
    nav.OnMenuChanged();

    QCOMPARE(client.Count("service.trigger"), 3);

    fakeNow = 35000;
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

QTEST_GUILESS_MAIN(GsxMenuNavigatorTest)

#include "tst_gsx_menu_navigator.moc"
