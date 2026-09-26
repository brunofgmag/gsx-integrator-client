#include <algorithm>

#include <QFile>
#include <QHostAddress>
#include <QJsonObject>
#include <QJsonValue>
#include <QSignalSpy>
#include <QStringList>
#include <QTcpServer>
#include <QTemporaryDir>
#include <QtTest/QTest>

#include "../src/infrastructure/gsx/GsxRemoteApiClient.h"
#include "../src/infrastructure/probe/ProbeLog.h"

namespace
{
    constexpr int kDeadlineMs = 30;
    constexpr auto kConnecting = "GSX RemoteAPI: connecting to";

    QStringList& Captured()
    {
        static QStringList messages;

        return messages;
    }

    QtMessageHandler& Chained()
    {
        static QtMessageHandler handler = nullptr;

        return handler;
    }

    void Capture(const QtMsgType type, const QMessageLogContext& context, const QString& message)
    {
        Captured().append(message);
        Chained()(type, context, message);
    }

    qsizetype Announcements()
    {
        return std::ranges::count_if(Captured(), [](const QString& message)
        {
            return message.contains(QLatin1String(kConnecting));
        });
    }

    QStringList WireLines()
    {
        QFile file(probe::RunLocation() + QLatin1Char('/') + probe::detail::ChannelFileName(probe::Channel::Wire));
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            return {};
        }

        return QString::fromUtf8(file.readAll()).split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    }

    void Reconnect(GsxRemoteApiClient& client)
    {
        QMetaObject::invokeMethod(&client, "OnReconnect", Qt::DirectConnection);
    }

    void Deliver(GsxRemoteApiClient& client, const QString& text)
    {
        QMetaObject::invokeMethod(&client, "OnTextMessage", Qt::DirectConnection, Q_ARG(QString, text));
    }

    void Connect(GsxRemoteApiClient& client)
    {
        QMetaObject::invokeMethod(&client, "OnConnected", Qt::DirectConnection);
    }

    void Disconnect(GsxRemoteApiClient& client)
    {
        QMetaObject::invokeMethod(&client, "OnDisconnected", Qt::DirectConnection);
    }
}

class GsxRemoteApiClientTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    static void init();
    static void cleanup();

    static void snapshotMessageEmitsSnapshotReceived();
    static void patchMessageEmitsPathAndValue();
    static void successResultEmitsOkWithEmptyCode();
    static void errorResultEmitsErrorCode();
    static void malformedJsonEmitsNothing();
    static void nonObjectJsonEmitsNothing();
    static void unknownTypeEmitsNothing();
    static void helloWithOtherProtocolEmitsNothing();
    static void commandIsDroppedWhileOffline();
    static void silentSocketIsDroppedAfterTheHandshakeDeadline();
    static void firstFrameCancelsTheHandshakeDeadline();
    static void theFirstFrameReportsTheConnection();
    static void aDropAfterAnAnswerReportsTheConnectionLost();
    static void aSocketThatNeverAnswersReportsNoConnection();
    static void retriesAfterTheFirstAttemptStayQuiet();
    static void anAnsweredConnectionAnnouncesTheNextAttempt();
    static void aConnectionGsxNeverAnsweredKeepsTheRetriesQuiet();
    static void everySentMessageLandsInTheWireLog();

private:
    QTemporaryDir probeDirectory_;
};

void GsxRemoteApiClientTest::initTestCase()
{
    QVERIFY(probeDirectory_.isValid());
    qputenv("GSXI_PROBE_DIR", probeDirectory_.path().toUtf8());
}

void GsxRemoteApiClientTest::init()
{
    Captured().clear();
    Chained() = qInstallMessageHandler(Capture);
}

void GsxRemoteApiClientTest::cleanup()
{
    qInstallMessageHandler(Chained());
    probe::SetEnabled(false);
#ifndef NDEBUG
    probe::ResetForTest();
#endif
}

void GsxRemoteApiClientTest::snapshotMessageEmitsSnapshotReceived()
{
    GsxRemoteApiClient client;
    const QSignalSpy spy(&client, &GsxRemoteApiClient::SnapshotReceived);

    Deliver(client, QStringLiteral(R"({"type":"snapshot","state":{"menu":{"shown":true}}})"));

    QCOMPARE(spy.count(), 1);

    const QJsonObject msg = spy.at(0).at(0).toJsonObject();

    QCOMPARE(msg.value("type").toString(), QStringLiteral("snapshot"));
    QVERIFY(msg.contains("state"));
}

