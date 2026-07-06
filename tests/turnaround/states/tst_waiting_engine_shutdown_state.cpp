#include <QtTest/QTest>

#include "../TurnaroundStateFixture.h"
#include "../../../src/domain/turnaround/states/WaitingEngineShutdownState.h"

class WaitingEngineShutdownStateTest final : public QObject
{
    Q_OBJECT

private slots:
    static void holdsUntilReadyToDeboard();
    static void advancesWhenReadyToDeboard();
};

void WaitingEngineShutdownStateTest::holdsUntilReadyToDeboard()
{
    TurnaroundStateFixture f;
    WaitingEngineShutdownState state;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
}

void WaitingEngineShutdownStateTest::advancesWhenReadyToDeboard()
{
    TurnaroundStateFixture f;
    WaitingEngineShutdownState state;

    f.aircraft.readyToDeboard = true;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RequestDeboarding);
}

QTEST_APPLESS_MAIN(WaitingEngineShutdownStateTest)

#include "tst_waiting_engine_shutdown_state.moc"
