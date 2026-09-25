#include <QtTest/QTest>

#include <algorithm>
#include <cmath>
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
    static void flagsAPlanWhoseOperatingEmptyWeightLeavesOutTheCrew();
    static void trustsAPlanWhoseOperatingEmptyWeightCountsTheCrew();
    static void callsAnOperatingEmptyWeightWithinTheWeightEpsilonTheAircraftEmptyWeight();
    static void staysQuietForAnAircraftThatReportsNoCrew();
    static void capturesTheRegeneratedPlanAfterGsxRefusedTheFirstGeneration();
    static void neverAsksForTheLatestPlanWhenGsxNeverRefused();
    static void asksAgainWhenTheLatestPlanFailsToLoad();
    static void capturesWithoutAFlightPlanSource();
};

namespace
{
    constexpr double kEmptyWeightKg = 42306.0;
    constexpr double kCrewKg = 195.044719;
    constexpr double kPayloadKg = 16500.0;
    constexpr double kTolerance = 1e-6;

    void ArrangeTheMeasuredPlan(TurnaroundStateFixture& f, const double plannedOperatingEmptyKg)
    {
        f.aircraft.flightPlanLoaded = true;
        f.aircraft.emptyZfwKg = kEmptyWeightKg;
        f.aircraft.crewOnBoardKg = kCrewKg;
        f.aircraft.plannedOperatingEmptyKg = plannedOperatingEmptyKg;
        f.aircraft.plannedZfwKg = kEmptyWeightKg + kCrewKg + kPayloadKg;
        f.gsxService.simbriefLoaded = true;
    }

    constexpr auto kRouteRefusal =
        "The loaded flight plan from LOWW doesn't match the one on SimBrief, from SBFZ to SBTE";

    void ArrangeTheRefusedPlan(TurnaroundStateFixture& f)
    {
        f.status.flightPlanStatus = FlightPlanStatus::Ready;
        f.aircraft.flightPlanLoaded = true;
        f.aircraft.plannedFuelKg = 4897.0;
        f.aircraft.plannedZfwKg = 52000.0;
        f.aircraft.plannedPax = 77;
        f.gsxService.simbriefLoaded = false;
        f.gsxService.simbriefError = kRouteRefusal;
    }

    void GsxAcceptsTheRegeneratedPlan(TurnaroundStateFixture& f)
    {
        f.gsxService.simbriefError.clear();
        f.gsxService.simbriefLoaded = true;
    }

    bool Logged(const TurnaroundStateFixture& f, const std::string& fragment)
    {
        return std::ranges::any_of(f.logger.messages, [&fragment](const std::string& message)
        {
            return message.find(fragment) != std::string::npos;
        });
    }

    bool LoggedTheOmittedCrew(const TurnaroundStateFixture& f)
    {
        return std::ranges::any_of(f.logger.messages, [](const std::string& message)
        {
            return message.find("The plan's operating empty weight of 42306 kg leaves out 195 kg of crew; "
                                "the ZFW target is 59001 kg, and SimBrief needs 42501 kg to count the crew")
                != std::string::npos;
        });
    }
}

void WaitingFlightPlanStateTest::flagsAPlanWhoseOperatingEmptyWeightLeavesOutTheCrew()
{
    TurnaroundStateFixture f;
    WaitingFlightPlanState state;

    ArrangeTheMeasuredPlan(f, kEmptyWeightKg);

    QVERIFY(state.Evaluate(f.ctx).has_value());
    QVERIFY(f.ctx.data.planOmitsCrew);
    QVERIFY(std::abs(f.ctx.data.omittedCrewKg - kCrewKg) < kTolerance);
    QVERIFY(std::abs(f.ctx.data.operatingEmptyWithCrewKg - (kEmptyWeightKg + kCrewKg)) < kTolerance);
    QVERIFY(LoggedTheOmittedCrew(f));
}

void WaitingFlightPlanStateTest::trustsAPlanWhoseOperatingEmptyWeightCountsTheCrew()
{
    TurnaroundStateFixture f;
    WaitingFlightPlanState state;

    ArrangeTheMeasuredPlan(f, kEmptyWeightKg + kCrewKg);

    QVERIFY(state.Evaluate(f.ctx).has_value());
    QVERIFY(!f.ctx.data.planOmitsCrew);
    QCOMPARE(f.ctx.data.omittedCrewKg, 0.0);
    QCOMPARE(f.ctx.data.operatingEmptyWithCrewKg, 0.0);
    QVERIFY(!LoggedTheOmittedCrew(f));
}

void WaitingFlightPlanStateTest::callsAnOperatingEmptyWeightWithinTheWeightEpsilonTheAircraftEmptyWeight()
{
    for (const double offsetKg : {-49.0, 49.0})
    {
        TurnaroundStateFixture f;
        WaitingFlightPlanState state;

        ArrangeTheMeasuredPlan(f, kEmptyWeightKg + offsetKg);

        QVERIFY(state.Evaluate(f.ctx).has_value());
        QVERIFY(f.ctx.data.planOmitsCrew);
        QVERIFY(std::abs(f.ctx.data.operatingEmptyWithCrewKg - (kEmptyWeightKg + offsetKg + kCrewKg)) < kTolerance);
    }

    for (const double offsetKg : {-51.0, 51.0})
    {
        TurnaroundStateFixture f;
        WaitingFlightPlanState state;

        ArrangeTheMeasuredPlan(f, kEmptyWeightKg + offsetKg);

        QVERIFY(state.Evaluate(f.ctx).has_value());
        QVERIFY(!f.ctx.data.planOmitsCrew);
    }
}

