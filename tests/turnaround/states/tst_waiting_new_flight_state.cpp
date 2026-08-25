#include <QtTest/QTest>

#include "tests/turnaround/TurnaroundStateFixture.h"
#include "src/domain/turnaround/states/WaitingNewFlightState.h"

class WaitingNewFlightStateTest final : public QObject
{
    Q_OBJECT

private slots:
    static void holdsWithoutSmartSwitch();
    static void proceedsOnSmartSwitch();
};

void WaitingNewFlightStateTest::holdsWithoutSmartSwitch()
{
    TurnaroundStateFixture f;
    WaitingNewFlightState state;

    f.ctx.pilotTouched = false;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
}

void WaitingNewFlightStateTest::proceedsOnSmartSwitch()
{
    TurnaroundStateFixture f;
    WaitingNewFlightState state;

    f.ctx.pilotTouched = true;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::WaitingSupportedAircraft);
    QVERIFY(!f.ctx.pilotTouched);
}

QTEST_APPLESS_MAIN(WaitingNewFlightStateTest)

#include "tst_waiting_new_flight_state.moc"
