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

    // Captura cada comando enviado, sem abrir socket. SendCommand e' virtual no client real.
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
    static void triggerServiceOpensMenuWhenClosed();
    static void triggerServiceDoesNotToggleOpenMenu();
    static void confirmGoodEnginesPicksWhenMenuVisible();
    static void confirmGoodEnginesOpensMenuAndDefersPick();
    static void picksGsxChoiceDuringServiceIntent();
    static void gsxChoiceSurvivesDispatchDelay();
    static void gsxChoiceSurvivesTransientMenuClose();
    static void resolverDoesNotRepickSameMenu();
    static void gsxChoicePickedEvenAfterIntentTtl();
    static void gsxChoiceNotPickedWhenFlagOff();
    static void manualMenuWithGsxChoiceIsPicked();
    static void manualMenuIsNotRepickedWhileUnchanged();
    static void manualMenuWithoutGsxChoiceIsIgnored();
    static void skipsDisabledEntryAndPicksEnabled();
    static void repositionWalksRootThenSubmenu();
    static void repositionSurvivesTransientCloseAndRootReshow();
    static void rejectedPickAllowsRepick();
    static void resetAllowsRepickingSameMenu();
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

void GsxMenuNavigatorTest::activeGsxIconIsNotReactivated()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    AutomationSettings settings;
    FakeDomainLogger logger;
    FakeVariableGateway gateway;
    gateway.lvars[IntegratorPluginCommBus::kGsxToolbarActiveLVar] = 1.0;
    CommBusPluginClient plugin(&gateway);
    GsxMenuNavigator nav(&client, &state, &settings, &logger, &plugin);

    QVERIFY(nav.RequestRefueling());

    QCOMPARE(gateway.Written(IntegratorPluginCommBus::kToolbarCmdLVar), -1.0);
    QCOMPARE(client.Count("service.trigger"), 1);
}

void GsxMenuNavigatorTest::triggerServiceOpensMenuWhenClosed()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    QVERIFY(nav.RequestRefueling());

    QCOMPARE(client.sent.size(), std::size_t(2));
    QCOMPARE(client.sent[0].verb, QString("menu.toggle"));
    QCOMPARE(client.sent[1].verb, QString("service.trigger"));
}

void GsxMenuNavigatorTest::triggerServiceDoesNotToggleOpenMenu()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    AutomationSettings settings;
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
    AutomationSettings settings;
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
    AutomationSettings settings;
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

void GsxMenuNavigatorTest::picksGsxChoiceDuringServiceIntent()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    AutomationSettings settings; // autoSelectGsxChoice == true por padrao
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    QVERIFY(nav.RequestRefueling()); // abre a intencao de servico
    // O operador recomendado vem marcado por "[GSX choice]" na propria entrada (formato real do GSX).
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
    AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    long long fakeNow = 0;
    nav.SetClockForTest([&fakeNow] { return fakeNow; });

    QVERIFY(nav.RequestRefueling()); // intenção aberta em t=0
    // Captura real: o menu de operador surge ~15 s após o service.trigger. Regressão anterior:
    // TTL de 8 s expirava antes e o auto-select não disparava.
    fakeNow = 30000;
    ShowMenu(state, "Select fueltruck operator", {"Air North [GSX choice]", "Strategic Aviation Canada"});
    nav.OnMenuChanged();

    const Sent* pick = client.Last("menu.pick");
    QVERIFY(pick != nullptr);
    QCOMPARE(pick->args.value("index").toInt(), 0);
}

void GsxMenuNavigatorTest::gsxChoiceSurvivesTransientMenuClose()
{
    // Regressão real: durante o service.trigger o menu abre, fecha por um instante e reabre no
    // submenu de operador. Fechar a intenção nesse fechamento transitório derrubava o auto-select.
    FakeRemoteClient client;
    GsxRemoteState state;
    AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    QVERIFY(nav.RequestRefueling());

    // 1) menu principal aberto pelo service.trigger (nada a responder aqui)
    ShowMenu(state, "Activate Services at CYVR/Vancouver Intl", {"Request Refueling", "Request Boarding"});
    nav.OnMenuChanged();
    QCOMPARE(client.Count("menu.pick"), 0);

    // 2) fechamento transitório entre o pick do serviço e o submenu de operador
    state.menu.shown = false;
    state.menu.title.clear();
    state.menu.entries.clear();
    nav.OnMenuChanged();

    // 3) submenu de operador aparece -> ainda deve auto-selecionar o operador marcado com [GSX choice]
    ShowMenu(state, "Select fueltruck operator", {"Air North [GSX choice]", "Strategic Aviation Canada"});
    nav.OnMenuChanged();

    const Sent* pick = client.Last("menu.pick");
    QVERIFY(pick != nullptr);
    QCOMPARE(pick->args.value("index").toInt(), 0);
}

