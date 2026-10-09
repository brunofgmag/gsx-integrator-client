#include <QtTest/QTest>

#include "../src/infrastructure/aircraft/EchoWait.h"

namespace
{
    constexpr int kTicksToWait = 5;
}

class EchoWaitTest final : public QObject
{
    Q_OBJECT

private slots:
    static void startsReadyToWrite();
    static void waitsTheConfiguredTicksAfterAWriteBeforeWritingAgain();
    static void anEchoMakesItReadyAgain();
    static void isNeverDueWhileTheValueIsEchoed();
    static void aWaitOfZeroIsDueOnEveryTickWithoutAnEcho();
};

void EchoWaitTest::startsReadyToWrite()
{
    EchoWait wait(kTicksToWait);

    QVERIFY(wait.WriteIsDue(false));
}

void EchoWaitTest::waitsTheConfiguredTicksAfterAWriteBeforeWritingAgain()
{
    EchoWait wait(kTicksToWait);

    QVERIFY(wait.WriteIsDue(false));

    for (int tick = 0; tick < kTicksToWait; ++tick)
    {
        QVERIFY(!wait.WriteIsDue(false));
    }

    QVERIFY(wait.WriteIsDue(false));

    for (int tick = 0; tick < kTicksToWait; ++tick)
    {
        QVERIFY(!wait.WriteIsDue(false));
    }

    QVERIFY(wait.WriteIsDue(false));
}

void EchoWaitTest::anEchoMakesItReadyAgain()
{
    EchoWait wait(kTicksToWait);

    QVERIFY(wait.WriteIsDue(false));
    QVERIFY(!wait.WriteIsDue(false));
    QVERIFY(!wait.WriteIsDue(true));

    QVERIFY(wait.WriteIsDue(false));
}

void EchoWaitTest::isNeverDueWhileTheValueIsEchoed()
{
    EchoWait wait(kTicksToWait);

    for (int tick = 0; tick < kTicksToWait * 3; ++tick)
    {
        QVERIFY(!wait.WriteIsDue(true));
    }
}

void EchoWaitTest::aWaitOfZeroIsDueOnEveryTickWithoutAnEcho()
{
    EchoWait wait(0);

    QVERIFY(wait.WriteIsDue(false));
    QVERIFY(wait.WriteIsDue(false));
    QVERIFY(!wait.WriteIsDue(true));
}

QTEST_APPLESS_MAIN(EchoWaitTest)

#include "tst_echo_wait.moc"