void GsxRemoteApiClientTest::patchMessageEmitsPathAndValue()
{
    GsxRemoteApiClient client;
    const QSignalSpy spy(&client, &GsxRemoteApiClient::PatchReceived);

    Deliver(client, QStringLiteral(R"({"type":"patch","path":"menu/shown","value":true})"));

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toString(), QStringLiteral("menu/shown"));
    QCOMPARE(spy.at(0).at(1).toJsonValue(), QJsonValue(true));
}

void GsxRemoteApiClientTest::successResultEmitsOkWithEmptyCode()
{
    GsxRemoteApiClient client;
    const QSignalSpy spy(&client, &GsxRemoteApiClient::ResultReceived);

    Deliver(client, QStringLiteral(R"({"type":"result","ok":true})"));

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toBool(), true);
    QVERIFY(spy.at(0).at(1).toString().isEmpty());
}

void GsxRemoteApiClientTest::errorResultEmitsErrorCode()
{
    GsxRemoteApiClient client;
    const QSignalSpy spy(&client, &GsxRemoteApiClient::ResultReceived);

    Deliver(client,
            QStringLiteral(R"({"type":"result","ok":false,"error":{"code":"EBUSY","message":"menu busy"}})"));

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toBool(), false);
    QCOMPARE(spy.at(0).at(1).toString(), QStringLiteral("EBUSY"));
}

void GsxRemoteApiClientTest::malformedJsonEmitsNothing()
{
    GsxRemoteApiClient client;
    QSignalSpy snapshots(&client, &GsxRemoteApiClient::SnapshotReceived);
    QSignalSpy patches(&client, &GsxRemoteApiClient::PatchReceived);
    QSignalSpy results(&client, &GsxRemoteApiClient::ResultReceived);

    Deliver(client, QStringLiteral("{not valid json"));

    QCOMPARE(snapshots.count(), 0);
    QCOMPARE(patches.count(), 0);
    QCOMPARE(results.count(), 0);
}

void GsxRemoteApiClientTest::nonObjectJsonEmitsNothing()
{
    GsxRemoteApiClient client;
    QSignalSpy snapshots(&client, &GsxRemoteApiClient::SnapshotReceived);
    QSignalSpy patches(&client, &GsxRemoteApiClient::PatchReceived);
    QSignalSpy results(&client, &GsxRemoteApiClient::ResultReceived);

    Deliver(client, QStringLiteral("[1,2,3]"));

    QCOMPARE(snapshots.count(), 0);
    QCOMPARE(patches.count(), 0);
    QCOMPARE(results.count(), 0);
}

void GsxRemoteApiClientTest::unknownTypeEmitsNothing()
{
    GsxRemoteApiClient client;
    QSignalSpy snapshots(&client, &GsxRemoteApiClient::SnapshotReceived);
    QSignalSpy patches(&client, &GsxRemoteApiClient::PatchReceived);
    QSignalSpy results(&client, &GsxRemoteApiClient::ResultReceived);

    Deliver(client, QStringLiteral(R"({"type":"toast","text":"hi"})"));

    QCOMPARE(snapshots.count(), 0);
    QCOMPARE(patches.count(), 0);
    QCOMPARE(results.count(), 0);
}

void GsxRemoteApiClientTest::helloWithOtherProtocolEmitsNothing()
{
    GsxRemoteApiClient client;
    QSignalSpy snapshots(&client, &GsxRemoteApiClient::SnapshotReceived);
    QSignalSpy patches(&client, &GsxRemoteApiClient::PatchReceived);
    QSignalSpy results(&client, &GsxRemoteApiClient::ResultReceived);

    Deliver(client, QStringLiteral(R"({"type":"hello","protocol":99})"));

    QCOMPARE(snapshots.count(), 0);
    QCOMPARE(patches.count(), 0);
    QCOMPARE(results.count(), 0);
}

void GsxRemoteApiClientTest::commandIsDroppedWhileOffline()
{
    GsxRemoteApiClient client;

    QVERIFY(!client.SendCommand(QStringLiteral("menu.toggle")));
}

void GsxRemoteApiClientTest::silentSocketIsDroppedAfterTheHandshakeDeadline()
{
    GsxRemoteApiClient client;
    client.SetHandshakeTimeoutForTest(kDeadlineMs);

    Connect(client);

    QVERIFY(client.SendCommand(QStringLiteral("menu.toggle")));

    QTest::qWait(kDeadlineMs * 4);

    QVERIFY(!client.SendCommand(QStringLiteral("menu.toggle")));
}

