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
}

class GsxMenuNavigatorTest final : public QObject
{
    Q_OBJECT

private slots:
    static void serviceTriggersUseCanonicalVerbs();
    static void inactiveGsxIconIsActivatedBeforeMenuAction();
    static void activeGsxIconIsNotReactivated();
    static void openGsxOnRequestsOffSkipsToolbarButStillToggles();
    static void openGsxOnRequestsOnActivatesToolbar();
    static void triggerServiceOpensMenuWhenClosed();
    static void triggerServiceDoesNotToggleOpenMenu();
    static void confirmGoodEnginesPicksWhenMenuVisible();
    static void confirmGoodEnginesOpensMenuAndDefersPick();
    static void completePushbackPicksEntryOnInterruptPushbackMenu();
    static void deferredConfirmEnginesPicksEntryOnInterruptPushbackMenu();
    static void completeRefuelPicksCompleteNowViaServiceMenu();
    static void completeRefuelIntentExpiresAfterTtl();
    static void completeRefuelMatchesLbsLoadedEntry();
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
    static void rejectedPickAllowsRepick();
    static void resetAllowsRepickingSameMenu();
    static void staleRefuelingLevelMenuResyncsAndPicksBlockFuel();
    static void swallowedRepositionPickRetriesAfterResyncSnapshot();
    static void stalledMenuResyncIsBounded();
    static void lateResyncSnapshotDoesNotRepickAdvancedMenu();
    static void groundServiceTriggersUseCanonicalVerbs();
    static void menuSettlesAfterQuietPeriod();
    static void pendingResyncKeepsMenuUnsettled();
};

void GsxMenuNavigatorTest::serviceTriggersUseCanonicalVerbs()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    QVERIFY(nav.RequestRefueling());
    QVERIFY(nav.RequestBoarding());
    QVERIFY(nav.RequestDeboarding());
    QVERIFY(nav.RequestPushback());
    QVERIFY(nav.CallJetway());
    QVERIFY(nav.CallStairs());
    QVERIFY(nav.RequestSimbriefLoad());

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

void GsxMenuNavigatorTest::inactiveGsxIconIsActivatedBeforeMenuAction()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    FakeVariableGateway gateway;
    CommBusPluginClient plugin(&gateway);
    GsxMenuNavigator nav(&client, &state, &settings, &logger, &plugin);

    QVERIFY(nav.RequestRefueling());

    QCOMPARE(gateway.Written(IntegratorPluginCommBus::kToolbarCmdLVar),
             static_cast<double>(IntegratorPluginCommBus::ToolbarCmd::Open));
    QCOMPARE(client.Count("service.trigger"), 1);
}

void GsxMenuNavigatorTest::activeGsxIconIsNotReactivated()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    FakeVariableGateway gateway;
    gateway.lvars[IntegratorPluginCommBus::kGsxToolbarActiveLVar] = 1.0;
    CommBusPluginClient plugin(&gateway);
    GsxMenuNavigator nav(&client, &state, &settings, &logger, &plugin);

    QVERIFY(nav.RequestRefueling());

    QCOMPARE(gateway.Written(IntegratorPluginCommBus::kToolbarCmdLVar), -1.0);
    QCOMPARE(client.Count("service.trigger"), 1);
}

void GsxMenuNavigatorTest::openGsxOnRequestsOffSkipsToolbarButStillToggles()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    AutomationSettings settings;
    settings.openGsxOnRequests = false;
    FakeDomainLogger logger;
    FakeVariableGateway gateway;
    CommBusPluginClient plugin(&gateway);
    GsxMenuNavigator nav(&client, &state, &settings, &logger, &plugin);

    QVERIFY(nav.RequestRefueling());

    QCOMPARE(gateway.Written(IntegratorPluginCommBus::kToolbarCmdLVar), -1.0);
    QCOMPARE(client.Count("menu.toggle"), 1);
    QCOMPARE(client.Count("service.trigger"), 1);
}

void GsxMenuNavigatorTest::openGsxOnRequestsOnActivatesToolbar()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    AutomationSettings settings;
    FakeDomainLogger logger;
    FakeVariableGateway gateway;
    CommBusPluginClient plugin(&gateway);
    GsxMenuNavigator nav(&client, &state, &settings, &logger, &plugin);

    QVERIFY(nav.RequestRefueling());

    QCOMPARE(gateway.Written(IntegratorPluginCommBus::kToolbarCmdLVar),
             static_cast<double>(IntegratorPluginCommBus::ToolbarCmd::Open));
    QCOMPARE(client.Count("service.trigger"), 1);
}

