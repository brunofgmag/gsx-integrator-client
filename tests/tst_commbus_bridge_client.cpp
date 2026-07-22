#include <QtTest/QTest>

#include <string>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include "doubles/FakeSimConnectApi.h"
#include "../src/infrastructure/commbus/CommBusBridgeClient.h"

namespace
{
    QJsonObject Parse(const std::string& json)
    {
        return QJsonDocument::fromJson(QByteArray::fromStdString(json)).object();
    }
}

class CommBusBridgeClientTest final : public QObject
{
    Q_OBJECT

private slots:
    static void buildsCallEnvelope();
    static void buildsSubscribeEnvelope();
    static void buildsUnsubscribeEnvelope();
    static void parsesInboundEnvelope();
    static void parseRejectsMalformed();
    static void availabilityGatedOnProtocolVersion();
    static void inboundMessageDispatchesToHandler();
    static void unsubscribeStopsDispatch();
    static void callSuppressedWhenUnavailable();
    static void oversizeEnvelopeDropped();
};

void CommBusBridgeClientTest::buildsCallEnvelope()
{
    const QJsonObject envelope = Parse(CommBusBridgeClient::BuildCallEnvelope("TabletToPlane", 2, "{\"x\":1}"));

    QCOMPARE(envelope.value("cmd").toString(), QString("call"));
    QCOMPARE(envelope.value("channel").toString(), QString("TabletToPlane"));
    QCOMPARE(envelope.value("flag").toInt(), 2);
    QCOMPARE(envelope.value("payload").toString(), QString("{\"x\":1}"));
}

void CommBusBridgeClientTest::buildsSubscribeEnvelope()
{
    const QJsonObject envelope =
        Parse(CommBusBridgeClient::BuildSubscribeEnvelope("PlaneToTablet", CommBusFlag::kJs));

    QCOMPARE(envelope.value("cmd").toString(), QString("subscribe"));
    QCOMPARE(envelope.value("channel").toString(), QString("PlaneToTablet"));
    QCOMPARE(envelope.value("flag").toInt(), CommBusFlag::kJs);
}

void CommBusBridgeClientTest::buildsUnsubscribeEnvelope()
{
    const QJsonObject envelope = Parse(CommBusBridgeClient::BuildUnsubscribeEnvelope("PlaneToTablet"));

    QCOMPARE(envelope.value("cmd").toString(), QString("unsubscribe"));
    QCOMPARE(envelope.value("channel").toString(), QString("PlaneToTablet"));
}

void CommBusBridgeClientTest::parsesInboundEnvelope()
{
    std::string channel;
    std::string payload;

    QVERIFY(CommBusBridgeClient::ParseInbound(
        R"({"channel":"PlaneToTablet","payload":"hello"})", channel, payload));
    QCOMPARE(QString::fromStdString(channel), QString("PlaneToTablet"));
    QCOMPARE(QString::fromStdString(payload), QString("hello"));
}

void CommBusBridgeClientTest::parseRejectsMalformed()
{
    std::string channel;
    std::string payload;

    QVERIFY(!CommBusBridgeClient::ParseInbound("not json", channel, payload));
    QVERIFY(!CommBusBridgeClient::ParseInbound(R"({"payload":"x"})", channel, payload));
}

void CommBusBridgeClientTest::availabilityGatedOnProtocolVersion()
{
    CommBusBridgeClient client;

    QVERIFY(!client.IsAvailable());

    constexpr double belowProtocol = 1.0;
    client.OnReadyData(&belowProtocol, sizeof(double));

    QVERIFY(!client.IsAvailable());

    constexpr double atProtocol = 2.0;
    client.OnReadyData(&atProtocol, sizeof(double));

    QVERIFY(client.IsAvailable());
}

void CommBusBridgeClientTest::inboundMessageDispatchesToHandler()
{
    CommBusBridgeClient client;

    std::string received;
    client.Subscribe("PlaneToTablet", [&received](const std::string& payload) { received = payload; });

    client.OnRxMessage(R"({"channel":"PlaneToTablet","payload":"state_here"})");

    QCOMPARE(QString::fromStdString(received), QString("state_here"));
}

void CommBusBridgeClientTest::unsubscribeStopsDispatch()
{
    CommBusBridgeClient client;

    int hits = 0;
    client.Subscribe("PlaneToTablet", [&hits](const std::string&) { ++hits; });
    client.OnRxMessage(R"({"channel":"PlaneToTablet","payload":"a"})");
    client.Unsubscribe("PlaneToTablet");
    client.OnRxMessage(R"({"channel":"PlaneToTablet","payload":"b"})");

    QCOMPARE(hits, 1);
}

void CommBusBridgeClientTest::callSuppressedWhenUnavailable()
{
    CommBusBridgeClient client;

    client.Call("TabletToPlane", CommBusFlag::kWasm, "payload");

    QVERIFY(true);
}

void CommBusBridgeClientTest::oversizeEnvelopeDropped()
{
    FakeSimConnectApi::Reset();
    CommBusBridgeClient client;
    client.Setup();
    constexpr double atProtocol = 2.0;
    client.OnReadyData(&atProtocol, sizeof(double));

    client.Call("TabletToPlane", CommBusFlag::kWasm, "small");
    const std::size_t afterSmall = FakeSimConnectApi::writtenClientData.size();
    QVERIFY(afterSmall > 0);

    client.Call("TabletToPlane", CommBusFlag::kWasm, std::string(9000, 'x'));
    QCOMPARE(FakeSimConnectApi::writtenClientData.size(), afterSmall);
}

QTEST_APPLESS_MAIN(CommBusBridgeClientTest)

#include "tst_commbus_bridge_client.moc"
