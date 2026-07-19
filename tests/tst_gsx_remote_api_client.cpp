#include <QJsonObject>
#include <QJsonValue>
#include <QSignalSpy>
#include <QtTest/QTest>

#include "../src/infrastructure/gsx/GsxRemoteApiClient.h"

namespace
{
    void Deliver(GsxRemoteApiClient& client, const QString& text)
    {
        QMetaObject::invokeMethod(&client, "OnTextMessage", Qt::DirectConnection, Q_ARG(QString, text));
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

QTEST_GUILESS_MAIN(GsxRemoteApiClientTest)

#include "tst_gsx_remote_api_client.moc"
