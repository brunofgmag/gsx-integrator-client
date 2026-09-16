#include <QJsonObject>
#include <QJsonValue>
#include <QSignalSpy>
#include <QtTest/QTest>

#include "../src/infrastructure/gsx/GsxRemoteApiClient.h"

namespace
{
    constexpr int kDeadlineMs = 30;

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
};

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

QTEST_GUILESS_MAIN(GsxRemoteApiClientTest)

#include "tst_gsx_remote_api_client.moc"