void GsxRemoteApiClientTest::firstFrameCancelsTheHandshakeDeadline()
{
    GsxRemoteApiClient client;
    client.SetHandshakeTimeoutForTest(kDeadlineMs);

    Connect(client);
    Deliver(client, QStringLiteral(R"({"type":"hello","protocol":1})"));

    QTest::qWait(kDeadlineMs * 4);

    QVERIFY(client.SendCommand(QStringLiteral("menu.toggle")));
}

void GsxRemoteApiClientTest::theFirstFrameReportsTheConnection()
{
    GsxRemoteApiClient client;
    const QSignalSpy spy(&client, &GsxRemoteApiClient::ConnectionChanged);

    Connect(client);

    QCOMPARE(spy.count(), 0);

    Deliver(client, QStringLiteral(R"({"type":"hello","protocol":1})"));
    Deliver(client, QStringLiteral(R"({"type":"snapshot","state":{}})"));

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toBool(), true);
}

void GsxRemoteApiClientTest::aDropAfterAnAnswerReportsTheConnectionLost()
{
    GsxRemoteApiClient client;
    const QSignalSpy spy(&client, &GsxRemoteApiClient::ConnectionChanged);

    Connect(client);
    Deliver(client, QStringLiteral(R"({"type":"hello","protocol":1})"));
    Disconnect(client);

    QCOMPARE(spy.count(), 2);
    QCOMPARE(spy.at(1).at(0).toBool(), false);

    Disconnect(client);

    QCOMPARE(spy.count(), 2);
}

void GsxRemoteApiClientTest::aSocketThatNeverAnswersReportsNoConnection()
{
    GsxRemoteApiClient client;
    client.SetHandshakeTimeoutForTest(kDeadlineMs);
    const QSignalSpy spy(&client, &GsxRemoteApiClient::ConnectionChanged);

    Connect(client);

    QTest::qWait(kDeadlineMs * 4);

    Disconnect(client);

    QCOMPARE(spy.count(), 0);
}

void GsxRemoteApiClientTest::retriesAfterTheFirstAttemptStayQuiet()
{
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    GsxRemoteApiClient client;
    client.SetPortForTest(server.serverPort());

    client.Start();
    Reconnect(client);
    Reconnect(client);

    QCOMPARE(Announcements(), 1);
}

void GsxRemoteApiClientTest::anAnsweredConnectionAnnouncesTheNextAttempt()
{
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    GsxRemoteApiClient client;
    client.SetPortForTest(server.serverPort());

    client.Start();
    Reconnect(client);
    Connect(client);
    Deliver(client, QStringLiteral("{\"type\":\"hello\",\"protocol\":1}"));
    Disconnect(client);
    Reconnect(client);
    Reconnect(client);

    QCOMPARE(Announcements(), 2);
}

void GsxRemoteApiClientTest::aConnectionGsxNeverAnsweredKeepsTheRetriesQuiet()
{
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    GsxRemoteApiClient client;
    client.SetPortForTest(server.serverPort());

    client.Start();
    Connect(client);
    Disconnect(client);
    Reconnect(client);

    QCOMPARE(Announcements(), 1);
}

void GsxRemoteApiClientTest::everySentMessageLandsInTheWireLog()
{
#ifndef NDEBUG
    probe::ResetForTest();
    probe::SetEnabled(true);
    GsxRemoteApiClient client;

    Connect(client);
    QVERIFY(client.SendCommand(QStringLiteral("menu.open"), QJsonObject{{QStringLiteral("index"), 2}}));

    const QStringList lines = WireLines();

    QCOMPARE(lines.size(), 2);
    QVERIFY(lines.at(0).endsWith(
        QStringLiteral("{\"type\":\"sent\",\"message\":{\"channels\":[\"state\",\"prompts\",\"toasts\"],\"type\":\"subscribe\"}}")));
    QVERIFY(lines.at(1).endsWith(
        QStringLiteral("{\"type\":\"sent\",\"message\":{\"args\":{\"index\":2},\"type\":\"command\",\"verb\":\"menu.open\"}}")));
#else
    QSKIP("probe recording is compiled out of Release builds");
#endif
}

QTEST_GUILESS_MAIN(GsxRemoteApiClientTest)

#include "tst_gsx_remote_api_client.moc"
