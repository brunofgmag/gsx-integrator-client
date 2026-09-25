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
    static void fetchesTheLatestPlanEveryThirtyTicksWhileTheAircraftPlanDiffers();
    static void neverFetchesTheLatestPlanWhileTheAircraftHasNoPlan();
    static void holdsADifferingPlanWithoutAFlightPlanSource();
    static void leavesTheFetchToTheRefusalFlowWhileItAwaitsTheLatestPlan();
    static void ignoresTheRefusalGsxPublishedBeforeTheRequest();
    static void retriesOnTheTenthTickAfterTheRequest();
    static void flagsTheRefusalGsxPublishesAtTheRetry();
    static void reloadsGsxWhenTheDifferingPlanFetchBringsANewOfp();
    static void retriesTheReloadOnTheTenthTickWhileGsxServesTheOldGeneration();
    static void waitsPastANewerGenerationGsxRefused();
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

    constexpr auto kDifferingPlanFetch = "The aircraft flight plan differs from the OFP: fetching the latest OFP";

    void TickOnce(TurnaroundStateFixture& f, WaitingFlightPlanState& state)
    {
        ++f.ctx.data.stateTickCount;
        QVERIFY(!state.Evaluate(f.ctx).has_value());
    }

    constexpr int kTicksUntilTheRetry = 11;

    void TickUntilTheRetry(TurnaroundStateFixture& f, WaitingFlightPlanState& state)
    {
        for (int tick = 0; tick < kTicksUntilTheRetry; ++tick)
        {
            TickOnce(f, state);
        }
    }

    constexpr double kPreviousFlightFuelKg = 5210.0;
    constexpr double kNewFlightFuelKg = 8381.0;

    void FetchTheNewOfpWhileGsxServesThePreviousFlight(TurnaroundStateFixture& f, WaitingFlightPlanState& state)
    {
        f.status.flightPlanStatus = FlightPlanStatus::Ready;
        f.aircraft.plannedFuelKg = kPreviousFlightFuelKg;
        f.gsxService.simbriefLoaded = true;
        f.gsxService.simbriefGeneration = 1;
        f.aircraft.flightPlanDiffersFromTheOfp = true;

        TickOnce(f, state);
        QCOMPARE(f.flightPlanSource.latestRequests, 1);

        f.aircraft.flightPlanDiffersFromTheOfp = false;
        f.aircraft.flightPlanLoaded = true;
        f.aircraft.plannedFuelKg = kNewFlightFuelKg;
        f.status.flightPlanStatus = FlightPlanStatus::Ready;
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

    TickUntilTheRetry(f, state);

    GsxAcceptsTheRegeneratedPlan(f);

    ++f.ctx.data.stateTickCount;
    QVERIFY(!state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.flightPlanSource.latestRequests, 1);

    f.status.flightPlanStatus = FlightPlanStatus::Error;
    f.aircraft.flightPlanLoaded = false;

    while (f.ctx.data.stateTickCount < 19)
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

    TickUntilTheRetry(f, state);
    QVERIFY(f.ctx.data.flightPlanRefused);

    GsxAcceptsTheRegeneratedPlan(f);

    ++f.ctx.data.stateTickCount;
    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(f.ctx.data.plannedFuelKg, 4897.0);
}

void WaitingFlightPlanStateTest::fetchesTheLatestPlanEveryThirtyTicksWhileTheAircraftPlanDiffers()
{
    TurnaroundStateFixture f;
    WaitingFlightPlanState state;

    f.status.flightPlanStatus = FlightPlanStatus::Ready;

    for (int tick = 0; tick < 7; ++tick)
    {
        TickOnce(f, state);
    }

    QCOMPARE(f.flightPlanSource.latestRequests, 0);

    f.aircraft.flightPlanDiffersFromTheOfp = true;

    TickOnce(f, state);
    QCOMPARE(f.flightPlanSource.latestRequests, 1);
    QVERIFY(Logged(f, kDifferingPlanFetch));

    for (int tick = 0; tick < 29; ++tick)
    {
        TickOnce(f, state);
    }

    QCOMPARE(f.flightPlanSource.latestRequests, 1);

    TickOnce(f, state);
    QCOMPARE(f.flightPlanSource.latestRequests, 2);

    f.aircraft.flightPlanDiffersFromTheOfp = false;
    f.aircraft.flightPlanLoaded = true;
    f.aircraft.plannedFuelKg = 12971.0;
    f.status.flightPlanStatus = FlightPlanStatus::Ready;
    f.gsxService.simbriefLoaded = true;
    f.gsxService.simbriefGeneration = 1;

    ++f.ctx.data.stateTickCount;
    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::WaitingPowerOn);
    QCOMPARE(f.flightPlanSource.latestRequests, 2);
    QCOMPARE(f.ctx.data.plannedFuelKg, 12971.0);
}

