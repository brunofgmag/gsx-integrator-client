#include <QtTest/QTest>

#include "../TurnaroundStateFixture.h"
#include "../../../src/domain/turnaround/states/RequestBoardingState.h"

class RequestBoardingStateTest final : public QObject
{
    Q_OBJECT

private slots:
    static void callsMenuWhenCallable();
    static void doesNotRequestTwice();
    static void advancesForPassengers();
    static void advancesForCargo();
    static void advancesForCargoBeforePassengersOnPassengerVariant();
    static void holdsUntilGsxActive();
    static void retriesWhenBoardingDoesNotStart();
    static void advancesWhenBoardingAlreadyCompleted();
};

void RequestBoardingStateTest::callsMenuWhenCallable()
{
    TurnaroundStateFixture f;
    RequestBoardingState state;

    f.gsxService.boardingState = GsxStateStatus::Callable;

    QVERIFY(!state.Evaluate(f.ctx).has_value());

    QCOMPARE(f.menuGateway.boardingCalls, 1);
    QVERIFY(f.ctx.data.boardingRequested);
}

void RequestBoardingStateTest::doesNotRequestTwice()
{
    TurnaroundStateFixture f;
    RequestBoardingState state;

    f.gsxService.boardingState = GsxStateStatus::Callable;
    f.ctx.data.boardingRequested = true;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(!transition.has_value());
    QCOMPARE(f.menuGateway.boardingCalls, 0);
}

void RequestBoardingStateTest::advancesForPassengers()
{
    TurnaroundStateFixture f;
    RequestBoardingState state;

    f.aircraft.cargo = false;
    f.gsxService.boardingState = GsxStateStatus::Active;
    f.gsxService.boardedPassengers = 1;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::Boarding);
}

void RequestBoardingStateTest::advancesForCargo()
{
    TurnaroundStateFixture f;
    RequestBoardingState state;

    f.aircraft.cargo = true;
    f.gsxService.boardingState = GsxStateStatus::Active;
    f.gsxService.cargoPercent = 5.0;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::Boarding);
}

void RequestBoardingStateTest::advancesForCargoBeforePassengersOnPassengerVariant()
{
    TurnaroundStateFixture f;
    RequestBoardingState state;

    f.aircraft.cargo = false;
    f.gsxService.boardingState = GsxStateStatus::Active;
    f.gsxService.boardedPassengers = 0;
    f.gsxService.cargoPercent = 17.0;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::Boarding);
}

void RequestBoardingStateTest::holdsUntilGsxActive()
{
    TurnaroundStateFixture f;
    RequestBoardingState state;

    f.aircraft.cargo = true;
    f.gsxService.boardingState = GsxStateStatus::Requested;
    f.gsxService.cargoPercent = 5.0;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
}

void RequestBoardingStateTest::retriesWhenBoardingDoesNotStart()
{
    TurnaroundStateFixture f;
    RequestBoardingState state;

    f.gsxService.boardingState = GsxStateStatus::Callable;

    for (int tick = 0; tick < 12; ++tick)
    {
        ++f.ctx.data.stateTickCount;
        QVERIFY(!state.Evaluate(f.ctx).has_value());
    }

    QCOMPARE(f.menuGateway.boardingCalls, 2);
    QVERIFY(f.ctx.data.boardingRequested);
}

void RequestBoardingStateTest::advancesWhenBoardingAlreadyCompleted()
{
    TurnaroundStateFixture f;
    RequestBoardingState state;

    f.aircraft.cargo = false;
    f.gsxService.boardingState = GsxStateStatus::Unavailable;
    f.gsxService.boardingCompleted = true;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::Boarding);
    QCOMPARE(f.menuGateway.boardingCalls, 0);
}

QTEST_APPLESS_MAIN(RequestBoardingStateTest)

#include "tst_request_boarding_state.moc"
