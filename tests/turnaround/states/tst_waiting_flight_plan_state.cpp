#include <QtTest/QTest>

#include <algorithm>
#include <string>

#include "../TurnaroundStateFixture.h"
#include "../../../src/domain/turnaround/states/WaitingFlightPlanState.h"

class WaitingFlightPlanStateTest final : public QObject
{
    Q_OBJECT

private slots:
    static void holdsWithoutAircraftFlightPlan();
    static void advancesAfterRequestSimbrief();
    static void holdsWhenSimbriefRequestFails();
    static void usesGsxPassengersWhenAircraftPlanHasNone();
    static void shouldRetryWhenFlightPlanFailsToLoad();
    static void logsTheReasonGsxRefusedThePlan();
    static void keepsTheSilentSentenceWhenGsxGaveNoReason();
    static void unloadsPayloadWhileWaiting();
};

void WaitingFlightPlanStateTest::holdsWithoutAircraftFlightPlan()
{
    TurnaroundStateFixture f;
    WaitingFlightPlanState state;

    QVERIFY(!state.Evaluate(f.ctx).has_value());
}

void WaitingFlightPlanStateTest::unloadsPayloadWhileWaiting()
{
    TurnaroundStateFixture f;
    WaitingFlightPlanState state;

    f.aircraft.emptyZfwKg = 90000.0;
    f.aircraft.currentZfwKg = 180000.0;
    f.aircraft.currentFuelKg = 5000.0;

    QVERIFY(!state.Evaluate(f.ctx).has_value());

    QCOMPARE(f.aircraft.currentZfwKg, 90000.0);
    QCOMPARE(f.aircraft.currentFuelKg, 5000.0);
}

void WaitingFlightPlanStateTest::advancesAfterRequestSimbrief()
{
    TurnaroundStateFixture f;
    WaitingFlightPlanState state;

    f.aircraft.flightPlanLoaded = true;
    f.aircraft.plannedFuelKg = 12000.0;
    f.aircraft.plannedZfwKg = 180000.0;
    f.aircraft.currentZfwKg = 100000.0;
    f.aircraft.currentFuelKg = 5000.0;
    f.aircraft.emptyZfwKg = 90000.0;
    f.aircraft.plannedPax = 210;
    f.gsxService.simbriefLoaded = false;

    QVERIFY(!state.Evaluate(f.ctx).has_value());

    f.gsxService.simbriefLoaded = true;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(f.menuGateway.simbriefLoadCalls, 1);
    QCOMPARE(transition->next, TurnaroundPhase::WaitingPowerOn);
    QCOMPARE(f.ctx.data.plannedFuelKg, 12000.0);
    QCOMPARE(f.ctx.data.plannedZfwKg, 180000.0);
    QCOMPARE(f.ctx.data.plannedPassengers, 210);
    QCOMPARE(f.ctx.data.loadedFuelKg, 5000.0);
    QCOMPARE(f.ctx.data.initialFuelKg, 5000.0);
    QCOMPARE(f.ctx.data.loadedZfwKg, 90000.0);
    QCOMPARE(f.ctx.data.initialZfwKg, 90000.0);
    QCOMPARE(f.aircraft.currentZfwKg, 90000.0);
}

void WaitingFlightPlanStateTest::holdsWhenSimbriefRequestFails()
{
    TurnaroundStateFixture f;
    WaitingFlightPlanState state;

    f.aircraft.flightPlanLoaded = true;
    f.gsxService.simbriefLoaded = false;

    for (int tick = 0; tick < 45; ++tick)
    {
        f.ctx.data.stateTickCount = tick;
        QVERIFY(!state.Evaluate(f.ctx).has_value());
    }

    QCOMPARE(f.menuGateway.simbriefLoadCalls, 5);
}

void WaitingFlightPlanStateTest::usesGsxPassengersWhenAircraftPlanHasNone()
{
    TurnaroundStateFixture f;
    WaitingFlightPlanState state;

    f.aircraft.flightPlanLoaded = true;
    f.aircraft.plannedPax = 0;
    f.gsxService.simbriefLoaded = true;
    f.gsxService.plannedPassengers = 73;

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(f.ctx.data.plannedPassengers, 73);
}

void WaitingFlightPlanStateTest::shouldRetryWhenFlightPlanFailsToLoad()
{
    TurnaroundStateFixture f;
    WaitingFlightPlanState state;

    f.aircraft.flightPlanLoaded = true;

    for (int tick = 0; tick < 11; ++tick)
    {
        ++f.ctx.data.stateTickCount;
        QVERIFY(!state.Evaluate(f.ctx).has_value());
    }

    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(!transition.has_value());
    QCOMPARE(f.menuGateway.simbriefLoadCalls, 2);
}

void WaitingFlightPlanStateTest::logsTheReasonGsxRefusedThePlan()
{
    TurnaroundStateFixture f;
    WaitingFlightPlanState state;

    f.aircraft.flightPlanLoaded = true;
    f.gsxService.simbriefError = "SimBrief aircraft A320 doesn't match MSFS aircraft A321";

    for (int tick = 0; tick < 11; ++tick)
    {
        ++f.ctx.data.stateTickCount;
        (void)state.Evaluate(f.ctx);
    }

    const auto refusal = std::ranges::find_if(f.logger.messages, [](const std::string& message)
    {
        return message.find("A320 doesn't match MSFS aircraft A321") != std::string::npos;
    });

    QVERIFY(refusal != f.logger.messages.end());
}

void WaitingFlightPlanStateTest::keepsTheSilentSentenceWhenGsxGaveNoReason()
{
    TurnaroundStateFixture f;
    WaitingFlightPlanState state;

    f.aircraft.flightPlanLoaded = true;

    for (int tick = 0; tick < 11; ++tick)
    {
        ++f.ctx.data.stateTickCount;
        (void)state.Evaluate(f.ctx);
    }

    const auto timeout = std::ranges::find_if(f.logger.messages, [](const std::string& message)
    {
        return message.find("not loaded after") != std::string::npos;
    });

    QVERIFY(timeout != f.logger.messages.end());
}

QTEST_APPLESS_MAIN(WaitingFlightPlanStateTest)

#include "tst_waiting_flight_plan_state.moc"
