#include <QtCore/QCoreApplication>
#include <QtCore/QFileInfo>
#include <QtCore/QProcess>
#include <QtCore/QTemporaryDir>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "../src/infrastructure/platform/InstanceChannel.h"

namespace
{
    QString UniqueName()
    {
        return QStringLiteral("gsxi-test-%1-%2").arg(QCoreApplication::applicationPid()).arg(QTest::currentTestFunction());
    }
}

class InstanceChannelTest final : public QObject
{
    Q_OBJECT

private slots:
    static void nobodyListeningMeansThisIsTheFirstInstance();
    static void aSecondInstanceAsksTheFirstToShowItsWindow();
    static void aSecondInstanceStartedHiddenDoesNotAskForTheWindow();
    static void aStaleSocketFileDoesNotBlockTheNewFirstInstance();
    static void theChannelNameIsFixed();
};

void InstanceChannelTest::nobodyListeningMeansThisIsTheFirstInstance()
{
    QVERIFY(!NotifyRunningInstance(UniqueName(), true, 200));
}

void InstanceChannelTest::aSecondInstanceAsksTheFirstToShowItsWindow()
{
    InstanceServer server;
    QVERIFY(server.Listen(UniqueName()));
    QSignalSpy shown(&server, &InstanceServer::ShowRequested);

    QVERIFY(NotifyRunningInstance(UniqueName(), true));

    QTRY_COMPARE(shown.count(), 1);
}

void InstanceChannelTest::aSecondInstanceStartedHiddenDoesNotAskForTheWindow()
{
    InstanceServer server;
    QVERIFY(server.Listen(UniqueName()));
    QSignalSpy shown(&server, &InstanceServer::ShowRequested);

    QVERIFY(NotifyRunningInstance(UniqueName(), false));
    QTest::qWait(150);

    QCOMPARE(shown.count(), 0);
}

void InstanceChannelTest::aStaleSocketFileDoesNotBlockTheNewFirstInstance()
{
    const QTemporaryDir directory;
    const QString name = directory.filePath(QStringLiteral("stale.sock"));
    QProcess python;
    python.start(QStringLiteral("python3"),
                 {QStringLiteral("-c"),
                  QStringLiteral("import socket,sys; socket.socket(socket.AF_UNIX).bind(sys.argv[1])"), name});
    if (!python.waitForFinished(5000) || python.exitCode() != 0)
    {
        QSKIP("python3 is needed to leave a stale socket file behind");
    }
    QVERIFY(QFileInfo(name).exists());
    QVERIFY(!NotifyRunningInstance(name, true, 200));

    InstanceServer server;

    QVERIFY(server.Listen(name));
    QVERIFY(NotifyRunningInstance(name, false));
}

void InstanceChannelTest::theChannelNameIsFixed()
{
    QCOMPARE(InstanceChannelName(), QStringLiteral("gsx-integrator-client.single-instance"));
}

QTEST_MAIN(InstanceChannelTest)

#include "tst_instance_channel.moc"
