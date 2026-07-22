#include <QtTest/QTest>

#include "doubles/FakeCommBusBridgeGateway.h"
#include "../src/infrastructure/commbus/CommBusPluginClient.h"

using namespace IntegratorPluginCommBus;

class GsxPluginClientTest final : public QObject
{
    Q_OBJECT

private slots:
    static void reportsBridgeNotReadyByDefault();
    static void subscribesToolbarStateOnSetup();
    static void stateLatchFollowsSubscription();
    static void unavailableStateResetsLatch();
    static void openCommandSentWhenBridgeAvailable();
    static void openCommandSkippedWhenBridgeUnavailable();
    static void gsxToolbarActiveFollowsOpenState();
    static void shutdownUnsubscribesState();
};

void GsxPluginClientTest::reportsBridgeNotReadyByDefault()
{
    FakeCommBusBridgeGateway bridge;
    const CommBusPluginClient client(&bridge);

    QVERIFY(!client.IsBridgeReady());
}

void GsxPluginClientTest::subscribesToolbarStateOnSetup()
{
    FakeCommBusBridgeGateway bridge;
    CommBusPluginClient client(&bridge);

    client.Setup();

    QCOMPARE(bridge.subscribed.size(), static_cast<std::size_t>(1));
    QCOMPARE(QString::fromStdString(bridge.subscribed.front()), QString(kToolbarStateChannel));
}

void GsxPluginClientTest::stateLatchFollowsSubscription()
{
    FakeCommBusBridgeGateway bridge;
    CommBusPluginClient client(&bridge);

    client.Setup();
    bridge.Deliver(kToolbarStateChannel, "ready");

    QVERIFY(client.IsBridgeReady());
    QVERIFY(!client.IsGsxToolbarActive());

    bridge.Deliver(kToolbarStateChannel, "open");

    QVERIFY(client.IsBridgeReady());
    QVERIFY(client.IsGsxToolbarActive());
}

void GsxPluginClientTest::unavailableStateResetsLatch()
{
    FakeCommBusBridgeGateway bridge;
    CommBusPluginClient client(&bridge);

    client.Setup();
    bridge.Deliver(kToolbarStateChannel, "open");
    bridge.Deliver(kToolbarStateChannel, "unavailable");

    QVERIFY(!client.IsBridgeReady());
    QVERIFY(!client.IsGsxToolbarActive());
}

void GsxPluginClientTest::openCommandSentWhenBridgeAvailable()
{
    FakeCommBusBridgeGateway bridge;
    const CommBusPluginClient client(&bridge);

    QVERIFY(client.OpenGsxToolbar());
    QCOMPARE(bridge.CallCount(kToolbarCommandChannel), 1);

    const auto& [channel, flag, payload] = bridge.calls.front();
    QCOMPARE(QString::fromStdString(payload), QString(kCommandOpen));
    QCOMPARE(flag, CommBusFlag::kJs);
}

void GsxPluginClientTest::openCommandSkippedWhenBridgeUnavailable()
{
    FakeCommBusBridgeGateway bridge;
    bridge.available = false;
    const CommBusPluginClient client(&bridge);

    QVERIFY(!client.OpenGsxToolbar());
    QCOMPARE(bridge.CallCount(kToolbarCommandChannel), 0);
}

void GsxPluginClientTest::gsxToolbarActiveFollowsOpenState()
{
    FakeCommBusBridgeGateway bridge;
    CommBusPluginClient client(&bridge);

    client.Setup();

    QVERIFY(!client.IsGsxToolbarActive());

    bridge.Deliver(kToolbarStateChannel, "open");

    QVERIFY(client.IsGsxToolbarActive());

    bridge.Deliver(kToolbarStateChannel, "closed");

    QVERIFY(!client.IsGsxToolbarActive());
    QVERIFY(client.IsBridgeReady());
}

void GsxPluginClientTest::shutdownUnsubscribesState()
{
    FakeCommBusBridgeGateway bridge;
    CommBusPluginClient client(&bridge);

    client.Setup();
    client.Shutdown();

    QCOMPARE(bridge.unsubscribed.size(), static_cast<std::size_t>(1));
    QCOMPARE(QString::fromStdString(bridge.unsubscribed.front()), QString(kToolbarStateChannel));
}

QTEST_APPLESS_MAIN(GsxPluginClientTest)

#include "tst_gsx_plugin_client.moc"