void GsxMenuNavigatorTest::triggerServiceOpensMenuWhenClosed()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    QVERIFY(nav.RequestRefueling());

    QCOMPARE(client.sent.size(), static_cast<std::size_t>(2));
    QCOMPARE(client.sent[0].verb, QString("menu.toggle"));
    QCOMPARE(client.sent[1].verb, QString("service.trigger"));
}

void GsxMenuNavigatorTest::triggerServiceDoesNotToggleOpenMenu()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    ShowMenu(state, "Activate Services at CYVR/Vancouver Intl", {"Request Refueling"});

    QVERIFY(nav.RequestRefueling());

    QCOMPARE(client.Count("menu.toggle"), 0);
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

    QVERIFY(nav.CompleteRefuel());

    QCOMPARE(client.Count("menu.toggle"), 1);

    ShowMenu(state, "Activate Services at SBFZ",
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

void GsxMenuNavigatorTest::completeRefuelIntentExpiresAfterTtl()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    long long fakeNow = 0;
    nav.SetClockForTest([&fakeNow] { return fakeNow; });

    QVERIFY(nav.CompleteRefuel());

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

    QVERIFY(nav.CompleteRefuel());

    ShowMenu(state, "Activate Services at SBFZ",
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

    QVERIFY(nav.RepositionAircraft());
    QVERIFY(nav.RequestBoarding());

    ShowMenu(state, "Activate Services at SBFZ",
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

    QVERIFY(nav.RequestRefueling());

    ShowMenu(state, "Select fueltruck operator", {"Air North [GSX choice]", "Strategic Aviation Canada", "Back"});
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

    QVERIFY(nav.RequestRefueling());

    fakeNow = 30000;
    ShowMenu(state, "Select fueltruck operator", {"Air North [GSX choice]", "Strategic Aviation Canada"});
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

    QVERIFY(nav.RequestRefueling());

    ShowMenu(state, "Activate Services at CYVR/Vancouver Intl", {"Request Refueling", "Request Boarding"});
    nav.OnMenuChanged();

    QCOMPARE(client.Count("menu.pick"), 0);

    state.menu.shown = false;
    state.menu.title.clear();
    state.menu.entries.clear();
    nav.OnMenuChanged();

    ShowMenu(state, "Select fueltruck operator", {"Air North [GSX choice]", "Strategic Aviation Canada"});
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

    QVERIFY(nav.RequestRefueling());

    ShowMenu(state, "Select fueltruck operator", {"Air North [GSX choice]", "Strategic Aviation Canada"});

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

    QVERIFY(nav.RequestRefueling());

    fakeNow = 120000;
    ShowMenu(state, "Select fueltruck operator", {"Air North [GSX choice]", "Strategic Aviation Canada"});
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

    QVERIFY(nav.RequestRefueling());

    ShowMenu(state, "Select fueltruck operator", {"Air North [GSX choice]", "Strategic Aviation Canada"});
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

    QVERIFY(nav.RequestBoarding());

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
    settings.crewBoarding = CrewBoarding::Pilots;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    QVERIFY(nav.RequestBoarding());

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
    settings.crewBoarding = CrewBoarding::Nobody;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    QVERIFY(nav.RequestBoarding());

    ShowMenu(state, "Do you want to board crew?", {"Nobody", "Crew", "Pilots", "Both"});
    nav.OnMenuChanged();

    const Sent* boardPick = client.Last("menu.pick");

    QVERIFY(boardPick != nullptr);
    QCOMPARE(boardPick->args.value("index").toInt(), 0);

    QVERIFY(nav.RequestDeboarding());

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
    settings.crewBoarding = CrewBoarding::Crew;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    long long fakeNow = 0;
    nav.SetClockForTest([&fakeNow] { return fakeNow; });

    QVERIFY(nav.RequestDeboarding());

    fakeNow = 90000;
    ShowMenu(state, "Do you want to deboard crew?", {"No", "Crew", "Pilots", "Both"});
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

    QVERIFY(nav.RequestRefueling());
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

    QVERIFY(nav.RequestRefueling());

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

    ShowMenu(state, "Select fueltruck operator", {"Air North [GSX choice]", "Strategic Aviation Canada"});
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

    ShowMenu(state, "Select fueltruck operator", {"Air North [GSX choice]", "Strategic Aviation Canada"});

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

    ShowMenu(state, "Activate Services at CYVR/Vancouver Intl", {"Request Refueling", "Request Boarding"});
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

    QVERIFY(nav.RequestBoarding());

    ShowMenu(state, "Select handling operator",
             {"Air North [GSX choice]", "Menzies [GSX choice]"}, {true, false});
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

    QVERIFY(nav.RepositionAircraft());

    QVERIFY(client.Last("menu.toggle") != nullptr);

    ShowMenu(state, "Activate Services at KJFK/JFK",
             {"Request Deboarding", "Reposition Aircraft", "Operate Stairs"});
    nav.OnMenuChanged();
    const Sent* rootPick = client.Last("menu.pick");

    QVERIFY(rootPick != nullptr);

    QCOMPARE(rootPick->args.value("index").toInt(), 1);

    ShowMenu(state, "Select Position at KJFK", {"Reposition here", "Cancel"});
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

    QVERIFY(nav.RepositionAircraft());

    ShowMenu(state, "Activate Services at CYVR/Vancouver Intl",
             {"Request Refueling", "Reposition Aircraft"});
    nav.OnMenuChanged();

    QCOMPARE(client.Count("menu.pick"), 1);

    state.menu.shown = false;
    state.menu.title.clear();
    state.menu.entries.clear();
    nav.OnMenuChanged();

    ShowMenu(state, "Activate Services at CYVR/Vancouver Intl",
             {"Request Refueling", "Reposition Aircraft"});
    nav.OnMenuChanged();
    QCOMPARE(client.Count("menu.pick"), 2);

    ShowMenu(state, "Select Position at CYVR", {"Reposition here [Apron 6|Gate 70]", "Cancel"});
    nav.OnMenuChanged();

    const Sent* pick = client.Last("menu.pick");

    QVERIFY(pick != nullptr);

    QCOMPARE(pick->args.value("index").toInt(), 0);
    QCOMPARE(client.Count("menu.pick"), 3);
}

void GsxMenuNavigatorTest::rejectedPickAllowsRepick()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    constexpr AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    QVERIFY(nav.RequestRefueling());

    ShowMenu(state, "Select fueltruck operator", {"Air North [GSX choice]", "Strategic Aviation Canada"});

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

    QVERIFY(nav.RequestRefueling());

    ShowMenu(state, "Select fueltruck operator", {"Air North [GSX choice]", "Strategic Aviation Canada"});
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

    QVERIFY(nav.RequestRefueling());

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

    QVERIFY(nav.RepositionAircraft());

    ShowMenu(state, "Activate Services at SBFZ/Pinto Martins Intl",
             {"Request Refueling", "Reposition Aircraft"});
    nav.OnMenuChanged();

    QCOMPARE(client.Count("menu.pick"), 1);

    fakeNow = 2000;
    nav.OnMenuChanged();

    QCOMPARE(client.Count("state.get"), 1);
    QCOMPARE(client.Count("menu.pick"), 1);

    nav.OnSnapshot();

    QCOMPARE(client.Count("menu.pick"), 2);

    ShowMenu(state, "Select Position at SBFZ/Pinto Martins Intl", {"Reposition here [Gate 0]", "Cancel"});
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

    QVERIFY(nav.RequestRefueling());

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

    QVERIFY(navigator.ToggleGpu());
    QCOMPARE(client.Last("service.trigger")->args.value("service").toString(), QStringLiteral("GPU"));

    QVERIFY(navigator.RequestCatering());
    QCOMPARE(client.Last("service.trigger")->args.value("service").toString(), QStringLiteral("Catering"));

    QVERIFY(navigator.RequestLavatory());
    QCOMPARE(client.Last("service.trigger")->args.value("service").toString(), QStringLiteral("Lavatory"));

    QVERIFY(navigator.RequestWater());
    QCOMPARE(client.Last("service.trigger")->args.value("service").toString(), QStringLiteral("Water"));

    QVERIFY(navigator.RequestCleaning());
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

    QVERIFY(nav.RequestRefueling());
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

    QVERIFY(nav.RequestRefueling());

    ShowMenu(state, "Select refueling level", {"Request Refueling"});
    nav.OnMenuChanged();

    fakeNow = 2000;
    nav.OnMenuChanged();

    QCOMPARE(client.Count("state.get"), 1);

    fakeNow = 4000;

    QVERIFY(!nav.IsMenuSettled());
}

QTEST_GUILESS_MAIN(GsxMenuNavigatorTest)

#include "tst_gsx_menu_navigator.moc"