void WaitingFlightPlanStateTest::neverFetchesTheLatestPlanWhileTheAircraftHasNoPlan()
{
    TurnaroundStateFixture f;
    WaitingFlightPlanState state;

    f.status.flightPlanStatus = FlightPlanStatus::Ready;

    for (int tick = 0; tick < 95; ++tick)
    {
        TickOnce(f, state);
    }

    QCOMPARE(f.flightPlanSource.latestRequests, 0);
    QVERIFY(!Logged(f, kDifferingPlanFetch));
}

void WaitingFlightPlanStateTest::holdsADifferingPlanWithoutAFlightPlanSource()
{
    TurnaroundStateFixture f;
    WaitingFlightPlanState state;

    f.ctx.flightPlanSource = nullptr;
    f.status.flightPlanStatus = FlightPlanStatus::Ready;
    f.aircraft.flightPlanDiffersFromTheOfp = true;

    for (int tick = 0; tick < 65; ++tick)
    {
        TickOnce(f, state);
    }

    QVERIFY(!Logged(f, kDifferingPlanFetch));
}

void WaitingFlightPlanStateTest::leavesTheFetchToTheRefusalFlowWhileItAwaitsTheLatestPlan()
{
    TurnaroundStateFixture f;
    WaitingFlightPlanState state;

    ArrangeTheRefusedPlan(f);

    TickUntilTheRetry(f, state);
    GsxAcceptsTheRegeneratedPlan(f);
    TickOnce(f, state);
    QCOMPARE(f.flightPlanSource.latestRequests, 1);

    f.aircraft.flightPlanLoaded = false;
    f.aircraft.flightPlanDiffersFromTheOfp = true;

    for (int tick = 0; tick < 65; ++tick)
    {
        TickOnce(f, state);
    }

    QCOMPARE(f.flightPlanSource.latestRequests, 1);
    QVERIFY(!Logged(f, kDifferingPlanFetch));
}

void WaitingFlightPlanStateTest::ignoresTheRefusalGsxPublishedBeforeTheRequest()
{
    TurnaroundStateFixture f;
    WaitingFlightPlanState state;

    f.status.flightPlanStatus = FlightPlanStatus::Ready;
    f.gsxService.simbriefError = "No SimBrief plan loaded";

    while (f.ctx.data.stateTickCount < 18)
    {
        TickOnce(f, state);
    }

    QCOMPARE(f.menuGateway.simbriefLoadCalls, 0);

    f.aircraft.flightPlanLoaded = true;
    f.aircraft.plannedFuelKg = 5360.0;
    f.aircraft.plannedPax = 104;

    for (int tick = 0; tick < 3; ++tick)
    {
        TickOnce(f, state);
    }

    QCOMPARE(f.menuGateway.simbriefLoadCalls, 1);

    GsxAcceptsTheRegeneratedPlan(f);

    ++f.ctx.data.stateTickCount;
    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::WaitingPowerOn);
    QCOMPARE(f.menuGateway.simbriefLoadCalls, 1);
    QCOMPARE(f.flightPlanSource.latestRequests, 0);
    QVERIFY(!f.ctx.data.flightPlanRefused);
    QVERIFY(!Logged(f, "refused"));
    QCOMPARE(f.ctx.data.plannedFuelKg, 5360.0);
    QCOMPARE(f.ctx.data.plannedPassengers, 104);
}

void WaitingFlightPlanStateTest::retriesOnTheTenthTickAfterTheRequest()
{
    for (const int requestTick : {1, 5, 19, 27})
    {
        TurnaroundStateFixture f;
        WaitingFlightPlanState state;

        while (f.ctx.data.stateTickCount < requestTick - 1)
        {
            TickOnce(f, state);
        }

        f.aircraft.flightPlanLoaded = true;

        TickOnce(f, state);
        QCOMPARE(f.menuGateway.simbriefLoadCalls, 1);

        for (int tick = 0; tick < 9; ++tick)
        {
            TickOnce(f, state);
        }

        QVERIFY(!Logged(f, "not loaded after"));

        TickOnce(f, state);
        QVERIFY(Logged(f, "GSX Simbrief plan not loaded after 10 seconds"));
        QCOMPARE(f.menuGateway.simbriefLoadCalls, 1);

        TickOnce(f, state);
        QCOMPARE(f.menuGateway.simbriefLoadCalls, 2);
    }
}

