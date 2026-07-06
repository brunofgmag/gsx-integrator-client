#include <QtTest/QTest>

#include "../TurnaroundStateFixture.h"
#include "../../../src/domain/turnaround/states/WaitingPowerOnState.h"

class WaitingPowerOnStateTest final : public QObject
{
    Q_OBJECT

private slots:
    static void holdsWhenAircraftIsUnpowered();
    static void advancesWhenAircraftIsPowered();
};

void WaitingPowerOnStateTest::holdsWhenAircraftIsUnpowered()
{
    TurnaroundStateFixture f;
    WaitingPowerOnState state;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
}

void WaitingPowerOnStateTest::advancesWhenAircraftIsPowered()
{
    TurnaroundStateFixture f;
    WaitingPowerOnState state;

    f.aircraft.powered = true;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::RequestFuel);
}

QTEST_APPLESS_MAIN(WaitingPowerOnStateTest)

#include "tst_waiting_power_on_state.moc"
