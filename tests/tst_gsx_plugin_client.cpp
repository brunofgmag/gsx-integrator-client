#include <QtTest/QTest>

#include "TestDoubles.h"
#include "../src/infrastructure/commbus/CommBusPluginClient.h"

using namespace IntegratorPluginCommBus;

class GsxPluginClientTest final : public QObject
{
    Q_OBJECT

private slots:
    static void reportsBridgeNotReadyByDefault();
    static void reportsBridgeReadyWhenStateReady();
    static void reportsBridgeReadyWhenToolbarOpen();
    static void openCommandQueuedWhenBridgeNotReady();
    static void openCommandWrittenWhenBridgeReady();
    static void shutdownClearsCommand();
    static void unavailableStateIsNotReady();
    static void setupPreReadsToolbarState();
    static void gsxToolbarActiveFollowsLVar();
};

void GsxPluginClientTest::reportsBridgeNotReadyByDefault()
{
    FakeVariableGateway gateway;
    CommBusPluginClient client(&gateway);

    QVERIFY(!client.IsBridgeReady());
}

void GsxPluginClientTest::reportsBridgeReadyWhenStateReady()
{
    FakeVariableGateway gateway;
    CommBusPluginClient client(&gateway);

    gateway.lvars[kToolbarStateLVar] = static_cast<double>(ToolbarState::Ready);

    QVERIFY(client.IsBridgeReady());
}

void GsxPluginClientTest::reportsBridgeReadyWhenToolbarOpen()
{
    FakeVariableGateway gateway;
    CommBusPluginClient client(&gateway);

    gateway.lvars[kToolbarStateLVar] = static_cast<double>(ToolbarState::Open);

    QVERIFY(client.IsBridgeReady());
}

void GsxPluginClientTest::openCommandQueuedWhenBridgeNotReady()
{
    FakeVariableGateway gateway;
    CommBusPluginClient client(&gateway);

    QVERIFY(!client.OpenGsxToolbar());
    QCOMPARE(gateway.Written(kToolbarCmdLVar), static_cast<double>(ToolbarCmd::Open));
}

void GsxPluginClientTest::openCommandWrittenWhenBridgeReady()
{
    FakeVariableGateway gateway;
    CommBusPluginClient client(&gateway);

    gateway.lvars[kToolbarStateLVar] = static_cast<double>(ToolbarState::Ready);

    QVERIFY(client.OpenGsxToolbar());
    QCOMPARE(gateway.Written(kToolbarCmdLVar), static_cast<double>(ToolbarCmd::Open));
}

void GsxPluginClientTest::shutdownClearsCommand()
{
    FakeVariableGateway gateway;
    CommBusPluginClient client(&gateway);

    client.Shutdown();

    QCOMPARE(gateway.Written(kToolbarCmdLVar), static_cast<double>(ToolbarCmd::None));
}

void GsxPluginClientTest::unavailableStateIsNotReady()
{
    FakeVariableGateway gateway;
    CommBusPluginClient client(&gateway);

    gateway.lvars[kToolbarStateLVar] = static_cast<double>(ToolbarState::Unavailable);

    QVERIFY(!client.IsBridgeReady());
}

void GsxPluginClientTest::setupPreReadsToolbarState()
{
    FakeVariableGateway gateway;
    CommBusPluginClient client(&gateway);

    client.Setup();

    QVERIFY(gateway.lvars.find(kToolbarCmdLVar) == gateway.lvars.end());
}

void GsxPluginClientTest::gsxToolbarActiveFollowsLVar()
{
    FakeVariableGateway gateway;
    CommBusPluginClient client(&gateway);

    QVERIFY(!client.IsGsxToolbarActive());

    gateway.lvars[kGsxToolbarActiveLVar] = 1.0;

    QVERIFY(client.IsGsxToolbarActive());
}

QTEST_APPLESS_MAIN(GsxPluginClientTest)

#include "tst_gsx_plugin_client.moc"