void WaitingFlightPlanStateTest::staysQuietForAnAircraftThatReportsNoCrew()
{
    TurnaroundStateFixture f;
    WaitingFlightPlanState state;

    ArrangeTheMeasuredPlan(f, kEmptyWeightKg);
    f.aircraft.crewOnBoardKg = 0.0;

    QVERIFY(state.Evaluate(f.ctx).has_value());
    QVERIFY(!f.ctx.data.planOmitsCrew);
    QCOMPARE(f.ctx.data.omittedCrewKg, 0.0);
    QVERIFY(f.logger.messages.empty());
}

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

void WaitingFlightPlanStateTest::capturesTheRegeneratedPlanAfterGsxRefusedTheFirstGeneration()
{
    TurnaroundStateFixture f;
    WaitingFlightPlanState state;

    ArrangeTheRefusedPlan(f);

    for (int tick = 0; tick < 25; ++tick)
    {
        ++f.ctx.data.stateTickCount;
        QVERIFY(!state.Evaluate(f.ctx).has_value());
    }

    QCOMPARE(f.flightPlanSource.latestRequests, 0);

    GsxAcceptsTheRegeneratedPlan(f);

    ++f.ctx.data.stateTickCount;
    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.flightPlanSource.latestRequests, 1);
    QCOMPARE(f.status.flightPlanStatus, FlightPlanStatus::Fetching);
    QVERIFY(Logged(f, "GSX accepted the SimBrief plan after refusing it: fetching the latest OFP before capturing it"));

    for (int tick = 0; tick < 5; ++tick)
    {
        ++f.ctx.data.stateTickCount;
        QVERIFY(!state.Evaluate(f.ctx).has_value());
    }

    QCOMPARE(f.flightPlanSource.latestRequests, 1);

    f.aircraft.plannedFuelKg = 4455.0;
    f.aircraft.plannedZfwKg = 53400.0;
    f.aircraft.plannedPax = 92;
    f.status.flightPlanStatus = FlightPlanStatus::Ready;

    ++f.ctx.data.stateTickCount;
    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::WaitingPowerOn);
    QCOMPARE(f.flightPlanSource.latestRequests, 1);
    QCOMPARE(f.ctx.data.plannedFuelKg, 4455.0);
    QCOMPARE(f.ctx.data.plannedZfwKg, 53400.0);
    QCOMPARE(f.ctx.data.plannedPassengers, 92);
}

void WaitingFlightPlanStateTest::neverAsksForTheLatestPlanWhenGsxNeverRefused()
{
    TurnaroundStateFixture f;
    WaitingFlightPlanState state;

    ArrangeTheRefusedPlan(f);
    f.gsxService.simbriefError.clear();

    for (int tick = 0; tick < 25; ++tick)
    {
        ++f.ctx.data.stateTickCount;
        QVERIFY(!state.Evaluate(f.ctx).has_value());
    }

    f.gsxService.simbriefLoaded = true;

    ++f.ctx.data.stateTickCount;
    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(f.flightPlanSource.latestRequests, 0);
    QCOMPARE(f.status.flightPlanStatus, FlightPlanStatus::Ready);
    QCOMPARE(f.ctx.data.plannedFuelKg, 4897.0);
    QCOMPARE(f.ctx.data.plannedPassengers, 77);
}

void WaitingFlightPlanStateTest::asksAgainWhenTheLatestPlanFailsToLoad()
{
    TurnaroundStateFixture f;
    WaitingFlightPlanState state;

    ArrangeTheRefusedPlan(f);

    ++f.ctx.data.stateTickCount;
    QVERIFY(!state.Evaluate(f.ctx).has_value());

    GsxAcceptsTheRegeneratedPlan(f);

    ++f.ctx.data.stateTickCount;
    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.flightPlanSource.latestRequests, 1);

    f.status.flightPlanStatus = FlightPlanStatus::Error;
    f.aircraft.flightPlanLoaded = false;

    while (f.ctx.data.stateTickCount < 9)
    {
        ++f.ctx.data.stateTickCount;
        QVERIFY(!state.Evaluate(f.ctx).has_value());
    }

    QCOMPARE(f.flightPlanSource.latestRequests, 1);

    ++f.ctx.data.stateTickCount;
    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.flightPlanSource.latestRequests, 2);
    QCOMPARE(f.status.flightPlanStatus, FlightPlanStatus::Fetching);
    QVERIFY(Logged(f, "The latest SimBrief OFP failed to load: fetching it again"));
}

void WaitingFlightPlanStateTest::capturesWithoutAFlightPlanSource()
{
    TurnaroundStateFixture f;
    WaitingFlightPlanState state;

    f.ctx.flightPlanSource = nullptr;
    ArrangeTheRefusedPlan(f);

    ++f.ctx.data.stateTickCount;
    QVERIFY(!state.Evaluate(f.ctx).has_value());

    GsxAcceptsTheRegeneratedPlan(f);

    ++f.ctx.data.stateTickCount;
    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(f.ctx.data.plannedFuelKg, 4897.0);
}

QTEST_APPLESS_MAIN(WaitingFlightPlanStateTest)

#include "tst_waiting_flight_plan_state.moc"
