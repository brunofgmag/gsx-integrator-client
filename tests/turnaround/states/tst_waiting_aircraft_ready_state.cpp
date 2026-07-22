#include <QtTest/QTest>

#include "../TurnaroundStateFixture.h"
#include "../../../src/domain/turnaround/states/WaitingAircraftReadyState.h"

class WaitingAircraftReadyStateTest final : public QObject
{
    Q_OBJECT

private slots:
    static void holdsWhileEnginesRunning();
    static void advancesWhenEnginesOff();
};

void WaitingAircraftReadyStateTest::holdsWhileEnginesRunning()
{
    TurnaroundStateFixture f;
    WaitingAircraftReadyState state;

    f.aircraft.engineRunning = true;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
}

void WaitingAircraftReadyStateTest::advancesWhenEnginesOff()
{
    TurnaroundStateFixture f;
    WaitingAircraftReadyState state;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RepositionAircraft);
    QCOMPARE(transition->delayTicks, 0);
}

QTEST_APPLESS_MAIN(WaitingAircraftReadyStateTest)

#include "tst_waiting_aircraft_ready_state.moc"
