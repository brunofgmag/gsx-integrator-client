#include <QtTest/QTest>

#include "tests/turnaround/TurnaroundStateFixture.h"
#include "src/domain/turnaround/states/WaitingDepartureState.h"

class WaitingDepartureStateTest final : public QObject
{
    Q_OBJECT

private slots:
    static void holdsOnGround();
    static void advancesWhenAirborne();
    static void returnsToTheEngineWaitWhileGsxStillAsks();
    static void returnsToTheEngineWaitOnlyOnce();
    static void staysWhenTheConfirmationIsDisabled();
    static void staysWhenGsxDoesNotAsk();
    static void staysOnTheGroundOnceTheAircraftLeftTheStand();
    static void leavesForTheFlightWhenAirborneEvenIfGsxStillAsks();
};

namespace
{
    void ArmStillAsking(TurnaroundStateFixture& f)
    {
        f.gsxService.goodEngineStartConfirmation = true;
        f.gsxService.waitingForEngines = true;
        f.ctx.data.engineConfirmationSent = true;
    }
}

void WaitingDepartureStateTest::holdsOnGround()
{
    TurnaroundStateFixture f;
    WaitingDepartureState state;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
}

void WaitingDepartureStateTest::advancesWhenAirborne()
{
    TurnaroundStateFixture f;
    WaitingDepartureState state;

    f.gsxService.onGround = false;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::OnFlight);
}

void WaitingDepartureStateTest::returnsToTheEngineWaitWhileGsxStillAsks()
{
    TurnaroundStateFixture f;
    WaitingDepartureState state;

    ArmStillAsking(f);

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::WaitingForEngines);
    QVERIFY(f.ctx.data.engineWaitResumed);
    QVERIFY(!f.ctx.data.engineConfirmationSent);
    QCOMPARE(f.logger.messages.size(), std::size_t{1});
}

void WaitingDepartureStateTest::returnsToTheEngineWaitOnlyOnce()
{
    TurnaroundStateFixture f;
    WaitingDepartureState state;

    ArmStillAsking(f);

    QVERIFY(state.Evaluate(f.ctx).has_value());
    QVERIFY(!state.Evaluate(f.ctx).has_value());
}

void WaitingDepartureStateTest::staysWhenTheConfirmationIsDisabled()
{
    TurnaroundStateFixture f;
    WaitingDepartureState state;

    ArmStillAsking(f);
    f.gsxService.goodEngineStartConfirmation = false;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QVERIFY(!f.ctx.data.engineWaitResumed);
}

void WaitingDepartureStateTest::staysWhenGsxDoesNotAsk()
{
    TurnaroundStateFixture f;
    WaitingDepartureState state;

    ArmStillAsking(f);
    f.gsxService.waitingForEngines = false;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QVERIFY(!f.ctx.data.engineWaitResumed);
}

void WaitingDepartureStateTest::staysOnTheGroundOnceTheAircraftLeftTheStand()
{
    TurnaroundStateFixture f;
    WaitingDepartureState state;

    ArmStillAsking(f);
    f.aircraft.engineRunning = true;
    f.gsxService.groundSpeedKnots = 12.0;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QVERIFY(!f.ctx.data.engineWaitResumed);
}

void WaitingDepartureStateTest::leavesForTheFlightWhenAirborneEvenIfGsxStillAsks()
{
    TurnaroundStateFixture f;
    WaitingDepartureState state;

    ArmStillAsking(f);
    f.gsxService.onGround = false;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::OnFlight);
    QVERIFY(!f.ctx.data.engineWaitResumed);
}

QTEST_APPLESS_MAIN(WaitingDepartureStateTest)

#include "tst_waiting_departure_state.moc"
