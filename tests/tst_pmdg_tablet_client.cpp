#include <QtTest/QTest>

#include <string>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include "doubles/FakeCommBusBridgeGateway.h"
#include "../src/infrastructure/pmdg/PmdgTabletClient.h"

namespace
{
    constexpr auto kChannelToPlane = "TabletToPlane";

    constexpr auto kStateReply =
        R"({"message_tag":"state_reply","tablet_side":"FO","doors":{"abilities":{"close_all_enabled":true},)"
        R"("individual_doors":{"entry1_left":"CLOSE","entry1_right":"OPEN","entry2_left":"DISARM",)"
        R"("other_doors":{"fwd_cargo":"CLOSE","main_cargo":"OPEN"}}}})";

    QJsonObject Parse(const std::string& json)
    {
        return QJsonDocument::fromJson(QByteArray::fromStdString(json)).object();
    }
}

class PmdgTabletClientTest final : public QObject
{
    Q_OBJECT

private slots:
    static void buildsWbPayloadEnvelope();
    static void buildsGroundConnEnvelope();
    static void buildsGroundVehicleEnvelope();
    static void availabilityFollowsBridge();
    static void sendsSuppressedWhenUnavailable();
    static void sendsFuelWhenAvailable();
    static void sendsGroundConnWhenAvailable();
    static void subscribesPlaneToTabletWithJsFlag();
    static void latchesEfbPlanImportOnFetchSuccess();
    static void readsDoorStatesFromStateReply();
    static void doorInMotionHasNoState();
    static void requestStateAsksThePlaneForGroundState();
    static void unsubscribesBorrowedBridgeOnDestruction();
    static void skipsUnsubscribeWhenNeverPolled();
};

void PmdgTabletClientTest::buildsWbPayloadEnvelope()
{
    const QJsonObject envelope = Parse(PmdgTabletClient::BuildWbPayload("fuel_total_lbs", 120000));

    QCOMPARE(envelope.value("message_tag").toString(), QString("wb_payload"));
    QCOMPARE(envelope.value("tablet_side").toString(), QString("CA"));
    QCOMPARE(envelope.value("data").toObject().value("fuel_total_lbs").toInt(), 120000);
}

void PmdgTabletClientTest::buildsGroundConnEnvelope()
{
    const QJsonObject envelope = Parse(PmdgTabletClient::BuildGroundConn("wheel_chocks"));

    QCOMPARE(envelope.value("message_tag").toString(), QString("ground_conn"));
    QCOMPARE(envelope.value("data").toObject().value("wheel_chocks").toInt(), 1);
}

void PmdgTabletClientTest::buildsGroundVehicleEnvelope()
{
    const QJsonObject envelope = Parse(PmdgTabletClient::BuildGroundVehicle("stairs_1l"));

    QCOMPARE(envelope.value("message_tag").toString(), QString("ground_vehicles"));
    QCOMPARE(envelope.value("data").toObject().value("stairs_1l").toInt(), 1);
}

void PmdgTabletClientTest::availabilityFollowsBridge()
{
    FakeCommBusBridgeGateway bridge;
    const PmdgTabletClient client(&bridge);

    QVERIFY(client.IsAvailable());

    bridge.available = false;

    QVERIFY(!client.IsAvailable());
}

void PmdgTabletClientTest::sendsSuppressedWhenUnavailable()
{
    FakeCommBusBridgeGateway bridge;
    bridge.available = false;
    PmdgTabletClient client(&bridge);
    client.Poll();

    client.SendFuelTotalLbs(100000);
    client.SendPaxTotal(200);
    client.RequestGroundConn("ground_power");

    QCOMPARE(bridge.CallCount(kChannelToPlane), 0);
}

void PmdgTabletClientTest::sendsFuelWhenAvailable()
{
    FakeCommBusBridgeGateway bridge;
    PmdgTabletClient client(&bridge);
    client.Poll();

    client.SendFuelTotalLbs(123456);

    QCOMPARE(bridge.CallCount(kChannelToPlane), 1);
    const auto& [channel, flag, payload] = bridge.calls.back();
    QCOMPARE(Parse(payload).value("data").toObject().value("fuel_total_lbs").toInt(), 123456);
    QCOMPARE(flag, CommBusFlag::kWasm);
}

void PmdgTabletClientTest::sendsGroundConnWhenAvailable()
{
    FakeCommBusBridgeGateway bridge;
    PmdgTabletClient client(&bridge);
    client.Poll();

    client.RequestGroundConn("wheel_chocks");

    QCOMPARE(bridge.CallCount(kChannelToPlane), 1);
    const auto& [channel, flag, payload] = bridge.calls.back();
    QCOMPARE(Parse(payload).value("message_tag").toString(), QString("ground_conn"));
    QCOMPARE(flag, CommBusFlag::kWasm);
}