void GsxMenuNavigatorTest::resolverDoesNotRepickSameMenu()
{
    // O resolvedor roda a cada tick; enquanto o GSX não trocar o menu, não deve reenviar o pick.
    FakeRemoteClient client;
    GsxRemoteState state;
    AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    QVERIFY(nav.RequestRefueling());
    ShowMenu(state, "Select fueltruck operator", {"Air North [GSX choice]", "Strategic Aviation Canada"});

    nav.OnMenuChanged(); // tick 1 -> pica
    nav.OnMenuChanged(); // tick 2 -> mesmo menu, não repica
    nav.OnMenuChanged(); // tick 3 -> idem

    QCOMPARE(client.Count("menu.pick"), 1);
}

void GsxMenuNavigatorTest::gsxChoicePickedEvenAfterIntentTtl()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    long long fakeNow = 0;
    nav.SetClockForTest([&fakeNow] { return fakeNow; });

    QVERIFY(nav.RequestRefueling());
    fakeNow = 120000; // intencao expirada -> o pick global de "GSX choice" nao depende dela
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

void GsxMenuNavigatorTest::manualMenuWithGsxChoiceIsPicked()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    // Nenhum comando nosso antes: o menu foi aberto pelo usuario / Stream Deck.
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
    AutomationSettings settings;
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
    AutomationSettings settings;
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
    AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    QVERIFY(nav.RequestBoarding());
    // O primeiro operador marcado esta' desabilitado (cinza); deve escolher o segundo marcado.
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
    AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    QVERIFY(nav.RepositionAircraft());
    QVERIFY(client.Last("menu.toggle") != nullptr); // menu estava fechado -> abre

    // Menu principal: escolher "Reposition Aircraft" (indice 1).
    ShowMenu(state, "Activate Services at KJFK/JFK",
             {"Request Deboarding", "Reposition Aircraft", "Operate Stairs"});
    nav.OnMenuChanged();
    const Sent* rootPick = client.Last("menu.pick");
    QVERIFY(rootPick != nullptr);
    QCOMPARE(rootPick->args.value("index").toInt(), 1);

    // Submenu de posicao: escolher "Reposition here" (indice 0).
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
    AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    QVERIFY(nav.RepositionAircraft());

    ShowMenu(state, "Activate Services at CYVR/Vancouver Intl",
             {"Request Refueling", "Reposition Aircraft"});
    nav.OnMenuChanged();
    QCOMPARE(client.Count("menu.pick"), 1);

    // Fechamento transitório entre o pick do root e o submenu (comportamento real do GSX).
    state.menu.shown = false;
    state.menu.title.clear();
    state.menu.entries.clear();
    nav.OnMenuChanged();

    // O GSX reapresenta o menu root em vez do submenu: o walk deve re-pickar, não estagnar.
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
    AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    QVERIFY(nav.RequestRefueling());
    ShowMenu(state, "Select fueltruck operator", {"Air North [GSX choice]", "Strategic Aviation Canada"});

    nav.OnMenuChanged();
    QCOMPARE(client.Count("menu.pick"), 1);

    // Mesmo menu num tick seguinte: o de-dup impede o re-pick.
    nav.OnMenuChanged();
    QCOMPARE(client.Count("menu.pick"), 1);

    // O GSX recusa o pick; o navegador deve poder re-tentar o mesmo menu aparente.
    client.EmitRejection("not_available");
    nav.OnMenuChanged();
    QCOMPARE(client.Count("menu.pick"), 2);
}

void GsxMenuNavigatorTest::resetAllowsRepickingSameMenu()
{
    FakeRemoteClient client;
    GsxRemoteState state;
    AutomationSettings settings;
    FakeDomainLogger logger;
    GsxMenuNavigator nav(&client, &state, &settings, &logger);

    QVERIFY(nav.RequestRefueling());
    ShowMenu(state, "Select fueltruck operator", {"Air North [GSX choice]", "Strategic Aviation Canada"});
    nav.OnMenuChanged();
    QCOMPARE(client.Count("menu.pick"), 1);

    // Reset limpa o historico de picks; o pick global de "GSX choice" reavalia o menu visivel.
    nav.Reset();
    nav.OnMenuChanged();
    QCOMPARE(client.Count("menu.pick"), 2);
}

QTEST_GUILESS_MAIN(GsxMenuNavigatorTest)

#include "tst_gsx_menu_navigator.moc"
