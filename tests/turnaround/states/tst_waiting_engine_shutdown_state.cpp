#include <QtTest/QTest>

#include "../TurnaroundStateFixture.h"
#include "../../../src/domain/turnaround/states/WaitingEngineShutdownState.h"

class WaitingEngineShutdownStateTest final : public QObject
{
    Q_OBJECT

private slots:
    static void holdsWhileEnginesRunning();
    static void advancesWhenEnginesOff();
};

void WaitingEngineShutdownStateTest::holdsWhileEnginesRunning()
{
    TurnaroundStateFixture f;
    WaitingEngineShutdownState state;

    f.aircraft.engineRunning = true;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
}

void WaitingEngineShutdownStateTest::advancesWhenEnginesOff()
{
    TurnaroundStateFixture f;
    WaitingEngineShutdownState state;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::PlaceArrivalGroundEquipment);
}

QTEST_APPLESS_MAIN(WaitingEngineShutdownStateTest)

#include "tst_waiting_engine_shutdown_state.moc"