void WaitingFlightPlanStateTest::flagsTheRefusalGsxPublishesAtTheRetry()
{
    TurnaroundStateFixture f;
    WaitingFlightPlanState state;

    ArrangeTheRefusedPlan(f);
    f.gsxService.simbriefError.clear();

    TickOnce(f, state);
    QCOMPARE(f.menuGateway.simbriefLoadCalls, 1);

    f.gsxService.simbriefError = kRouteRefusal;

    for (int tick = 0; tick < 9; ++tick)
    {
        TickOnce(f, state);
    }

    QVERIFY(!f.ctx.data.flightPlanRefused);

    TickOnce(f, state);
    QVERIFY(f.ctx.data.flightPlanRefused);
    QVERIFY(Logged(f, std::string("GSX refused the SimBrief plan: ") + kRouteRefusal));

    GsxAcceptsTheRegeneratedPlan(f);

    TickOnce(f, state);
    QCOMPARE(f.flightPlanSource.latestRequests, 1);
    QVERIFY(Logged(f, "GSX accepted the SimBrief plan after refusing it: fetching the latest OFP before capturing it"));
}

void WaitingFlightPlanStateTest::reloadsGsxWhenTheDifferingPlanFetchBringsANewOfp()
{
    TurnaroundStateFixture f;
    WaitingFlightPlanState state;

    FetchTheNewOfpWhileGsxServesThePreviousFlight(f, state);
    QCOMPARE(f.menuGateway.simbriefLoadCalls, 0);

    TickOnce(f, state);
    QCOMPARE(f.menuGateway.simbriefLoadCalls, 1);
    QVERIFY(Logged(f, "Asking GSX to reload SimBrief: it must serve a generation newer than 1"));

    for (int tick = 0; tick < 8; ++tick)
    {
        TickOnce(f, state);
    }

    QCOMPARE(f.menuGateway.simbriefLoadCalls, 1);
    QCOMPARE(f.ctx.data.plannedFuelKg, 0.0);

    f.gsxService.simbriefGeneration = 2;

    ++f.ctx.data.stateTickCount;
    const auto transition = state.Evaluate(f.ctx);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::WaitingPowerOn);
    QCOMPARE(f.menuGateway.simbriefLoadCalls, 1);
    QCOMPARE(f.flightPlanSource.latestRequests, 1);
    QCOMPARE(f.ctx.data.plannedFuelKg, kNewFlightFuelKg);
}

void WaitingFlightPlanStateTest::retriesTheReloadOnTheTenthTickWhileGsxServesTheOldGeneration()
{
    TurnaroundStateFixture f;
    WaitingFlightPlanState state;

    FetchTheNewOfpWhileGsxServesThePreviousFlight(f, state);

    TickOnce(f, state);
    QCOMPARE(f.menuGateway.simbriefLoadCalls, 1);

    for (int tick = 0; tick < 9; ++tick)
    {
        TickOnce(f, state);
    }

    QVERIFY(!Logged(f, "not loaded after"));

    TickOnce(f, state);
    QVERIFY(Logged(f, "GSX Simbrief plan not loaded after 10 seconds"));
    QCOMPARE(f.menuGateway.simbriefLoadCalls, 1);

    TickOnce(f, state);
    QCOMPARE(f.menuGateway.simbriefLoadCalls, 2);

    f.gsxService.simbriefGeneration = 2;

    ++f.ctx.data.stateTickCount;
    QVERIFY(state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.ctx.data.plannedFuelKg, kNewFlightFuelKg);
}

void WaitingFlightPlanStateTest::waitsPastANewerGenerationGsxRefused()
{
    TurnaroundStateFixture f;
    WaitingFlightPlanState state;

    FetchTheNewOfpWhileGsxServesThePreviousFlight(f, state);

    TickOnce(f, state);
    QCOMPARE(f.menuGateway.simbriefLoadCalls, 1);

    f.gsxService.simbriefGeneration = 2;
    f.gsxService.simbriefError = kRouteRefusal;

    TickOnce(f, state);
    QCOMPARE(f.ctx.data.plannedFuelKg, 0.0);

    f.gsxService.simbriefGeneration = 3;
    f.gsxService.simbriefError.clear();

    ++f.ctx.data.stateTickCount;
    QVERIFY(state.Evaluate(f.ctx).has_value());
    QCOMPARE(f.ctx.data.plannedFuelKg, kNewFlightFuelKg);
}

QTEST_APPLESS_MAIN(WaitingFlightPlanStateTest)

#include "tst_waiting_flight_plan_state.moc"
