#include <QtTest/QTest>

#include "../TurnaroundStateFixture.h"
#include "../../../src/domain/turnaround/states/RequestPushbackState.h"

class RequestPushbackStateTest final : public QObject
{
    Q_OBJECT

private slots:
    static void requestsPushbackWhenCallable();
    static void doesNotRequestTwice();
    static void advancesOnRequested();
    static void advancesOnActive();
    static void advancesWhenDepartureRequestedRemotely();
    static void retryWhenPushbackStaysCallable();
    static void advancesWhenPushbackAlreadyCompleted();
    static void holdsWhileDeiceRequested();
    static void holdsWhileDeiceActive();
    static void doesNotRetryWhileDeiceActive();
    static void resumesAfterDeiceCompletes();
    static void asksForTheClearanceAndSkipsTheTugWhereGsxOffersNoPushback();
    static void theClearanceWaitsForDeiceToFinish();
};

void RequestPushbackStateTest::requestsPushbackWhenCallable()
{
    TurnaroundStateFixture f;
    RequestPushbackState state;

    f.gsxService.departureState = GsxStateStatus::Callable;

    QVERIFY(!state.Evaluate(f.ctx).has_value());

    QCOMPARE(f.menuGateway.pushbackCalls, 1);
    QVERIFY(f.ctx.data.pushbackRequested);
}

void RequestPushbackStateTest::doesNotRequestTwice()
{
    TurnaroundStateFixture f;
    RequestPushbackState state;

    f.gsxService.departureState = GsxStateStatus::Callable;
    f.ctx.data.pushbackRequested = true;

    QVERIFY(!state.Evaluate(f.ctx).has_value());

    QCOMPARE(f.menuGateway.pushbackCalls, 0);
}

void RequestPushbackStateTest::advancesOnRequested()
{
    TurnaroundStateFixture f;
    RequestPushbackState state;

    f.gsxService.departureState = GsxStateStatus::Requested;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::WaitingPushbackToStart);
}

void RequestPushbackStateTest::advancesOnActive()
{
    TurnaroundStateFixture f;
    RequestPushbackState state;

    f.gsxService.departureState = GsxStateStatus::Active;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::WaitingPushbackToStart);
}

void RequestPushbackStateTest::advancesWhenDepartureRequestedRemotely()
{
    TurnaroundStateFixture f;
    RequestPushbackState state;

    f.gsxService.departureState = GsxStateStatus::Callable;
    f.gsxService.departureInProgress = true;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::WaitingPushbackToStart);
    QCOMPARE(f.menuGateway.pushbackCalls, 0);
}

void RequestPushbackStateTest::retryWhenPushbackStaysCallable()
{
    TurnaroundStateFixture f;
    RequestPushbackState state;

    f.gsxService.departureState = GsxStateStatus::Callable;

    for (int tick = 0; tick < 62; ++tick)
    {
        ++f.ctx.data.stateTickCount;
        QVERIFY(!state.Evaluate(f.ctx).has_value());
    }

    QVERIFY(f.ctx.data.pushbackRequested);
    QCOMPARE(f.menuGateway.pushbackCalls, 2);
}

void RequestPushbackStateTest::advancesWhenPushbackAlreadyCompleted()
{
    TurnaroundStateFixture f;
    RequestPushbackState state;

    f.gsxService.departureState = GsxStateStatus::Unavailable;
    f.gsxService.pushbackCompleted = true;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::WaitingPushbackToStart);
    QCOMPARE(f.menuGateway.pushbackCalls, 0);
}

void RequestPushbackStateTest::holdsWhileDeiceRequested()
{
    TurnaroundStateFixture f;
    RequestPushbackState state;

    f.gsxService.departureState = GsxStateStatus::Callable;
    f.gsxService.deiceState = GsxStateStatus::Requested;

    QVERIFY(!state.Evaluate(f.ctx).has_value());

    QCOMPARE(f.menuGateway.pushbackCalls, 0);
    QVERIFY(!f.ctx.data.pushbackRequested);
}

void RequestPushbackStateTest::holdsWhileDeiceActive()
{
    TurnaroundStateFixture f;
    RequestPushbackState state;

    f.gsxService.departureState = GsxStateStatus::Requested;
    f.gsxService.deiceState = GsxStateStatus::Active;

    QVERIFY(!state.Evaluate(f.ctx).has_value());

    QCOMPARE(f.menuGateway.pushbackCalls, 0);
}

void RequestPushbackStateTest::doesNotRetryWhileDeiceActive()
{
    TurnaroundStateFixture f;
    RequestPushbackState state;

    f.gsxService.departureState = GsxStateStatus::Callable;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.menuGateway.pushbackCalls, 1);

    f.gsxService.deiceState = GsxStateStatus::Active;

    for (int tick = 0; tick < 62; ++tick)
    {
        ++f.ctx.data.stateTickCount;
        QVERIFY(!state.Evaluate(f.ctx).has_value());
    }

    QCOMPARE(f.menuGateway.pushbackCalls, 1);
    QVERIFY(f.ctx.data.pushbackRequested);
}

void RequestPushbackStateTest::resumesAfterDeiceCompletes()
{
    TurnaroundStateFixture f;
    RequestPushbackState state;

    f.gsxService.departureState = GsxStateStatus::Callable;
    f.gsxService.deiceState = GsxStateStatus::Active;

    QVERIFY(!state.Evaluate(f.ctx).has_value());

    f.gsxService.deiceState = GsxStateStatus::Completed;
    f.gsxService.departureState = GsxStateStatus::Requested;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::WaitingPushbackToStart);
}

void RequestPushbackStateTest::asksForTheClearanceAndSkipsTheTugWhereGsxOffersNoPushback()
{
    TurnaroundStateFixture f;
    RequestPushbackState state;

    f.gsxService.offersPushback = false;
    f.gsxService.departureState = GsxStateStatus::Callable;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::WaitingDeparture);
    QCOMPARE(f.menuGateway.departureClearanceCalls, 1);
    QCOMPARE(f.menuGateway.pushbackCalls, 0);
    QVERIFY(f.ctx.data.pushbackRequested);

    const auto again = state.Evaluate(f.ctx);

    QVERIFY(again.has_value());
    QCOMPARE(again->next, TurnaroundPhase::WaitingDeparture);
    QCOMPARE(f.menuGateway.departureClearanceCalls, 1);
}

void RequestPushbackStateTest::theClearanceWaitsForDeiceToFinish()
{
    TurnaroundStateFixture f;
    RequestPushbackState state;

    f.gsxService.offersPushback = false;
    f.gsxService.departureState = GsxStateStatus::Callable;
    f.gsxService.deiceState = GsxStateStatus::Active;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.menuGateway.departureClearanceCalls, 0);

    f.gsxService.deiceState = GsxStateStatus::Completed;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::WaitingDeparture);
    QCOMPARE(f.menuGateway.departureClearanceCalls, 1);
}

QTEST_APPLESS_MAIN(RequestPushbackStateTest)

#include "tst_request_pushback_state.moc"
