#include <QtTest/QTest>

#include "../TurnaroundStateFixture.h"
#include "../../../src/domain/turnaround/states/RequestDeboardingState.h"

class RequestDeboardingStateTest final : public QObject
{
    Q_OBJECT

private slots:
    static void callsMenuWhenCallable();
    static void holdsRequestUntilReadyToDeboard();
    static void doesNotRequestTwice();
    static void advancesForPassengers();
    static void advancesForCargo();
    static void advancesForCargoBeforePassengersOnPassengerVariant();
    static void advancesWhenDeboardingAlreadyCompleted();
    static void holdsUntilGsxActive();
    static void retriesWhenBoardingDoesNotStart();
    static void retriesForPassengersWhenDeboardingDoesNotStart();
    static void holdsRequestWhileMenuUnsettled();
};

void RequestDeboardingStateTest::callsMenuWhenCallable()
{
    TurnaroundStateFixture f;
    RequestDeboardingState state;

    f.aircraft.readyToDeboard = true;
    f.gsxService.deboardingState = GsxStateStatus::Callable;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(!transition.has_value());
    QCOMPARE(f.menuGateway.deboardingCalls, 1);
    QVERIFY(f.ctx.data.deboardingRequested);
}

void RequestDeboardingStateTest::holdsRequestUntilReadyToDeboard()
{
    TurnaroundStateFixture f;
    RequestDeboardingState state;

    f.gsxService.deboardingState = GsxStateStatus::Callable;

    QVERIFY(!state.Evaluate(f.ctx).has_value());

    QCOMPARE(f.menuGateway.deboardingCalls, 0);
    QVERIFY(!f.ctx.data.deboardingRequested);
}

void RequestDeboardingStateTest::doesNotRequestTwice()
{
    TurnaroundStateFixture f;
    RequestDeboardingState state;

    f.aircraft.readyToDeboard = true;
    f.gsxService.deboardingState = GsxStateStatus::Callable;
    f.ctx.data.deboardingRequested = true;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(!transition.has_value());
    QCOMPARE(f.menuGateway.deboardingCalls, 0);
}

void RequestDeboardingStateTest::advancesForPassengers()
{
    TurnaroundStateFixture f;
    RequestDeboardingState state;

    f.aircraft.cargo = false;
    f.gsxService.deboardingState = GsxStateStatus::Active;
    f.ctx.data.plannedPassengers = 200;
    f.gsxService.deboardedPassengers = 10;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::Deboarding);
}

void RequestDeboardingStateTest::advancesForCargo()
{
    TurnaroundStateFixture f;
    RequestDeboardingState state;

    f.aircraft.cargo = true;
    f.gsxService.deboardingState = GsxStateStatus::Active;
    f.gsxService.deboardingCargoPercent = 5.0;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::Deboarding);
}

void RequestDeboardingStateTest::advancesForCargoBeforePassengersOnPassengerVariant()
{
    TurnaroundStateFixture f;
    RequestDeboardingState state;

    f.aircraft.cargo = false;
    f.gsxService.deboardingState = GsxStateStatus::Active;
    f.gsxService.deboardedPassengers = 0;
    f.gsxService.deboardingCargoPercent = 17.0;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::Deboarding);
}

void RequestDeboardingStateTest::advancesWhenDeboardingAlreadyCompleted()
{
    TurnaroundStateFixture f;
    RequestDeboardingState state;

    f.aircraft.cargo = false;
    f.ctx.data.plannedPassengers = 0;
    f.gsxService.deboardingCompleted = true;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::Deboarding);
}

void RequestDeboardingStateTest::holdsUntilGsxActive()
{
    TurnaroundStateFixture f;
    RequestDeboardingState state;

    f.aircraft.cargo = true;
    f.gsxService.deboardingState = GsxStateStatus::Requested;
    f.gsxService.deboardingCargoPercent = 5.0;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
}

void RequestDeboardingStateTest::retriesWhenBoardingDoesNotStart()
{
    TurnaroundStateFixture f;
    RequestDeboardingState state;

    f.aircraft.readyToDeboard = true;
    f.gsxService.deboardingState = GsxStateStatus::Callable;

    for (int tick = 0; tick < 62; ++tick)
    {
        ++f.ctx.data.stateTickCount;
        QVERIFY(!state.Evaluate(f.ctx).has_value());
    }

    QCOMPARE(f.menuGateway.deboardingCalls, 2);
    QVERIFY(f.ctx.data.deboardingRequested);
}

void RequestDeboardingStateTest::retriesForPassengersWhenDeboardingDoesNotStart()
{
    TurnaroundStateFixture f;
    RequestDeboardingState state;

    f.aircraft.cargo = false;
    f.ctx.data.plannedPassengers = 200;
    f.gsxService.deboardedPassengers = 0;
    f.aircraft.readyToDeboard = true;
    f.gsxService.deboardingState = GsxStateStatus::Callable;

    for (int tick = 0; tick < 62; ++tick)
    {
        ++f.ctx.data.stateTickCount;
        QVERIFY(!state.Evaluate(f.ctx).has_value());
    }

    QCOMPARE(f.menuGateway.deboardingCalls, 2);
    QVERIFY(f.ctx.data.deboardingRequested);
}

void RequestDeboardingStateTest::holdsRequestWhileMenuUnsettled()
{
    TurnaroundStateFixture f;
    RequestDeboardingState state;

    f.aircraft.readyToDeboard = true;
    f.gsxService.deboardingState = GsxStateStatus::Callable;
    f.menuGateway.menuSettled = false;

    QVERIFY(!state.Evaluate(f.ctx).has_value());

    QCOMPARE(f.menuGateway.deboardingCalls, 0);
    QVERIFY(!f.ctx.data.deboardingRequested);
}

QTEST_APPLESS_MAIN(RequestDeboardingStateTest)

#include "tst_request_deboarding_state.moc"
