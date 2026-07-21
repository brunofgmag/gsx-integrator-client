#include <QtTest/QTest>

#include <string>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include "doubles/FakeCommBusBridgeGateway.h"
#include "../src/infrastructure/pmdg/Pmdg777TabletClient.h"

namespace
{
    constexpr auto kChannelToPlane = "TabletToPlane";

    QJsonObject Parse(const std::string& json)
    {
        return QJsonDocument::fromJson(QByteArray::fromStdString(json)).object();
    }
}

class Pmdg777TabletClientTest final : public QObject
{
    Q_OBJECT

private slots:
    static void buildsWbPayloadEnvelope();
    static void buildsGroundConnEnvelope();
    static void availabilityFollowsBridge();
    static void sendsSuppressedWhenUnavailable();
    static void sendsFuelWhenAvailable();
    static void sendsGroundConnWhenAvailable();
    static void subscribesPlaneToTabletWithJsFlag();
    static void latchesEfbPlanImportOnFetchSuccess();
};

void Pmdg777TabletClientTest::buildsWbPayloadEnvelope()
{
    const QJsonObject envelope = Parse(Pmdg777TabletClient::BuildWbPayload("fuel_total_lbs", 120000));

    QCOMPARE(envelope.value("message_tag").toString(), QString("wb_payload"));
    QCOMPARE(envelope.value("tablet_side").toString(), QString("CA"));
    QCOMPARE(envelope.value("data").toObject().value("fuel_total_lbs").toInt(), 120000);
}

void Pmdg777TabletClientTest::buildsGroundConnEnvelope()
{
    const QJsonObject envelope = Parse(Pmdg777TabletClient::BuildGroundConn("wheel_chocks"));

    QCOMPARE(envelope.value("message_tag").toString(), QString("ground_conn"));
    QCOMPARE(envelope.value("data").toObject().value("wheel_chocks").toInt(), 1);
}

void Pmdg777TabletClientTest::availabilityFollowsBridge()
{
    FakeCommBusBridgeGateway bridge;
    Pmdg777TabletClient client(&bridge);

    QVERIFY(client.IsAvailable());

    bridge.available = false;

    QVERIFY(!client.IsAvailable());
}

void Pmdg777TabletClientTest::sendsSuppressedWhenUnavailable()
{
    FakeCommBusBridgeGateway bridge;
    bridge.available = false;
    Pmdg777TabletClient client(&bridge);
    client.Poll();

    client.SendFuelTotalLbs(100000);
    client.SendPaxTotal(200);
    client.RequestGroundConn("ground_power");

    QCOMPARE(bridge.CallCount(kChannelToPlane), 0);
}

void Pmdg777TabletClientTest::sendsFuelWhenAvailable()
{
    FakeCommBusBridgeGateway bridge;
    Pmdg777TabletClient client(&bridge);
    client.Poll();

    client.SendFuelTotalLbs(123456);

    QCOMPARE(bridge.CallCount(kChannelToPlane), 1);
    const auto& [channel, flag, payload] = bridge.calls.back();
    QCOMPARE(Parse(payload).value("data").toObject().value("fuel_total_lbs").toInt(), 123456);
    QCOMPARE(flag, CommBusFlag::kWasm);
}

void Pmdg777TabletClientTest::sendsGroundConnWhenAvailable()
{
    FakeCommBusBridgeGateway bridge;
    Pmdg777TabletClient client(&bridge);
    client.Poll();

    client.RequestGroundConn("wheel_chocks");

    QCOMPARE(bridge.CallCount(kChannelToPlane), 1);
    const auto& [channel, flag, payload] = bridge.calls.back();
    QCOMPARE(Parse(payload).value("message_tag").toString(), QString("ground_conn"));
    QCOMPARE(flag, CommBusFlag::kWasm);
}

void Pmdg777TabletClientTest::subscribesPlaneToTabletWithJsFlag()
{
    FakeCommBusBridgeGateway bridge;
    Pmdg777TabletClient client(&bridge);

    client.Poll();
    client.Poll();

    QCOMPARE(bridge.subscribed.size(), std::size_t(1));
    QCOMPARE(QString::fromStdString(bridge.subscribed.front()), QString("PlaneToTablet"));
    QCOMPARE(bridge.subscribedFlags.front(), CommBusFlag::kJs);
}

void Pmdg777TabletClientTest::latchesEfbPlanImportOnFetchSuccess()
{
    FakeCommBusBridgeGateway bridge;
    Pmdg777TabletClient client(&bridge);
    client.Poll();

    QVERIFY(!client.EfbPlanImported());

    bridge.Deliver("PlaneToTablet",
                   R"({"message_tag":"simbrief_fetch_result","data":{"result":"404"},"tablet_side":"CA"})");
    QVERIFY(!client.EfbPlanImported());

    bridge.Deliver("PlaneToTablet",
                   R"({"message_tag":"simbrief_fetch_result","data":{"result":200},"tablet_side":"CA"})");
    QVERIFY(client.EfbPlanImported());
}

QTEST_APPLESS_MAIN(Pmdg777TabletClientTest)

#include "tst_pmdg777_tablet_client.moc"