void PmdgTabletClientTest::subscribesPlaneToTabletWithJsFlag()
{
    FakeCommBusBridgeGateway bridge;
    PmdgTabletClient client(&bridge);

    client.Poll();
    client.Poll();

    QCOMPARE(bridge.subscribed.size(), static_cast<std::size_t>(1));
    QCOMPARE(QString::fromStdString(bridge.subscribed.front()), QString("PlaneToTablet"));
    QCOMPARE(bridge.subscribedFlags.front(), CommBusFlag::kJs);
}

void PmdgTabletClientTest::latchesEfbPlanImportOnFetchSuccess()
{
    FakeCommBusBridgeGateway bridge;
    PmdgTabletClient client(&bridge);
    client.Poll();

    QVERIFY(!client.EfbPlanImported());

    bridge.Deliver("PlaneToTablet",
                   R"({"message_tag":"simbrief_fetch_result","data":{"result":"404"},"tablet_side":"CA"})");
    QVERIFY(!client.EfbPlanImported());

    bridge.Deliver("PlaneToTablet",
                   R"({"message_tag":"simbrief_fetch_result","data":{"result":200},"tablet_side":"CA"})");
    QVERIFY(client.EfbPlanImported());
}

void PmdgTabletClientTest::readsDoorStatesFromStateReply()
{
    FakeCommBusBridgeGateway bridge;
    PmdgTabletClient client(&bridge);
    client.Poll();

    QVERIFY(!client.DoorOpen("entry1_left").has_value());

    bridge.Deliver("PlaneToTablet", kStateReply);

    QCOMPARE(client.DoorOpen("entry1_left"), std::optional(true));
    QCOMPARE(client.DoorOpen("entry1_right"), std::optional(false));
    QCOMPARE(client.DoorOpen("fwd_cargo"), std::optional(true));
    QCOMPARE(client.DoorOpen("main_cargo"), std::optional(false));
    QVERIFY(!client.DoorOpen("airstair").has_value());
}

void PmdgTabletClientTest::doorInMotionHasNoState()
{
    FakeCommBusBridgeGateway bridge;
    PmdgTabletClient client(&bridge);
    client.Poll();

    bridge.Deliver("PlaneToTablet",
                   R"({"message_tag":"state_reply","tablet_side":"FO","doors":{"individual_doors":)"
                   R"({"entry1_left":"OPENING","entry1_right":"CLOSE",)"
                   R"("other_doors":{"main_cargo":"CLOSING","fwd_cargo":"OPEN"}}}})");

    QVERIFY(!client.DoorOpen("entry1_left").has_value());
    QVERIFY(!client.DoorOpen("main_cargo").has_value());
    QCOMPARE(client.DoorOpen("entry1_right"), std::optional(true));
    QCOMPARE(client.DoorOpen("fwd_cargo"), std::optional(false));
}

void PmdgTabletClientTest::requestStateAsksThePlaneForGroundState()
{
    FakeCommBusBridgeGateway bridge;
    PmdgTabletClient client(&bridge);

    client.RequestState();

    QCOMPARE(bridge.calls.size(), static_cast<std::size_t>(1));

    const auto& [channel, flag, payload] = bridge.calls.front();

    QCOMPARE(QString::fromStdString(channel), QString(kChannelToPlane));
    QCOMPARE(flag, CommBusFlag::kWasm);

    const QJsonObject envelope = Parse(payload);

    QCOMPARE(envelope.value("message_tag").toString(), QString("query_state"));
    QCOMPARE(envelope.value("data").toObject().value("request").toString(), QString("yes"));
}

void PmdgTabletClientTest::unsubscribesBorrowedBridgeOnDestruction()
{
    FakeCommBusBridgeGateway bridge;
    {
        PmdgTabletClient client(&bridge);
        client.Poll();
    }

    QCOMPARE(bridge.unsubscribed.size(), static_cast<std::size_t>(1));
    QCOMPARE(QString::fromStdString(bridge.unsubscribed.front()), QString("PlaneToTablet"));
}

void PmdgTabletClientTest::skipsUnsubscribeWhenNeverPolled()
{
    FakeCommBusBridgeGateway bridge;
    {
        PmdgTabletClient client(&bridge);
    }

    QVERIFY(bridge.unsubscribed.empty());
}

QTEST_APPLESS_MAIN(PmdgTabletClientTest)

#include "tst_pmdg_tablet_client.moc"
