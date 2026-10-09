#include <QtTest/QTest>

#include <array>
#include <utility>
#include <vector>

#include "TestDoubles.h"
#include "../src/infrastructure/gsx/GsxStateService.h"
#include "../src/infrastructure/gsx/GsxLVars.h"

namespace
{
    using namespace gsx::lvars;

    constexpr auto kSimOnGround = "SIM ON GROUND";
    constexpr auto kGroundVelocity = "GROUND VELOCITY";

    struct ServiceStateLVar
    {
        const char* lvar;
        GsxState state;
    };

    constexpr std::array kServicesThatEndThroughCompleted = {
        ServiceStateLVar{kRefuelingState, GsxState::Refueling},
        ServiceStateLVar{kBoardingState, GsxState::Boarding},
        ServiceStateLVar{kDeboardingState, GsxState::Deboarding},
    };

    void ObserveFor(GsxStateService& gsx, FakeVariableGateway& gateway, const char* stateLVar,
                    const double couatlStarted, const double state, const int ticks)
    {
        gateway.lvars[kCouatlStarted] = couatlStarted;
        gateway.lvars[stateLVar] = state;

        for (int tick = 0; tick < ticks; ++tick)
        {
            gsx.Observe();
        }
    }

    double AsLVar(const GsxStateStatus status)
    {
        return static_cast<double>(status);
    }

    constexpr std::array kResumeReadings = {
        kCouatlStarted,
        kRefuelingState,
        kBoardingState,
        kPushbackVehicleState,
        kDeboardingState,
        kDeiceState,
        kPushbackStatus,
    };

    struct CounterReading
    {
        const char* stateLVar;
        const char* counterLVar;
        double (*read)(GsxStateService& gsx);
    };

    double ReadBoardedPassengers(GsxStateService& gsx)
    {
        return gsx.GetBoardedPassengers();
    }

    double ReadDeboardedPassengers(GsxStateService& gsx)
    {
        return gsx.GetDeboardedPassengers();
    }

    double ReadBoardingCargo(GsxStateService& gsx)
    {
        return gsx.GetBoardingCargoPercent();
    }

    double ReadDeboardingCargo(GsxStateService& gsx)
    {
        return gsx.GetDeboardingCargoPercent();
    }

    constexpr std::array kCounterReadings = {
        CounterReading{.stateLVar = kBoardingState, .counterLVar = kNumPassengersBoardingTotal,
                       .read = ReadBoardedPassengers},
        CounterReading{.stateLVar = kDeboardingState, .counterLVar = kNumPassengersDeboardingTotal,
                       .read = ReadDeboardedPassengers},
        CounterReading{.stateLVar = kBoardingState, .counterLVar = kBoardingCargoPercent,
                       .read = ReadBoardingCargo},
        CounterReading{.stateLVar = kDeboardingState, .counterLVar = kDeboardingCargoPercent,
                       .read = ReadDeboardingCargo},
    };

    MemoryBag MemoryOfAServiceLeftAt(const char* stateLVar, const GsxStateStatus status)
    {
        FakeVariableGateway gateway;
        GsxStateService gsx(&gateway);

        ObserveFor(gsx, gateway, stateLVar, 1.0, AsLVar(status), 3);

        return gsx.TakeMemory();
    }

    int BoardedWithTheTotalAt(GsxStateService& gsx, FakeVariableGateway& gateway, const double total)
    {
        gateway.lvars[kNumPassengersBoardingTotal] = total;

        return gsx.GetBoardedPassengers();
    }

    MemoryBag MemoryOfARemoteStandBoardingAt92()
    {
        FakeVariableGateway gateway;
        GsxStateService gsx(&gateway);

        gateway.lvars[kBoardingState] = AsLVar(GsxStateStatus::Active);
        for (const double total : {0.0, 40.0, 0.0, 40.0, 0.0})
        {
            static_cast<void>(BoardedWithTheTotalAt(gsx, gateway, total));
        }

        static_cast<void>(BoardedWithTheTotalAt(gsx, gateway, 12.0));

        return gsx.TakeMemory();
    }

    MemoryBag MemoryOfEverythingTheServiceKeeps()
    {
        FakeVariableGateway gateway;
        GsxStateService gsx(&gateway);

        gateway.lvars[kCouatlStarted] = 1.0;
        gateway.lvars[kGpuConnected] = 0.0;
        gateway.lvars[kRefuelingState] = AsLVar(GsxStateStatus::Active);
        gateway.lvars[kBoardingState] = AsLVar(GsxStateStatus::Active);
        gateway.lvars[kDeboardingState] = AsLVar(GsxStateStatus::Active);
        gateway.lvars[kBoardingCargoPercent] = 0.0;
        gateway.lvars[kDeboardingCargoPercent] = 0.0;
        gateway.lvars[kNumPassengersDeboardingTotal] = 0.0;
        gsx.Observe();
        gsx.TakeOverFuelAndPayload();
        static_cast<void>(BoardedWithTheTotalAt(gsx, gateway, 0.0));
        static_cast<void>(BoardedWithTheTotalAt(gsx, gateway, 40.0));
        static_cast<void>(BoardedWithTheTotalAt(gsx, gateway, 0.0));
        static_cast<void>(BoardedWithTheTotalAt(gsx, gateway, 9.0));
        gateway.lvars[kBoardingCargoPercent] = 35.0;
        static_cast<void>(gsx.GetBoardingCargoPercent());
        static_cast<void>(gsx.GetDeboardedPassengers());
        gateway.lvars[kNumPassengersDeboardingTotal] = 18.0;
        static_cast<void>(gsx.GetDeboardedPassengers());
        gateway.lvars[kDeboardingCargoPercent] = 20.0;
        static_cast<void>(gsx.GetDeboardingCargoPercent());
        gsx.Observe();

        return gsx.TakeMemory();
    }
}

class GsxInterfaceTest final : public QObject
{
    Q_OBJECT

private slots:
    static void availabilityFollowsCouatlFlag();
    static void mapsServiceStateLVars();
    static void recordsExplicitCompletedState();
    static void aServiceTheCouatlDropsIsNotRecordedAsCompleted();
    static void aServiceThePilotEndsWithALiveCouatlIsRecordedAsCompleted();
    static void aCouatlDeathStopsCountingOnceTheServiceRunsAgain();
    static void aCouatlDeathNoObserveSawIsStillADrop();
    static void gsxCountsAsDownOnlyWhenTheCouatlFlagDippedSinceTheLastObserve();
    static void aServiceThatPassesThroughCompletedStaysRecordedBackAtIdle();
    static void doesNotRecordCompletionWithoutActiveState();
    static void latchAdvancesWithoutAnyoneAskingForTheStatus();
    static void aDeiceReturningToIdleIsRecordedAsCompleted();
    static void readingTheStatusDoesNotAdvanceTheLatch();
    static void readsFuelHoseAndPassengerCounts();
    static void detectsSimbriefLoaded();
    static void resetClearsCompletionFlags();
    static void detectsWaitingForEngines();
    static void detectsPushbackStarted();
    static void detectsPushbackFinished();
    static void detectsRepositioning();
    static void aServiceUnderwayIsUnknownUntilTheThreeStateLVarsArrive();
    static void aServiceUnderwayIsRequestedActiveOrCompletingOnAnyOfTheThree();
    static void cargoPercentReadsLVars();
    static void boardingCargoPercentIgnoresTheStalePercentUntilItMoves();
    static void deboardingCargoPercentIgnoresTheStalePercentUntilItMoves();
    static void cargoLoadingReadsLVars();
    static void loaderWaitingForDoorNamesTheMainDeckOneWhenSeveralWait();
    static void aLoaderAtAHoldIsOneInPositionOrLoading();
    static void jetwayAndStairsAvailability();
    static void jetwayAndStairsUnavailableUntilLVarsReceived();
    static void jetwayAndStairsUnavailableWhileGsxStillEvaluatesTheParking();
    static void stairsAreAvailableWhenTheLVarReadsZeroAndTheRemoteApiListsThemCallable();
    static void stairsTheLVarRulesOutStayUnavailableWhateverTheRemoteApiLists();
    static void theJetwayNeverFollowsTheRemoteApiListing();
    static void serviceVehicleActiveFollowsTheStairsVehicles();
    static void goodEngineStartAssumedEnabledUntilLVarReceived();
    static void aircraftOnGroundFollowsSimVar();
    static void groundSpeedReadsZeroUntilTheSimVarArrives();
    static void boardedPassengersAccumulatesAcrossResets();
    static void deboardedPassengersAccumulatesAcrossResets();
    static void boardedPassengersIgnoresStaleTotalBeforeBoardingStarts();
    static void deboardedPassengersIgnoresStaleTotalBeforeDeboardingStarts();
    static void boardedPassengersIgnoresTheStaleTotalOnTheFirstActiveTick();
    static void deboardedPassengersIgnoresTheStaleTotalOnTheFirstActiveTick();
    static void aBoardingFoundUnderwayTrustsItsFirstReading();
    static void aBoardingFoundUnderwayFollowsTheCountFromItsFirstReading();
    static void aStateThatWasNeverObservedAsActiveKeepsTheLeftoverRule();
    static void aStateLVarThatHasNotArrivedIsNotAnObservationOfTheService();
    static void aDeboardingFoundUnderwayTrustsItsFirstReading();
    static void aServiceFoundUnderwayStopsBeingFoundOnceItLeavesActive();
    static void theTurnForgetsTheCompletionsOfTheLastTurnaround();
    static void theTurnKeepsTheReadingSoAServiceEndingAcrossItStillCounts();
    static void boardedPassengersStartOverAfterTheTurn();
    static void boardingCargoStartsOverAfterTheTurn();
    static void takeOverFuelAndPayloadClearsAutomationLVars();
    static void reassertsTakeoverAfterCouatlReset();
    static void refuelCounterComesFromFuelCounterLvar();
    static void gpuStatusPrefersTheRemoteApiOverALyingLVar();
    static void gpuStatusFallsBackToTheLVarsWithoutTheRemoteApi();
    static void gpuStatusFollowsStateLVar();
    static void gpuStatusConnectedWhenConnectedFlagSetDespiteState();
    static void gpuStatusIgnoresAConnectedFlagInheritedFromTheLastTurnaround();
    static void gpuStatusDoesNotArmTheConnectedFlagBeforeItArrives();
    static void serviceInProgressFollowsRemoteStateRaw();
    static void serviceInProgressFalseWhenAbsentOrNoRemote();
    static void pushbackIsOfferedUnlessTheApronVerdictSaysOtherwise();
    static void remoteApiCountsAsConnectedOnlyWhileTheLinkIsUp();
    static void aRestoredBoardingKeepsTheBusesTheLastProcessCounted();
    static void aRestoredBoardingKeepsItsCountWhileTheCounterHasNotArrived();
    static void aClosedServiceKeepsItsCountersAcrossTheMemory();
    static void aCounterThatHasNotArrivedReportsNothingAndTakesNoBaseline();
    static void aRestoredStatusSurvivesObservationsBeforeTheStateLVarArrives();
    static void aCouatlFlagThatHasNotArrivedIsNotACouatlDeath();
    static void aServiceFoundAvailableAfterTheMemoryIsRecordedAsCompleted();
    static void aServiceRestoredAfterACouatlRestartIsInterruptedNotCompleted();
    static void aPushbackAndADeiceRestoredUnderwayAreNeverRecordedCompletedByWhatTheyFindIdle();
    static void aPushbackSavedWhileTheTugApproachesAndReadMidPushIsNotRecordedCompleted();
    static void aRestoredPushbackAndDeiceSeenActiveAgainFollowTheLiveRule();
    static void aRestartIsNotWrittenIntoTheMemoryBeforeAReadingConfirmsIt();
    static void aRestartDoesNotMarkAPushbackOrADeiceAsHavingLostItsCouatl();
    static void restoringAnEmptyMemoryForgetsWhatTheServiceSawBefore();
    static void aRestoredCargoPercentIsClampedLikeTheCounts();
    static void aServiceRestoredAsRequestedAndFoundAvailableCountsAsCompleted();
    static void theRestoredMarkClearsAtTheFirstReadingSoTheCompletionCountsOnce();
    static void aLiveServiceGoingFromRequestedToAvailableIsNotCompleted();
    static void aCompletedServiceStaysCompletedAcrossTheMemory();
    static void aCouatlDeathTheLastProcessSawSurvivesTheMemory();
    static void aRestoredCargoReadingStillIgnoresTheStalePercent();
    static void aServiceThatGoesThroughCompletingStraightToIdleIsRecordedAsCompleted();
    static void restoringBeforeOrAfterTheFirstReadingGivesTheSameVerdict();
    static void theMemoryRestoresToTheMemoryItWasTakenFrom();
    static void theMemoryIgnoresUnknownNames();
    static void anEmptyMemoryRestoresTheStateOfAReset();
    static void theGpuClearSightingSurvivesTheMemory();
    static void theTakeoverOfFuelAndPayloadSurvivesTheMemory();
    static void reassertingBeforeTheAutomationFlagsArriveStillTakesFuelAndPayloadBack();
    static void theResumeReadingsHaveArrivedOnlyWhenEveryOneHas();
    static void theResumeReadingsHaveNotArrivedWhenAnyOneIsMissingAlone();
    static void askingIfTheResumeReadingsArrivedRequestsEveryOne();
};

void GsxInterfaceTest::availabilityFollowsCouatlFlag()
{
    FakeVariableGateway gateway;

    const GsxStateService gsx(&gateway);

    QVERIFY(!gsx.IsAvailable());

    gateway.lvars[kCouatlStarted] = 1.0;

    QVERIFY(gsx.IsAvailable());
}

void GsxInterfaceTest::mapsServiceStateLVars()
{
    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    gateway.lvars[kRefuelingState] = static_cast<double>(GsxStateStatus::Active);
    gateway.lvars[kBoardingState] = static_cast<double>(GsxStateStatus::Completed);
    gateway.lvars[kDeiceState] = static_cast<double>(GsxStateStatus::Requested);

    QCOMPARE(gsx.GetStateStatus(GsxState::Refueling), GsxStateStatus::Active);
    QCOMPARE(gsx.GetStateStatus(GsxState::Boarding), GsxStateStatus::Completed);
    QCOMPARE(gsx.GetStateStatus(GsxState::Pushback), GsxStateStatus::Unavailable);
    QCOMPARE(gsx.GetStateStatus(GsxState::Deice), GsxStateStatus::Requested);
}

void GsxInterfaceTest::recordsExplicitCompletedState()
{
    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    gateway.lvars[kRefuelingState] = static_cast<double>(GsxStateStatus::Completed);
    gsx.Observe();

    QCOMPARE(gsx.GetStateStatus(GsxState::Refueling), GsxStateStatus::Completed);
    QVERIFY(gsx.WasStateCompleted(GsxState::Refueling));
}

void GsxInterfaceTest::aServiceTheCouatlDropsIsNotRecordedAsCompleted()
{
    for (const auto& [stateLVar, service] : kServicesThatEndThroughCompleted)
    {
        FakeVariableGateway gateway;
        GsxStateService gsx(&gateway);

        ObserveFor(gsx, gateway, stateLVar, 1.0, 5.0, 30);
        ObserveFor(gsx, gateway, stateLVar, 0.0, 5.0, 12);
        ObserveFor(gsx, gateway, stateLVar, 1.0, 5.0, 5);
        ObserveFor(gsx, gateway, stateLVar, 1.0, 1.0, 30);

        QVERIFY2(!gsx.WasStateCompleted(service), stateLVar);
    }
}

void GsxInterfaceTest::aServiceThePilotEndsWithALiveCouatlIsRecordedAsCompleted()
{
    for (const auto& [stateLVar, service] : kServicesThatEndThroughCompleted)
    {
        FakeVariableGateway gateway;
        GsxStateService gsx(&gateway);

        ObserveFor(gsx, gateway, stateLVar, 1.0, 5.0, 30);
        ObserveFor(gsx, gateway, stateLVar, 1.0, 1.0, 5);

        QVERIFY2(gsx.WasStateCompleted(service), stateLVar);
    }
}

void GsxInterfaceTest::aCouatlDeathStopsCountingOnceTheServiceRunsAgain()
{
    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    ObserveFor(gsx, gateway, kBoardingState, 1.0, 5.0, 10);
    ObserveFor(gsx, gateway, kBoardingState, 0.0, 5.0, 12);
    ObserveFor(gsx, gateway, kBoardingState, 1.0, 1.0, 5);

    QVERIFY(!gsx.WasStateCompleted(GsxState::Boarding));

    ObserveFor(gsx, gateway, kBoardingState, 1.0, 4.0, 3);
    ObserveFor(gsx, gateway, kBoardingState, 1.0, 5.0, 10);
    ObserveFor(gsx, gateway, kBoardingState, 1.0, 1.0, 5);

    QVERIFY(gsx.WasStateCompleted(GsxState::Boarding));
}

void GsxInterfaceTest::aCouatlDeathNoObserveSawIsStillADrop()
{
    for (const auto& [stateLVar, service] : kServicesThatEndThroughCompleted)
    {
        FakeVariableGateway gateway;
        GsxStateService gsx(&gateway);

        ObserveFor(gsx, gateway, stateLVar, 1.0, 5.0, 30);

        gateway.lvarSpans[kCouatlStarted] = LVarSpan{0.0, 1.0, true};
        ObserveFor(gsx, gateway, stateLVar, 1.0, 1.0, 5);

        QVERIFY2(!gsx.WasStateCompleted(service), stateLVar);
    }
}

void GsxInterfaceTest::gsxCountsAsDownOnlyWhenTheCouatlFlagDippedSinceTheLastObserve()
{
    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    gateway.lvars[kCouatlStarted] = 1.0;
    gsx.Observe();

    QVERIFY(!gsx.WasGsxDownSinceLastObserve());

    gateway.lvarSpans[kCouatlStarted] = LVarSpan{0.0, 1.0, true};
    gsx.Observe();

    QVERIFY(gsx.WasGsxDownSinceLastObserve());

    gsx.Observe();

    QVERIFY(!gsx.WasGsxDownSinceLastObserve());
}

void GsxInterfaceTest::aServiceThatPassesThroughCompletedStaysRecordedBackAtIdle()
{
    for (const auto& [stateLVar, service] : kServicesThatEndThroughCompleted)
    {
        FakeVariableGateway gateway;
        GsxStateService gsx(&gateway);

        ObserveFor(gsx, gateway, stateLVar, 1.0, 5.0, 63);
        ObserveFor(gsx, gateway, stateLVar, 1.0, 7.0, 2);
        ObserveFor(gsx, gateway, stateLVar, 1.0, 6.0, 11);
        ObserveFor(gsx, gateway, stateLVar, 1.0, 1.0, 20);

        QVERIFY2(gsx.WasStateCompleted(service), stateLVar);
    }
}

void GsxInterfaceTest::doesNotRecordCompletionWithoutActiveState()
{
    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    gateway.lvars[kBoardingState] = static_cast<double>(GsxStateStatus::Callable);
    gsx.Observe();

    QCOMPARE(gsx.GetStateStatus(GsxState::Boarding), GsxStateStatus::Callable);
    QVERIFY(!gsx.WasStateCompleted(GsxState::Boarding));
}

void GsxInterfaceTest::readsFuelHoseAndPassengerCounts()
{
    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    gateway.lvars[kFuelHoseConnected] = 1.0;
    gateway.lvars[kMaxPassengers] = 215.0;
    gateway.lvars[kBoardingState] = static_cast<double>(GsxStateStatus::Active);
    gateway.lvars[kNumPassengersBoardingTotal] = 0.0;

    QVERIFY(gsx.IsFuelHoseConnected());
    QCOMPARE(gsx.GetPlannedPassengers(), 215);
    QCOMPARE(gsx.GetBoardedPassengers(), 0);

    gateway.lvars[kNumPassengersBoardingTotal] = 130.0;

    QCOMPARE(gsx.GetBoardedPassengers(), 130);
}

void GsxInterfaceTest::detectsSimbriefLoaded()
{
    FakeVariableGateway gateway;
    const GsxStateService gsx(&gateway);

    QVERIFY(!gsx.IsSimbriefLoaded());

    gateway.lvars[kSimbriefSuccess] = 1.0;

    QVERIFY(gsx.IsSimbriefLoaded());
}

void GsxInterfaceTest::resetClearsCompletionFlags()
{
    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    gateway.lvars[kBoardingState] = static_cast<double>(GsxStateStatus::Active);
    gsx.Observe();
    gateway.lvars[kBoardingState] = static_cast<double>(GsxStateStatus::Completed);
    gsx.Observe();

    QVERIFY(gsx.WasStateCompleted(GsxState::Boarding));

    gsx.Reset();

    QVERIFY(!gsx.WasStateCompleted(GsxState::Boarding));
}

void GsxInterfaceTest::detectsWaitingForEngines()
{
    FakeVariableGateway gateway;
    const GsxStateService gsx(&gateway);

    QVERIFY(!gsx.IsWaitingForEngines());

    gateway.lvars[kPushbackStatus] = 8.0;

    QVERIFY(gsx.IsWaitingForEngines());

    gateway.lvars[kPushbackStatus] = 0.0;

    QVERIFY(!gsx.IsWaitingForEngines());
}

void GsxInterfaceTest::detectsPushbackStarted()
{
    FakeVariableGateway gateway;
    const GsxStateService gsx(&gateway);

    QVERIFY(!gsx.HasPushbackStarted());

    gateway.lvars[kPushbackStatus] = 4.0;

    QVERIFY(!gsx.HasPushbackStarted());

    gateway.lvars[kPushbackStatus] = 6.0;

    QVERIFY(gsx.HasPushbackStarted());

    gateway.lvars[kPushbackStatus] = 8.0;

    QVERIFY(gsx.HasPushbackStarted());
}

void GsxInterfaceTest::detectsPushbackFinished()
{
    FakeVariableGateway gateway;
    const GsxStateService gsx(&gateway);

    gateway.lvars[kPushbackStatus] = 0.0;
    gateway.lvars[kPushbackVehicleState] = static_cast<double>(GsxStateStatus::Active);

    QVERIFY(!gsx.IsPushbackFinished());

    gateway.lvars[kPushbackStatus] = 6.0;

    QVERIFY(!gsx.IsPushbackFinished());

    gateway.lvars[kPushbackStatus] = 11.0;

    QVERIFY(gsx.IsPushbackFinished());

    gateway.lvars[kPushbackStatus] = 0.0;
    gateway.lvars[kPushbackVehicleState] = static_cast<double>(GsxStateStatus::Completed);

    QVERIFY(!gsx.IsPushbackFinished());
}

void GsxInterfaceTest::aServiceUnderwayIsUnknownUntilTheThreeStateLVarsArrive()
{
    FakeVariableGateway gateway;
    const GsxStateService gsx(&gateway);

    QVERIFY(!gsx.HasServiceUnderway().has_value());

    gateway.lvars[kRefuelingState] = static_cast<double>(GsxStateStatus::Active);

    QVERIFY(!gsx.HasServiceUnderway().has_value());

    gateway.lvars[kBoardingState] = static_cast<double>(GsxStateStatus::Callable);

    QVERIFY(!gsx.HasServiceUnderway().has_value());

    gateway.lvars[kDeboardingState] = static_cast<double>(GsxStateStatus::Callable);

    QVERIFY(gsx.HasServiceUnderway() == std::optional(true));
}

void GsxInterfaceTest::aServiceUnderwayIsRequestedActiveOrCompletingOnAnyOfTheThree()
{
    for (const auto& underway : kServicesThatEndThroughCompleted)
    {
        FakeVariableGateway gateway;
        const GsxStateService gsx(&gateway);

        for (const auto& idle : kServicesThatEndThroughCompleted)
        {
            gateway.lvars[idle.lvar] = static_cast<double>(GsxStateStatus::Callable);
        }

        QVERIFY(gsx.HasServiceUnderway() == std::optional(false));

        for (const auto status : {GsxStateStatus::Requested, GsxStateStatus::Active, GsxStateStatus::Completing})
        {
            gateway.lvars[underway.lvar] = static_cast<double>(status);

            QVERIFY2(gsx.HasServiceUnderway().value_or(false), underway.lvar);
        }

        gateway.lvars[underway.lvar] = static_cast<double>(GsxStateStatus::Completed);

        QVERIFY2(gsx.HasServiceUnderway() == std::optional(false), underway.lvar);
    }
}

void GsxInterfaceTest::detectsRepositioning()
{
    FakeVariableGateway gateway;
    const GsxStateService gsx(&gateway);

    QVERIFY(!gsx.IsRepositioning());

    gateway.lvars[kRepositioning] = 1.0;

    QVERIFY(gsx.IsRepositioning());

    gateway.lvars[kRepositioning] = 2.0;

    QVERIFY(!gsx.IsRepositioning());
}

void GsxInterfaceTest::cargoPercentReadsLVars()
{
    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    gateway.lvars[kBoardingState] = static_cast<double>(GsxStateStatus::Active);
    gateway.lvars[kDeboardingState] = static_cast<double>(GsxStateStatus::Active);
    gateway.lvars[kBoardingCargoPercent] = 0.0;
    gateway.lvars[kDeboardingCargoPercent] = 0.0;

    QCOMPARE(gsx.GetBoardingCargoPercent(), 0.0);
    QCOMPARE(gsx.GetDeboardingCargoPercent(), 0.0);

    gateway.lvars[kBoardingCargoPercent] = 42.5;
    gateway.lvars[kDeboardingCargoPercent] = 17.0;

    QCOMPARE(gsx.GetBoardingCargoPercent(), 42.5);
    QCOMPARE(gsx.GetDeboardingCargoPercent(), 17.0);
}

void GsxInterfaceTest::boardingCargoPercentIgnoresTheStalePercentUntilItMoves()
{
    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    gateway.lvars[kBoardingState] = static_cast<double>(GsxStateStatus::Callable);
    gateway.lvars[kBoardingCargoPercent] = 92.0;

    QCOMPARE(gsx.GetBoardingCargoPercent(), 0.0);

    gateway.lvars[kBoardingState] = static_cast<double>(GsxStateStatus::Active);

    QCOMPARE(gsx.GetBoardingCargoPercent(), 0.0);
    QCOMPARE(gsx.GetBoardingCargoPercent(), 0.0);

    gateway.lvars[kBoardingCargoPercent] = 0.0;

    QCOMPARE(gsx.GetBoardingCargoPercent(), 0.0);

    gateway.lvars[kBoardingCargoPercent] = 35.0;

    QCOMPARE(gsx.GetBoardingCargoPercent(), 35.0);
}

void GsxInterfaceTest::deboardingCargoPercentIgnoresTheStalePercentUntilItMoves()
{
    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    gateway.lvars[kDeboardingState] = static_cast<double>(GsxStateStatus::Callable);
    gateway.lvars[kDeboardingCargoPercent] = 67.0;

    QCOMPARE(gsx.GetDeboardingCargoPercent(), 0.0);

    gateway.lvars[kDeboardingState] = static_cast<double>(GsxStateStatus::Active);

    QCOMPARE(gsx.GetDeboardingCargoPercent(), 0.0);
    QCOMPARE(gsx.GetDeboardingCargoPercent(), 0.0);

    gateway.lvars[kDeboardingCargoPercent] = 0.0;

    QCOMPARE(gsx.GetDeboardingCargoPercent(), 0.0);

    gateway.lvars[kDeboardingCargoPercent] = 40.0;

    QCOMPARE(gsx.GetDeboardingCargoPercent(), 40.0);
}

void GsxInterfaceTest::cargoLoadingReadsLVars()
{
    FakeVariableGateway gateway;
    const GsxStateService gsx(&gateway);

    QVERIFY(!gsx.IsLoadingCargo());
    QCOMPARE(gsx.GetLoaderWaitingForDoor(), CargoLoader::None);

    gateway.lvars[kBoardingCargo] = 1.0;
    gateway.lvars[kBaggageLoaderMainState] = gsx::states::kLoaderWaitingForDoor;

    QVERIFY(gsx.IsLoadingCargo());
    QCOMPARE(gsx.GetLoaderWaitingForDoor(), CargoLoader::MainDeck);

    gateway.lvars[kBaggageLoaderMainState] = gsx::states::kLoaderRetracting;

    QCOMPARE(gsx.GetLoaderWaitingForDoor(), CargoLoader::None);

    gateway.lvars[kBaggageLoaderFrontState] = gsx::states::kLoaderWaitingForDoor;

    QCOMPARE(gsx.GetLoaderWaitingForDoor(), CargoLoader::Front);
}

void GsxInterfaceTest::loaderWaitingForDoorNamesTheMainDeckOneWhenSeveralWait()
{
    FakeVariableGateway gateway;
    const GsxStateService gsx(&gateway);

    gateway.lvars[kBaggageLoaderRearState] = gsx::states::kLoaderWaitingForDoor;

    QCOMPARE(gsx.GetLoaderWaitingForDoor(), CargoLoader::Rear);

    gateway.lvars[kBaggageLoaderFrontState] = gsx::states::kLoaderWaitingForDoor;

    QCOMPARE(gsx.GetLoaderWaitingForDoor(), CargoLoader::Rear);

    gateway.lvars[kBaggageLoaderMainState] = gsx::states::kLoaderWaitingForDoor;

    QCOMPARE(gsx.GetLoaderWaitingForDoor(), CargoLoader::MainDeck);
}

void GsxInterfaceTest::aLoaderAtAHoldIsOneInPositionOrLoading()
{
    const std::array loaderStates = {kBaggageLoaderMainState, kBaggageLoaderRearState, kBaggageLoaderFrontState};

    for (const char* const stateLVar : loaderStates)
    {
        FakeVariableGateway gateway;
        const GsxStateService gsx(&gateway);

        QVERIFY(!gsx.IsALoaderAtAHold());

        for (const double state : {1.0, 4.0, 6.0, 10.0})
        {
            gateway.lvars[stateLVar] = state;

            QVERIFY(!gsx.IsALoaderAtAHold());
        }

        gateway.lvars[stateLVar] = gsx::states::kLoaderInPosition;

        QVERIFY(gsx.IsALoaderAtAHold());

        gateway.lvars[stateLVar] = gsx::states::kLoaderLoading;

        QVERIFY(gsx.IsALoaderAtAHold());
    }
}

void GsxInterfaceTest::jetwayAndStairsAvailability()
{
    FakeVariableGateway gateway;
    const GsxStateService gsx(&gateway);

    gateway.lvars[kJetway] = 5.0;
    gateway.lvars[kStairs] = 5.0;

    QVERIFY(gsx.IsJetwayInPlace());
    QVERIFY(gsx.AreStairsInPlace());
    QVERIFY(gsx.IsJetwayAvailable());
    QVERIFY(gsx.AreStairsAvailable());

    gateway.lvars[kJetway] = 2.0;
    gateway.lvars[kStairs] = 2.0;

    QVERIFY(!gsx.IsJetwayInPlace());
    QVERIFY(!gsx.AreStairsInPlace());
    QVERIFY(!gsx.IsJetwayAvailable());
    QVERIFY(!gsx.AreStairsAvailable());

    gateway.lvars[kJetway] = 3.0;

    QVERIFY(gsx.IsJetwayAvailable());
    QVERIFY(!gsx.IsJetwayInPlace());
}

void GsxInterfaceTest::jetwayAndStairsUnavailableUntilLVarsReceived()
{
    FakeVariableGateway gateway;
    const GsxStateService gsx(&gateway);

    QVERIFY(!gsx.IsJetwayAvailable());
    QVERIFY(!gsx.AreStairsAvailable());

    gateway.lvars[kJetway] = 1.0;
    gateway.lvars[kStairs] = 1.0;

    QVERIFY(gsx.IsJetwayAvailable());
    QVERIFY(gsx.AreStairsAvailable());
}

void GsxInterfaceTest::jetwayAndStairsUnavailableWhileGsxStillEvaluatesTheParking()
{
    FakeVariableGateway gateway;
    const GsxStateService gsx(&gateway);

    gateway.lvars[kJetway] = 0.0;
    gateway.lvars[kStairs] = 0.0;

    QVERIFY(!gsx.IsJetwayAvailable());
    QVERIFY(!gsx.AreStairsAvailable());
}

void GsxInterfaceTest::stairsAreAvailableWhenTheLVarReadsZeroAndTheRemoteApiListsThemCallable()
{
    FakeVariableGateway gateway;
    GsxRemoteState remote;
    const GsxStateService gsx(&gateway, &remote);

    gateway.lvars[kStairs] = 0.0;
    gateway.lvars[kJetway] = 2.0;
    remote.connected = true;
    remote.services.push_back(GsxRemoteService{.id = "OperateStairs",
                                               .stateRaw = static_cast<int>(GsxStateStatus::Callable),
                                               .canTrigger = true});

    QVERIFY(gsx.AreStairsAvailable());

    for (const double jetwayReading : {0.0, 1.0, 5.0})
    {
        gateway.lvars[kJetway] = jetwayReading;

        QVERIFY(!gsx.AreStairsAvailable());
    }

    gateway.lvars[kJetway] = 2.0;
    remote.services.front().canTrigger = false;

    QVERIFY(!gsx.AreStairsAvailable());

    remote.services.front().canTrigger = true;
    remote.services.front().stateRaw = static_cast<int>(GsxStateStatus::Requested);

    QVERIFY(!gsx.AreStairsAvailable());

    remote.services.front().stateRaw = static_cast<int>(GsxStateStatus::Callable);
    remote.connected = false;

    QVERIFY(!gsx.AreStairsAvailable());

    remote.connected = true;
    remote.services.clear();

    QVERIFY(!gsx.AreStairsAvailable());
}

void GsxInterfaceTest::stairsTheLVarRulesOutStayUnavailableWhateverTheRemoteApiLists()
{
    FakeVariableGateway gateway;
    GsxRemoteState remote;
    const GsxStateService gsx(&gateway, &remote);

    gateway.lvars[kStairs] = 2.0;
    remote.connected = true;
    remote.services.push_back(GsxRemoteService{.id = "OperateStairs",
                                               .stateRaw = static_cast<int>(GsxStateStatus::Callable),
                                               .canTrigger = true});

    QVERIFY(!gsx.AreStairsAvailable());
}

void GsxInterfaceTest::theJetwayNeverFollowsTheRemoteApiListing()
{
    FakeVariableGateway gateway;
    GsxRemoteState remote;
    const GsxStateService gsx(&gateway, &remote);

    remote.connected = true;
    remote.services.push_back(GsxRemoteService{.id = "OperateJetways",
                                               .stateRaw = static_cast<int>(GsxStateStatus::Callable),
                                               .canTrigger = true});

    for (const double reading : {0.0, 2.0})
    {
        gateway.lvars[kJetway] = reading;

        QVERIFY(!gsx.IsJetwayAvailable());
    }
}

void GsxInterfaceTest::serviceVehicleActiveFollowsTheStairsVehicles()
{
    FakeVariableGateway gateway;
    const GsxStateService gsx(&gateway);

    QVERIFY(!gsx.IsServiceVehicleActive());

    gateway.lvars[kPassengerStairsRearState] = 1.0;

    QVERIFY(!gsx.IsServiceVehicleActive());

    gateway.lvars[kPassengerStairsRearState] = gsx::states::kVehicleApproaching;

    QVERIFY(gsx.IsServiceVehicleActive());

    gateway.lvars[kPassengerStairsRearState] = 0.0;
    gateway.lvars[kPassengerStairsMiddleState] = gsx::states::kVehicleDispatched;

    QVERIFY(gsx.IsServiceVehicleActive());
}

void GsxInterfaceTest::goodEngineStartAssumedEnabledUntilLVarReceived()
{
    FakeVariableGateway gateway;
    const GsxStateService gsx(&gateway);

    QVERIFY(gsx.IsGoodEngineStartConfirmationEnabled());

    gateway.lvars[kGoodEngineStart] = 0.0;

    QVERIFY(!gsx.IsGoodEngineStartConfirmationEnabled());

    gateway.lvars[kGoodEngineStart] = 1.0;

    QVERIFY(gsx.IsGoodEngineStartConfirmationEnabled());
}

void GsxInterfaceTest::aircraftOnGroundFollowsSimVar()
{
    FakeVariableGateway gateway;
    const GsxStateService gsx(&gateway);

    QVERIFY(gsx.IsAircraftOnGround());

    gateway.avars[kSimOnGround] = 1.0;

    QVERIFY(gsx.IsAircraftOnGround());

    gateway.avars[kSimOnGround] = 0.0;

    QVERIFY(!gsx.IsAircraftOnGround());
}

void GsxInterfaceTest::groundSpeedReadsZeroUntilTheSimVarArrives()
{
    FakeVariableGateway gateway;
    const GsxStateService gsx(&gateway);

    QCOMPARE(gsx.GetGroundSpeedKnots(), 0.0);

    gateway.avars[kGroundVelocity] = 12.5;

    QCOMPARE(gsx.GetGroundSpeedKnots(), 12.5);
}

void GsxInterfaceTest::boardedPassengersAccumulatesAcrossResets()
{
    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    gateway.lvars[kBoardingState] = static_cast<double>(GsxStateStatus::Active);
    gateway.lvars[kNumPassengersBoardingTotal] = 0.0;

    QCOMPARE(gsx.GetBoardedPassengers(), 0);

    gateway.lvars[kNumPassengersBoardingTotal] = 50.0;

    QCOMPARE(gsx.GetBoardedPassengers(), 50);

    gateway.lvars[kNumPassengersBoardingTotal] = 10.0;

    QCOMPARE(gsx.GetBoardedPassengers(), 60);

    gateway.lvars[kNumPassengersBoardingTotal] = 30.0;

    QCOMPARE(gsx.GetBoardedPassengers(), 80);

    gateway.lvars[kNumPassengersBoardingTotal] = 0;

    QCOMPARE(gsx.GetBoardedPassengers(), 80);
}

void GsxInterfaceTest::deboardedPassengersAccumulatesAcrossResets()
{
    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    gateway.lvars[kDeboardingState] = static_cast<double>(GsxStateStatus::Active);
    gateway.lvars[kNumPassengersDeboardingTotal] = 0.0;

    QCOMPARE(gsx.GetDeboardedPassengers(), 0);

    gateway.lvars[kNumPassengersDeboardingTotal] = 80.0;

    QCOMPARE(gsx.GetDeboardedPassengers(), 80);

    gateway.lvars[kNumPassengersDeboardingTotal] = 5.0;

    QCOMPARE(gsx.GetDeboardedPassengers(), 85);

    gateway.lvars[kNumPassengersDeboardingTotal] = 0;

    QCOMPARE(gsx.GetDeboardedPassengers(), 85);
}

void GsxInterfaceTest::boardedPassengersIgnoresStaleTotalBeforeBoardingStarts()
{
    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    gateway.lvars[kBoardingState] = static_cast<double>(GsxStateStatus::Callable);
    gateway.lvars[kNumPassengersBoardingTotal] = 192.0;

    QCOMPARE(gsx.GetBoardedPassengers(), 0);

    gateway.lvars[kBoardingState] = static_cast<double>(GsxStateStatus::Requested);

    QCOMPARE(gsx.GetBoardedPassengers(), 0);

    gateway.lvars[kBoardingState] = static_cast<double>(GsxStateStatus::Active);
    gateway.lvars[kNumPassengersBoardingTotal] = 0.0;

    QCOMPARE(gsx.GetBoardedPassengers(), 0);

    gateway.lvars[kNumPassengersBoardingTotal] = 12.0;

    QCOMPARE(gsx.GetBoardedPassengers(), 12);
}

void GsxInterfaceTest::deboardedPassengersIgnoresStaleTotalBeforeDeboardingStarts()
{
    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    gateway.lvars[kDeboardingState] = static_cast<double>(GsxStateStatus::Requested);
    gateway.lvars[kNumPassengersDeboardingTotal] = 192.0;

    QCOMPARE(gsx.GetDeboardedPassengers(), 0);

    gateway.lvars[kDeboardingState] = static_cast<double>(GsxStateStatus::Active);
    gateway.lvars[kNumPassengersDeboardingTotal] = 0.0;

    QCOMPARE(gsx.GetDeboardedPassengers(), 0);

    gateway.lvars[kNumPassengersDeboardingTotal] = 7.0;

    QCOMPARE(gsx.GetDeboardedPassengers(), 7);
}

void GsxInterfaceTest::boardedPassengersIgnoresTheStaleTotalOnTheFirstActiveTick()
{
    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    gateway.lvars[kBoardingState] = static_cast<double>(GsxStateStatus::Active);
    gateway.lvars[kNumPassengersBoardingTotal] = 92.0;

    QCOMPARE(gsx.GetBoardedPassengers(), 0);
    QCOMPARE(gsx.GetBoardedPassengers(), 0);

    gateway.lvars[kNumPassengersBoardingTotal] = 0.0;

    QCOMPARE(gsx.GetBoardedPassengers(), 0);

    gateway.lvars[kNumPassengersBoardingTotal] = 10.0;

    QCOMPARE(gsx.GetBoardedPassengers(), 10);
}

void GsxInterfaceTest::deboardedPassengersIgnoresTheStaleTotalOnTheFirstActiveTick()
{
    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    gateway.lvars[kDeboardingState] = static_cast<double>(GsxStateStatus::Active);
    gateway.lvars[kNumPassengersDeboardingTotal] = 92.0;

    QCOMPARE(gsx.GetDeboardedPassengers(), 0);

    gateway.lvars[kNumPassengersDeboardingTotal] = 0.0;

    QCOMPARE(gsx.GetDeboardedPassengers(), 0);

    gateway.lvars[kNumPassengersDeboardingTotal] = 7.0;

    QCOMPARE(gsx.GetDeboardedPassengers(), 7);
}

void GsxInterfaceTest::aBoardingFoundUnderwayTrustsItsFirstReading()
{
    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    gateway.lvars[kBoardingState] = static_cast<double>(GsxStateStatus::Active);
    gateway.lvars[kNumPassengersBoardingTotal] = 109.0;
    gateway.lvars[kBoardingCargoPercent] = 50.0;
    gsx.Observe();

    QCOMPARE(gsx.GetBoardedPassengers(), 109);
    QCOMPARE(gsx.GetBoardedPassengers(), 109);
    QCOMPARE(gsx.GetBoardingCargoPercent(), 50.0);
    QCOMPARE(gsx.GetBoardingCargoPercent(), 50.0);

    gateway.lvars[kBoardingCargoPercent] = 75.0;
    gsx.Observe();

    QCOMPARE(gsx.GetBoardedPassengers(), 109);
    QCOMPARE(gsx.GetBoardingCargoPercent(), 75.0);
}

void GsxInterfaceTest::aBoardingFoundUnderwayFollowsTheCountFromItsFirstReading()
{
    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    gateway.lvars[kBoardingState] = static_cast<double>(GsxStateStatus::Active);
    gateway.lvars[kNumPassengersBoardingTotal] = 40.0;
    gsx.Observe();

    QCOMPARE(gsx.GetBoardedPassengers(), 40);

    gateway.lvars[kNumPassengersBoardingTotal] = 41.0;
    gsx.Observe();

    QCOMPARE(gsx.GetBoardedPassengers(), 41);

    gateway.lvars[kNumPassengersBoardingTotal] = 60.0;
    gsx.Observe();

    QCOMPARE(gsx.GetBoardedPassengers(), 60);
}

void GsxInterfaceTest::aStateThatWasNeverObservedAsActiveKeepsTheLeftoverRule()
{
    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    gateway.lvars[kBoardingState] = static_cast<double>(GsxStateStatus::Callable);
    gateway.lvars[kNumPassengersBoardingTotal] = 130.0;
    gateway.lvars[kBoardingCargoPercent] = 50.0;
    gsx.Observe();

    gateway.lvars[kBoardingState] = static_cast<double>(GsxStateStatus::Active);
    gsx.Observe();

    QCOMPARE(gsx.GetBoardedPassengers(), 0);
    QCOMPARE(gsx.GetBoardedPassengers(), 0);
    QCOMPARE(gsx.GetBoardingCargoPercent(), 0.0);
    QCOMPARE(gsx.GetBoardingCargoPercent(), 0.0);

    gateway.lvars[kNumPassengersBoardingTotal] = 131.0;
    gateway.lvars[kBoardingCargoPercent] = 60.0;
    gsx.Observe();

    QCOMPARE(gsx.GetBoardedPassengers(), 131);
    QCOMPARE(gsx.GetBoardingCargoPercent(), 60.0);
}

void GsxInterfaceTest::aStateLVarThatHasNotArrivedIsNotAnObservationOfTheService()
{
    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    gsx.Observe();
    gsx.Observe();

    gateway.lvars[kBoardingState] = static_cast<double>(GsxStateStatus::Callable);
    gsx.Observe();

    gateway.lvars[kBoardingState] = static_cast<double>(GsxStateStatus::Active);
    gateway.lvars[kNumPassengersBoardingTotal] = 130.0;
    gsx.Observe();

    QCOMPARE(gsx.GetBoardedPassengers(), 0);

    gateway.lvars[kNumPassengersBoardingTotal] = 131.0;
    gsx.Observe();

    QCOMPARE(gsx.GetBoardedPassengers(), 131);

    FakeVariableGateway lateGateway;
    GsxStateService lateGsx(&lateGateway);

    lateGsx.Observe();

    lateGateway.lvars[kBoardingState] = static_cast<double>(GsxStateStatus::Active);
    lateGateway.lvars[kNumPassengersBoardingTotal] = 109.0;
    lateGsx.Observe();

    QCOMPARE(lateGsx.GetBoardedPassengers(), 109);
}

void GsxInterfaceTest::aDeboardingFoundUnderwayTrustsItsFirstReading()
{
    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    gateway.lvars[kDeboardingState] = static_cast<double>(GsxStateStatus::Active);
    gateway.lvars[kNumPassengersDeboardingTotal] = 80.0;
    gateway.lvars[kDeboardingCargoPercent] = 40.0;
    gsx.Observe();

    QCOMPARE(gsx.GetDeboardedPassengers(), 80);
    QCOMPARE(gsx.GetDeboardedPassengers(), 80);
    QCOMPARE(gsx.GetDeboardingCargoPercent(), 40.0);
    QCOMPARE(gsx.GetDeboardingCargoPercent(), 40.0);

    gateway.lvars[kNumPassengersDeboardingTotal] = 83.0;
    gateway.lvars[kDeboardingCargoPercent] = 70.0;
    gsx.Observe();

    QCOMPARE(gsx.GetDeboardedPassengers(), 83);
    QCOMPARE(gsx.GetDeboardingCargoPercent(), 70.0);
}

void GsxInterfaceTest::aServiceFoundUnderwayStopsBeingFoundOnceItLeavesActive()
{
    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    gateway.lvars[kBoardingState] = static_cast<double>(GsxStateStatus::Active);
    gateway.lvars[kNumPassengersBoardingTotal] = 92.0;
    gateway.lvars[kBoardingCargoPercent] = 50.0;
    gsx.Observe();

    QCOMPARE(gsx.GetBoardedPassengers(), 92);
    QCOMPARE(gsx.GetBoardingCargoPercent(), 50.0);

    gateway.lvars[kBoardingState] = static_cast<double>(GsxStateStatus::Completed);
    gsx.Observe();

    gsx.OnTurnaroundTurned();

    gateway.lvars[kBoardingState] = static_cast<double>(GsxStateStatus::Callable);
    gsx.Observe();

    gateway.lvars[kBoardingState] = static_cast<double>(GsxStateStatus::Active);
    gsx.Observe();

    QCOMPARE(gsx.GetBoardedPassengers(), 0);
    QCOMPARE(gsx.GetBoardingCargoPercent(), 0.0);
}

void GsxInterfaceTest::theTurnForgetsTheCompletionsOfTheLastTurnaround()
{
    for (const auto& [stateLVar, service] : kServicesThatEndThroughCompleted)
    {
        FakeVariableGateway gateway;
        GsxStateService gsx(&gateway);

        ObserveFor(gsx, gateway, stateLVar, 1.0, 5.0, 30);
        ObserveFor(gsx, gateway, stateLVar, 1.0, 6.0, 10);
        ObserveFor(gsx, gateway, stateLVar, 1.0, 1.0, 5);

        QVERIFY2(gsx.WasStateCompleted(service), stateLVar);

        gsx.OnTurnaroundTurned();
        ObserveFor(gsx, gateway, stateLVar, 1.0, 1.0, 5);

        QVERIFY2(!gsx.WasStateCompleted(service), stateLVar);
    }
}

void GsxInterfaceTest::theTurnKeepsTheReadingSoAServiceEndingAcrossItStillCounts()
{
    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    ObserveFor(gsx, gateway, kBoardingState, 1.0, 5.0, 10);

    gsx.OnTurnaroundTurned();
    ObserveFor(gsx, gateway, kBoardingState, 1.0, 1.0, 1);

    QVERIFY(gsx.WasStateCompleted(GsxState::Boarding));
}

void GsxInterfaceTest::boardedPassengersStartOverAfterTheTurn()
{
    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    gateway.lvars[kBoardingState] = static_cast<double>(GsxStateStatus::Active);
    gateway.lvars[kNumPassengersBoardingTotal] = 0.0;

    QCOMPARE(gsx.GetBoardedPassengers(), 0);

    gateway.lvars[kNumPassengersBoardingTotal] = 92.0;

    QCOMPARE(gsx.GetBoardedPassengers(), 92);

    gateway.lvars[kBoardingState] = static_cast<double>(GsxStateStatus::Callable);
    gsx.OnTurnaroundTurned();
    gateway.lvars[kBoardingState] = static_cast<double>(GsxStateStatus::Active);

    QCOMPARE(gsx.GetBoardedPassengers(), 0);

    gateway.lvars[kNumPassengersBoardingTotal] = 0.0;

    QCOMPARE(gsx.GetBoardedPassengers(), 0);

    gateway.lvars[kNumPassengersBoardingTotal] = 10.0;

    QCOMPARE(gsx.GetBoardedPassengers(), 10);
}

void GsxInterfaceTest::boardingCargoStartsOverAfterTheTurn()
{
    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    gateway.lvars[kBoardingState] = static_cast<double>(GsxStateStatus::Active);
    gateway.lvars[kBoardingCargoPercent] = 0.0;

    QCOMPARE(gsx.GetBoardingCargoPercent(), 0.0);

    gateway.lvars[kBoardingCargoPercent] = 100.0;

    QCOMPARE(gsx.GetBoardingCargoPercent(), 100.0);

    gsx.OnTurnaroundTurned();

    QCOMPARE(gsx.GetBoardingCargoPercent(), 0.0);
}

void GsxInterfaceTest::takeOverFuelAndPayloadClearsAutomationLVars()
{
    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);
    
    gateway.lvars[kAutomationFuel] = 1.0;
    gateway.lvars[kAutomationPayload] = 1.0;

    gsx.TakeOverFuelAndPayload();

    QCOMPARE(gateway.Written(kAutomationFuel), 0.0);
    QCOMPARE(gateway.Written(kAutomationPayload), 0.0);
}

void GsxInterfaceTest::reassertsTakeoverAfterCouatlReset()
{
    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    gateway.lvars[kAutomationFuel] = 1.0;
    gateway.lvars[kAutomationPayload] = 1.0;

    gsx.ReassertTakeovers();

    QCOMPARE(gateway.Written(kAutomationFuel), 1.0);

    gsx.TakeOverFuelAndPayload();
    gateway.lvars[kAutomationFuel] = 1.0;
    gateway.lvars[kAutomationPayload] = 1.0;

    gsx.ReassertTakeovers();

    QCOMPARE(gateway.Written(kAutomationFuel), 0.0);
    QCOMPARE(gateway.Written(kAutomationPayload), 0.0);

    gsx.Reset();
    gateway.lvars[kAutomationFuel] = 1.0;
    gateway.lvars[kAutomationPayload] = 1.0;

    gsx.ReassertTakeovers();

    QCOMPARE(gateway.Written(kAutomationFuel), 1.0);
    QCOMPARE(gateway.Written(kAutomationPayload), 1.0);
}

void GsxInterfaceTest::refuelCounterComesFromFuelCounterLvar()
{
    FakeVariableGateway gateway;
    const GsxStateService gsx(&gateway);

    QCOMPARE(gsx.GetRefuelCounterGallons(), 0.0);

    gateway.lvars[kFuelCounter] = 5000.0;
    QCOMPARE(gsx.GetRefuelCounterGallons(), 5000.0);

    gateway.lvars[kFuelCounterMax] = 8000.0;
    QCOMPARE(gsx.GetRefuelCounterGallons(), 8000.0);

    gateway.lvars[kFuelCounter] = 9000.0;
    QCOMPARE(gsx.GetRefuelCounterGallons(), 9000.0);
}

void GsxInterfaceTest::gpuStatusPrefersTheRemoteApiOverALyingLVar()
{
    FakeVariableGateway gateway;
    GsxRemoteState remote;

    const GsxStateService gsx(&gateway, &remote);

    gateway.lvars[kGpuState] = static_cast<double>(GsxStateStatus::Callable);
    gateway.lvars[kGpuConnected] = 1.0;
    remote.services.push_back(GsxRemoteService{"GPU", 1, true});

    QCOMPARE(gsx.GetGpuStatus(), GroundPowerStatus::Disconnected);

    remote.services.front().stateRaw = 4;

    QCOMPARE(gsx.GetGpuStatus(), GroundPowerStatus::Disconnected);

    remote.services.front().stateRaw = 5;

    QCOMPARE(gsx.GetGpuStatus(), GroundPowerStatus::Connected);
}

void GsxInterfaceTest::gpuStatusFallsBackToTheLVarsWithoutTheRemoteApi()
{
    FakeVariableGateway gateway;
    GsxRemoteState remote;

    const GsxStateService gsx(&gateway, &remote);

    gateway.lvars[kGpuState] = static_cast<double>(GsxStateStatus::Active);

    QCOMPARE(gsx.GetGpuStatus(), GroundPowerStatus::Connected);
}

void GsxInterfaceTest::gpuStatusFollowsStateLVar()
{
    FakeVariableGateway gateway;

    const GsxStateService gsx(&gateway);

    QCOMPARE(gsx.GetGpuStatus(), GroundPowerStatus::Unknown);

    gateway.lvars[kGpuState] = static_cast<double>(GsxStateStatus::Active);
    gateway.lvars[kGpuConnected] = 0.0;

    QCOMPARE(gsx.GetGpuStatus(), GroundPowerStatus::Connected);

    gateway.lvars[kGpuState] = static_cast<double>(GsxStateStatus::Callable);

    QCOMPARE(gsx.GetGpuStatus(), GroundPowerStatus::Disconnected);
}

void GsxInterfaceTest::gpuStatusConnectedWhenConnectedFlagSetDespiteState()
{
    FakeVariableGateway gateway;

    GsxStateService gsx(&gateway);

    gateway.lvars[kGpuState] = static_cast<double>(GsxStateStatus::Requested);
    gateway.lvars[kGpuConnected] = 0.0;
    gsx.Observe();

    QCOMPARE(gsx.GetGpuStatus(), GroundPowerStatus::Disconnected);

    gateway.lvars[kGpuConnected] = 1.0;
    gsx.Observe();

    QCOMPARE(gsx.GetGpuStatus(), GroundPowerStatus::Connected);
}

void GsxInterfaceTest::gpuStatusIgnoresAConnectedFlagInheritedFromTheLastTurnaround()
{
    FakeVariableGateway gateway;

    GsxStateService gsx(&gateway);

    gateway.lvars[kGpuState] = static_cast<double>(GsxStateStatus::Callable);
    gateway.lvars[kGpuConnected] = 1.0;
    gsx.Observe();

    QCOMPARE(gsx.GetGpuStatus(), GroundPowerStatus::Disconnected);

    gateway.lvars[kGpuState] = static_cast<double>(GsxStateStatus::Requested);
    gsx.Observe();

    QCOMPARE(gsx.GetGpuStatus(), GroundPowerStatus::Disconnected);

    gateway.lvars[kGpuConnected] = 0.0;
    gsx.Observe();

    QCOMPARE(gsx.GetGpuStatus(), GroundPowerStatus::Disconnected);

    gateway.lvars[kGpuConnected] = 1.0;
    gsx.Observe();

    QCOMPARE(gsx.GetGpuStatus(), GroundPowerStatus::Connected);
}

void GsxInterfaceTest::gpuStatusDoesNotArmTheConnectedFlagBeforeItArrives()
{
    FakeVariableGateway gateway;

    GsxStateService gsx(&gateway);

    gsx.Observe();

    QCOMPARE(gsx.GetGpuStatus(), GroundPowerStatus::Unknown);

    gateway.lvars[kGpuState] = static_cast<double>(GsxStateStatus::Callable);
    gateway.lvars[kGpuConnected] = 1.0;
    gsx.Observe();

    QCOMPARE(gsx.GetGpuStatus(), GroundPowerStatus::Disconnected);
}

void GsxInterfaceTest::serviceInProgressFollowsRemoteStateRaw()
{
    FakeVariableGateway gateway;
    GsxRemoteState remote;
    remote.services.push_back(GsxRemoteService{"Catering", 5, false});
    remote.services.push_back(GsxRemoteService{"Water", 4, false});
    remote.services.push_back(GsxRemoteService{"Lavatory", 1, true});
    remote.services.push_back(GsxRemoteService{"Cleaning", 6, false});

    const GsxStateService gsx(&gateway, &remote);

    QVERIFY(gsx.IsServiceInProgress(GroundService::Catering));
    QVERIFY(gsx.IsServiceInProgress(GroundService::Water));
    QVERIFY(!gsx.IsServiceInProgress(GroundService::Lavatory));
    QVERIFY(!gsx.IsServiceInProgress(GroundService::Cleaning));
}

void GsxInterfaceTest::serviceInProgressFalseWhenAbsentOrNoRemote()
{
    FakeVariableGateway gateway;
    GsxRemoteState remote;
    remote.services.push_back(GsxRemoteService{"Catering", 5, false});

    const GsxStateService withRemote(&gateway, &remote);

    QVERIFY(!withRemote.IsServiceInProgress(GroundService::Lavatory));

    const GsxStateService noRemote(&gateway);

    QVERIFY(!noRemote.IsServiceInProgress(GroundService::Catering));
}

void GsxInterfaceTest::latchAdvancesWithoutAnyoneAskingForTheStatus()
{
    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    gateway.lvars[kPushbackVehicleState] = static_cast<double>(GsxStateStatus::Active);
    gsx.Observe();

    QVERIFY(!gsx.WasStateCompleted(GsxState::Pushback));

    gateway.lvars[kPushbackVehicleState] = static_cast<double>(GsxStateStatus::Callable);
    gsx.Observe();

    QVERIFY(gsx.WasStateCompleted(GsxState::Pushback));
}

void GsxInterfaceTest::aDeiceReturningToIdleIsRecordedAsCompleted()
{
    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    gateway.lvars[kDeiceState] = static_cast<double>(GsxStateStatus::Active);
    gsx.Observe();

    QVERIFY(!gsx.WasStateCompleted(GsxState::Deice));

    gateway.lvars[kDeiceState] = static_cast<double>(GsxStateStatus::Callable);
    gsx.Observe();

    QVERIFY(gsx.WasStateCompleted(GsxState::Deice));
}

void GsxInterfaceTest::readingTheStatusDoesNotAdvanceTheLatch()
{
    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    gateway.lvars[kBoardingState] = static_cast<double>(GsxStateStatus::Active);
    QCOMPARE(gsx.GetStateStatus(GsxState::Boarding), GsxStateStatus::Active);

    gateway.lvars[kBoardingState] = static_cast<double>(GsxStateStatus::Callable);
    QCOMPARE(gsx.GetStateStatus(GsxState::Boarding), GsxStateStatus::Callable);

    QVERIFY(!gsx.WasStateCompleted(GsxState::Boarding));
}

void GsxInterfaceTest::pushbackIsOfferedUnlessTheApronVerdictSaysOtherwise()
{
    FakeVariableGateway gateway;
    GsxRemoteState remote;
    remote.apronVerdict = {"Gate Medium", "no pushback", "no bus"};

    const GsxStateService withVerdict(&gateway, &remote);

    QVERIFY(!withVerdict.OffersPushback());

    remote.apronVerdict = {"Gate Medium", "no bus"};

    QVERIFY(withVerdict.OffersPushback());

    const GsxStateService noRemote(&gateway);

    QVERIFY(noRemote.OffersPushback());
}

void GsxInterfaceTest::remoteApiCountsAsConnectedOnlyWhileTheLinkIsUp()
{
    FakeVariableGateway gateway;
    gateway.lvars[kCouatlStarted] = 1.0;

    GsxRemoteState remote;
    const GsxStateService withRemote(&gateway, &remote);

    QVERIFY(!withRemote.IsRemoteApiConnected());

    remote.connected = true;

    QVERIFY(withRemote.IsRemoteApiConnected());

    remote.connected = false;

    QVERIFY(!withRemote.IsRemoteApiConnected());

    const GsxStateService noRemote(&gateway);

    QVERIFY(!noRemote.IsRemoteApiConnected());
}

void GsxInterfaceTest::aRestoredBoardingKeepsTheBusesTheLastProcessCounted()
{
    const MemoryBag memory = MemoryOfARemoteStandBoardingAt92();

    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kBoardingState] = AsLVar(GsxStateStatus::Active);
    gateway.lvars[kNumPassengersBoardingTotal] = 12.0;
    gsx.RestoreMemory(memory, false);
    gsx.Observe();

    QCOMPARE(gsx.GetBoardedPassengers(), 92);

    FakeVariableGateway forgetfulGateway;
    GsxStateService forgetful(&forgetfulGateway);

    forgetfulGateway.lvars[kCouatlStarted] = 1.0;
    forgetfulGateway.lvars[kBoardingState] = AsLVar(GsxStateStatus::Active);
    forgetfulGateway.lvars[kNumPassengersBoardingTotal] = 12.0;
    forgetful.Observe();

    QCOMPARE(forgetful.GetBoardedPassengers(), 12);
}

void GsxInterfaceTest::aRestoredBoardingKeepsItsCountWhileTheCounterHasNotArrived()
{
    const MemoryBag memory = MemoryOfARemoteStandBoardingAt92();

    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kBoardingState] = AsLVar(GsxStateStatus::Active);
    gsx.RestoreMemory(memory, false);
    gsx.Observe();

    QCOMPARE(gsx.GetBoardedPassengers(), 92);
    QCOMPARE(gsx.GetBoardedPassengers(), 92);

    gateway.lvars[kNumPassengersBoardingTotal] = 12.0;

    QCOMPARE(gsx.GetBoardedPassengers(), 92);

    gateway.lvars[kNumPassengersBoardingTotal] = 0.0;

    QCOMPARE(gsx.GetBoardedPassengers(), 92);

    gateway.lvars[kNumPassengersBoardingTotal] = 15.0;

    QCOMPARE(gsx.GetBoardedPassengers(), 107);
}

void GsxInterfaceTest::aClosedServiceKeepsItsCountersAcrossTheMemory()
{
    FakeVariableGateway previousGateway;
    GsxStateService previous(&previousGateway);

    previousGateway.lvars[kBoardingState] = AsLVar(GsxStateStatus::Active);
    previousGateway.lvars[kDeboardingState] = AsLVar(GsxStateStatus::Active);
    previousGateway.lvars[kBoardingCargoPercent] = 0.0;
    previousGateway.lvars[kDeboardingCargoPercent] = 0.0;
    previousGateway.lvars[kNumPassengersBoardingTotal] = 0.0;
    previousGateway.lvars[kNumPassengersDeboardingTotal] = 0.0;
    static_cast<void>(previous.GetBoardedPassengers());
    static_cast<void>(previous.GetDeboardedPassengers());
    static_cast<void>(previous.GetBoardingCargoPercent());
    static_cast<void>(previous.GetDeboardingCargoPercent());

    previousGateway.lvars[kNumPassengersBoardingTotal] = 130.0;
    previousGateway.lvars[kNumPassengersDeboardingTotal] = 80.0;
    previousGateway.lvars[kBoardingCargoPercent] = 60.0;
    previousGateway.lvars[kDeboardingCargoPercent] = 45.0;

    QCOMPARE(previous.GetBoardedPassengers(), 130);
    QCOMPARE(previous.GetDeboardedPassengers(), 80);
    QCOMPARE(previous.GetBoardingCargoPercent(), 60.0);
    QCOMPARE(previous.GetDeboardingCargoPercent(), 45.0);

    const MemoryBag memory = previous.TakeMemory();

    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    gateway.lvars[kCouatlStarted] = 1.0;
    gateway.lvars[kBoardingState] = AsLVar(GsxStateStatus::Callable);
    gateway.lvars[kDeboardingState] = AsLVar(GsxStateStatus::Callable);
    gsx.RestoreMemory(memory, false);
    gsx.Observe();

    QCOMPARE(gsx.GetBoardedPassengers(), 130);
    QCOMPARE(gsx.GetDeboardedPassengers(), 80);
    QCOMPARE(gsx.GetBoardingCargoPercent(), 60.0);
    QCOMPARE(gsx.GetDeboardingCargoPercent(), 45.0);

    gateway.lvars[kNumPassengersBoardingTotal] = 0.0;
    gateway.lvars[kNumPassengersDeboardingTotal] = 0.0;

    QCOMPARE(gsx.GetBoardedPassengers(), 130);
    QCOMPARE(gsx.GetDeboardedPassengers(), 80);

    for (const double percent : {60.0, 25.0, 0.0})
    {
        previousGateway.lvars[kBoardingCargoPercent] = percent;
        previousGateway.lvars[kDeboardingCargoPercent] = percent;
        gateway.lvars[kBoardingCargoPercent] = percent;
        gateway.lvars[kDeboardingCargoPercent] = percent;

        QCOMPARE(gsx.GetBoardingCargoPercent(), previous.GetBoardingCargoPercent());
        QCOMPARE(gsx.GetDeboardingCargoPercent(), previous.GetDeboardingCargoPercent());
    }
}

void GsxInterfaceTest::aCounterThatHasNotArrivedReportsNothingAndTakesNoBaseline()
{
    for (const auto& [stateLVar, counterLVar, read] : kCounterReadings)
    {
        FakeVariableGateway gateway;
        GsxStateService gsx(&gateway);

        gateway.lvars[stateLVar] = AsLVar(GsxStateStatus::Active);

        QVERIFY2(read(gsx) == 0.0, counterLVar);
        QVERIFY2(read(gsx) == 0.0, counterLVar);

        gateway.lvars[counterLVar] = 92.0;

        QVERIFY2(read(gsx) == 0.0, counterLVar);

        gateway.lvars[counterLVar] = 100.0;

        QVERIFY2(read(gsx) == 100.0, counterLVar);
    }
}

void GsxInterfaceTest::aRestoredStatusSurvivesObservationsBeforeTheStateLVarArrives()
{
    for (const auto& [stateLVar, service] : kServicesThatEndThroughCompleted)
    {
        const MemoryBag memory = MemoryOfAServiceLeftAt(stateLVar, GsxStateStatus::Active);

        FakeVariableGateway gateway;
        GsxStateService gsx(&gateway);

        gsx.RestoreMemory(memory, false);
        gsx.Observe();
        gsx.Observe();

        QVERIFY2(!gsx.WasStateCompleted(service), stateLVar);

        ObserveFor(gsx, gateway, stateLVar, 1.0, AsLVar(GsxStateStatus::Callable), 3);

        QVERIFY2(gsx.WasStateCompleted(service), stateLVar);
    }
}

void GsxInterfaceTest::aCouatlFlagThatHasNotArrivedIsNotACouatlDeath()
{
    const MemoryBag memory = MemoryOfAServiceLeftAt(kRefuelingState, GsxStateStatus::Active);

    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    gsx.RestoreMemory(memory, false);
    gateway.lvars[kRefuelingState] = AsLVar(GsxStateStatus::Callable);
    gsx.Observe();

    QVERIFY(!gsx.WasGsxDownSinceLastObserve());
    QVERIFY(gsx.WasStateCompleted(GsxState::Refueling));
}

void GsxInterfaceTest::aServiceFoundAvailableAfterTheMemoryIsRecordedAsCompleted()
{
    for (const auto& [stateLVar, service] : kServicesThatEndThroughCompleted)
    {
        const MemoryBag memory = MemoryOfAServiceLeftAt(stateLVar, GsxStateStatus::Active);

        FakeVariableGateway gateway;
        GsxStateService gsx(&gateway);

        gsx.RestoreMemory(memory, false);
        ObserveFor(gsx, gateway, stateLVar, 1.0, AsLVar(GsxStateStatus::Callable), 3);

        QVERIFY2(gsx.WasStateCompleted(service), stateLVar);
    }
}

void GsxInterfaceTest::aServiceRestoredAfterACouatlRestartIsInterruptedNotCompleted()
{
    for (const auto& [stateLVar, service] : kServicesThatEndThroughCompleted)
    {
        for (const auto status : {GsxStateStatus::Requested, GsxStateStatus::Active, GsxStateStatus::Completing})
        {
            const MemoryBag memory = MemoryOfAServiceLeftAt(stateLVar, status);

            FakeVariableGateway gateway;
            GsxStateService gsx(&gateway);

            gsx.RestoreMemory(memory, true);
            ObserveFor(gsx, gateway, stateLVar, 1.0, AsLVar(GsxStateStatus::Callable), 3);

            QVERIFY2(!gsx.WasStateCompleted(service), stateLVar);
        }
    }
}

void GsxInterfaceTest::aPushbackAndADeiceRestoredUnderwayAreNeverRecordedCompletedByWhatTheyFindIdle()
{
    for (const auto& [stateLVar, service] : {std::pair{kPushbackVehicleState, GsxState::Pushback},
                                             std::pair{kDeiceState, GsxState::Deice}})
    {
        for (const bool restarted : {false, true})
        {
            for (const auto saved : {GsxStateStatus::Requested, GsxStateStatus::Active, GsxStateStatus::Completing})
            {
                for (const auto found : {GsxStateStatus::Callable, GsxStateStatus::Bypassed})
                {
                    const MemoryBag memory = MemoryOfAServiceLeftAt(stateLVar, saved);

                    FakeVariableGateway gateway;
                    GsxStateService gsx(&gateway);

                    gsx.RestoreMemory(memory, restarted);
                    ObserveFor(gsx, gateway, stateLVar, 1.0, AsLVar(found), 3);

                    QVERIFY2(!gsx.WasStateCompleted(service), stateLVar);
                }
            }
        }
    }
}

void GsxInterfaceTest::aPushbackSavedWhileTheTugApproachesAndReadMidPushIsNotRecordedCompleted()
{
    const MemoryBag memory = MemoryOfAServiceLeftAt(kPushbackVehicleState, GsxStateStatus::Active);

    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    gsx.RestoreMemory(memory, false);
    ObserveFor(gsx, gateway, kPushbackVehicleState, 1.0, AsLVar(GsxStateStatus::Bypassed), 3);

    QVERIFY(!gsx.WasStateCompleted(GsxState::Pushback));
}

void GsxInterfaceTest::aRestoredPushbackAndDeiceSeenActiveAgainFollowTheLiveRule()
{
    for (const auto& [stateLVar, service] : {std::pair{kPushbackVehicleState, GsxState::Pushback},
                                             std::pair{kDeiceState, GsxState::Deice}})
    {
        const MemoryBag memory = MemoryOfAServiceLeftAt(stateLVar, GsxStateStatus::Active);

        FakeVariableGateway gateway;
        GsxStateService gsx(&gateway);

        gsx.RestoreMemory(memory, true);
        ObserveFor(gsx, gateway, stateLVar, 1.0, AsLVar(GsxStateStatus::Active), 3);

        QVERIFY2(!gsx.WasStateCompleted(service), stateLVar);

        ObserveFor(gsx, gateway, stateLVar, 1.0, AsLVar(GsxStateStatus::Callable), 3);

        QVERIFY2(gsx.WasStateCompleted(service), stateLVar);
    }
}

void GsxInterfaceTest::aRestartIsNotWrittenIntoTheMemoryBeforeAReadingConfirmsIt()
{
    const MemoryBag memory = MemoryOfAServiceLeftAt(kBoardingState, GsxStateStatus::Active);

    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    gsx.RestoreMemory(memory, true);

    QVERIFY(gsx.TakeMemory() == memory);
}

void GsxInterfaceTest::aRestartDoesNotMarkAPushbackOrADeiceAsHavingLostItsCouatl()
{
    for (const auto& [stateLVar, entry] : {std::pair{kPushbackVehicleState, "gsx.service.pushback.couatlDiedDuringRun"},
                                           std::pair{kDeiceState, "gsx.service.deice.couatlDiedDuringRun"}})
    {
        const MemoryBag memory = MemoryOfAServiceLeftAt(stateLVar, GsxStateStatus::Active);

        FakeVariableGateway gateway;
        GsxStateService gsx(&gateway);

        gsx.RestoreMemory(memory, true);
        ObserveFor(gsx, gateway, stateLVar, 1.0, AsLVar(GsxStateStatus::Active), 3);

        QVERIFY2(!gsx.TakeMemory().Flag(entry, true), entry);
    }
}

void GsxInterfaceTest::restoringAnEmptyMemoryForgetsWhatTheServiceSawBefore()
{
    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    gateway.lvarSpans[kCouatlStarted] = LVarSpan{0.0, 1.0, true};
    ObserveFor(gsx, gateway, kBoardingState, 1.0, AsLVar(GsxStateStatus::Callable), 1);

    QVERIFY(gsx.WasGsxDownSinceLastObserve());

    gsx.RestoreMemory(MemoryBag{}, false);

    QVERIFY(!gsx.WasGsxDownSinceLastObserve());

    gateway.lvars[kBoardingState] = AsLVar(GsxStateStatus::Active);
    gateway.lvars[kNumPassengersBoardingTotal] = 40.0;
    gsx.Observe();

    QCOMPARE(gsx.GetBoardedPassengers(), 40);
}

void GsxInterfaceTest::aRestoredCargoPercentIsClampedLikeTheCounts()
{
    const std::vector<MemoryBag::Entry> kept = MemoryOfEverythingTheServiceKeeps().Entries();

    for (const auto& [saved, expected] : {std::pair{"250", 100.0}, std::pair{"-5", 0.0}})
    {
        std::vector<MemoryBag::Entry> entries = kept;
        for (MemoryBag::Entry& entry : entries)
        {
            if (entry.first == "gsx.boardingCargo.last")
            {
                entry.second = saved;
            }
        }

        FakeVariableGateway gateway;
        GsxStateService gsx(&gateway);

        gsx.RestoreMemory(MemoryBag(entries), false);

        QCOMPARE(gsx.GetBoardingCargoPercent(), expected);
    }
}

void GsxInterfaceTest::aServiceRestoredAsRequestedAndFoundAvailableCountsAsCompleted()
{
    for (const auto& [stateLVar, service] : kServicesThatEndThroughCompleted)
    {
        const MemoryBag memory = MemoryOfAServiceLeftAt(stateLVar, GsxStateStatus::Requested);

        FakeVariableGateway gateway;
        GsxStateService gsx(&gateway);

        gsx.RestoreMemory(memory, false);
        ObserveFor(gsx, gateway, stateLVar, 1.0, AsLVar(GsxStateStatus::Callable), 3);

        QVERIFY2(gsx.WasStateCompleted(service), stateLVar);
    }
}

void GsxInterfaceTest::theRestoredMarkClearsAtTheFirstReadingSoTheCompletionCountsOnce()
{
    for (const auto& [stateLVar, service] : kServicesThatEndThroughCompleted)
    {
        const MemoryBag memory = MemoryOfAServiceLeftAt(stateLVar, GsxStateStatus::Requested);

        FakeVariableGateway gateway;
        GsxStateService gsx(&gateway);

        gsx.RestoreMemory(memory, false);
        ObserveFor(gsx, gateway, stateLVar, 1.0, AsLVar(GsxStateStatus::Callable), 3);
        gsx.OnTurnaroundTurned();
        ObserveFor(gsx, gateway, stateLVar, 1.0, AsLVar(GsxStateStatus::Requested), 3);
        ObserveFor(gsx, gateway, stateLVar, 1.0, AsLVar(GsxStateStatus::Callable), 3);

        QVERIFY2(!gsx.WasStateCompleted(service), stateLVar);
    }
}

void GsxInterfaceTest::aLiveServiceGoingFromRequestedToAvailableIsNotCompleted()
{
    for (const auto& [stateLVar, service] : kServicesThatEndThroughCompleted)
    {
        FakeVariableGateway gateway;
        GsxStateService gsx(&gateway);

        ObserveFor(gsx, gateway, stateLVar, 1.0, AsLVar(GsxStateStatus::Requested), 3);
        ObserveFor(gsx, gateway, stateLVar, 1.0, AsLVar(GsxStateStatus::Callable), 3);

        QVERIFY2(!gsx.WasStateCompleted(service), stateLVar);
    }
}

void GsxInterfaceTest::aCompletedServiceStaysCompletedAcrossTheMemory()
{
    for (const auto& [stateLVar, service] : kServicesThatEndThroughCompleted)
    {
        const MemoryBag memory = MemoryOfAServiceLeftAt(stateLVar, GsxStateStatus::Completed);

        FakeVariableGateway gateway;
        GsxStateService gsx(&gateway);

        QVERIFY2(!gsx.WasStateCompleted(service), stateLVar);

        gsx.RestoreMemory(memory, false);

        QVERIFY2(gsx.WasStateCompleted(service), stateLVar);
    }
}

void GsxInterfaceTest::aCouatlDeathTheLastProcessSawSurvivesTheMemory()
{
    FakeVariableGateway previousGateway;
    GsxStateService previous(&previousGateway);

    ObserveFor(previous, previousGateway, kBoardingState, 1.0, 5.0, 10);
    ObserveFor(previous, previousGateway, kBoardingState, 0.0, 5.0, 12);
    ObserveFor(previous, previousGateway, kBoardingState, 1.0, 5.0, 3);

    const MemoryBag memory = previous.TakeMemory();

    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    gsx.RestoreMemory(memory, false);
    ObserveFor(gsx, gateway, kBoardingState, 1.0, AsLVar(GsxStateStatus::Callable), 3);

    QVERIFY(!gsx.WasStateCompleted(GsxState::Boarding));
}

void GsxInterfaceTest::aRestoredCargoReadingStillIgnoresTheStalePercent()
{
    FakeVariableGateway previousGateway;
    GsxStateService previous(&previousGateway);

    previousGateway.lvars[kBoardingState] = AsLVar(GsxStateStatus::Active);
    previousGateway.lvars[kBoardingCargoPercent] = 92.0;

    QCOMPARE(previous.GetBoardingCargoPercent(), 0.0);

    const MemoryBag memory = previous.TakeMemory();

    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    gateway.lvars[kBoardingState] = AsLVar(GsxStateStatus::Active);
    gateway.lvars[kBoardingCargoPercent] = 92.0;
    gsx.RestoreMemory(memory, false);

    QCOMPARE(gsx.GetBoardingCargoPercent(), 0.0);

    gateway.lvars[kBoardingCargoPercent] = 95.0;

    QCOMPARE(gsx.GetBoardingCargoPercent(), 95.0);
}

void GsxInterfaceTest::aServiceThatGoesThroughCompletingStraightToIdleIsRecordedAsCompleted()
{
    for (const auto& [stateLVar, service] : kServicesThatEndThroughCompleted)
    {
        FakeVariableGateway gateway;
        GsxStateService gsx(&gateway);

        ObserveFor(gsx, gateway, stateLVar, 1.0, 5.0, 30);
        ObserveFor(gsx, gateway, stateLVar, 1.0, 7.0, 2);
        ObserveFor(gsx, gateway, stateLVar, 1.0, 1.0, 5);

        QVERIFY2(gsx.WasStateCompleted(service), stateLVar);
    }
}

void GsxInterfaceTest::restoringBeforeOrAfterTheFirstReadingGivesTheSameVerdict()
{
    const MemoryBag memory = MemoryOfAServiceLeftAt(kBoardingState, GsxStateStatus::Requested);

    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    ObserveFor(gsx, gateway, kBoardingState, 1.0, AsLVar(GsxStateStatus::Callable), 3);

    QVERIFY(!gsx.WasStateCompleted(GsxState::Boarding));

    gsx.RestoreMemory(memory, false);
    ObserveFor(gsx, gateway, kBoardingState, 1.0, AsLVar(GsxStateStatus::Callable), 3);

    QVERIFY(gsx.WasStateCompleted(GsxState::Boarding));
}

void GsxInterfaceTest::theMemoryRestoresToTheMemoryItWasTakenFrom()
{
    const MemoryBag memory = MemoryOfEverythingTheServiceKeeps();

    FakeVariableGateway fresh;
    const GsxStateService untouched(&fresh);

    QVERIFY(memory != untouched.TakeMemory());

    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    gsx.RestoreMemory(memory, false);

    QVERIFY(gsx.TakeMemory() == memory);
}

void GsxInterfaceTest::theMemoryIgnoresUnknownNames()
{
    const MemoryBag memory = MemoryOfEverythingTheServiceKeeps();

    std::vector<MemoryBag::Entry> entries = memory.Entries();
    entries.emplace_back("doorSync.FwdPax", "open");
    entries.emplace_back("gsx.somethingNobodyKnows", "1");
    entries.emplace_back("", "");

    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    gsx.RestoreMemory(MemoryBag(entries), false);

    QVERIFY(gsx.TakeMemory() == memory);
}

void GsxInterfaceTest::anEmptyMemoryRestoresTheStateOfAReset()
{
    FakeVariableGateway untouchedGateway;
    const GsxStateService untouched(&untouchedGateway);

    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    gsx.RestoreMemory(MemoryOfEverythingTheServiceKeeps(), false);

    QVERIFY(gsx.TakeMemory() != untouched.TakeMemory());

    gsx.RestoreMemory(MemoryBag{}, false);

    QVERIFY(gsx.TakeMemory() == untouched.TakeMemory());
}

void GsxInterfaceTest::theGpuClearSightingSurvivesTheMemory()
{
    FakeVariableGateway previousGateway;
    GsxStateService previous(&previousGateway);

    previousGateway.lvars[kGpuConnected] = 0.0;
    previous.Observe();

    const MemoryBag memory = previous.TakeMemory();

    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    gateway.lvars[kGpuState] = AsLVar(GsxStateStatus::Callable);
    gateway.lvars[kGpuConnected] = 1.0;

    QCOMPARE(gsx.GetGpuStatus(), GroundPowerStatus::Disconnected);

    gsx.RestoreMemory(memory, false);

    QCOMPARE(gsx.GetGpuStatus(), GroundPowerStatus::Connected);
}

void GsxInterfaceTest::theTakeoverOfFuelAndPayloadSurvivesTheMemory()
{
    FakeVariableGateway previousGateway;
    GsxStateService previous(&previousGateway);

    previous.TakeOverFuelAndPayload();

    const MemoryBag memory = previous.TakeMemory();

    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    gateway.lvars[kAutomationFuel] = 1.0;
    gateway.lvars[kAutomationPayload] = 1.0;
    gsx.ReassertTakeovers();

    QCOMPARE(gateway.Written(kAutomationFuel), 1.0);

    gsx.RestoreMemory(memory, false);
    gsx.ReassertTakeovers();

    QCOMPARE(gateway.Written(kAutomationFuel), 0.0);
    QCOMPARE(gateway.Written(kAutomationPayload), 0.0);
}

void GsxInterfaceTest::reassertingBeforeTheAutomationFlagsArriveStillTakesFuelAndPayloadBack()
{
    FakeVariableGateway previousGateway;
    GsxStateService previous(&previousGateway);

    previous.TakeOverFuelAndPayload();

    FakeVariableGateway gateway;
    GsxStateService gsx(&gateway);

    gsx.RestoreMemory(previous.TakeMemory(), false);
    gsx.ReassertTakeovers();

    QCOMPARE(gateway.WriteCount(kAutomationFuel), 1);
    QCOMPARE(gateway.WriteCount(kAutomationPayload), 1);
    QCOMPARE(gateway.Written(kAutomationFuel), 0.0);
    QCOMPARE(gateway.Written(kAutomationPayload), 0.0);

    gsx.ReassertTakeovers();

    QCOMPARE(gateway.WriteCount(kAutomationFuel), 1);
    QCOMPARE(gateway.WriteCount(kAutomationPayload), 1);

    gateway.lvars[kAutomationFuel] = 1.0;
    gateway.lvars[kAutomationPayload] = 1.0;
    gsx.ReassertTakeovers();

    QCOMPARE(gateway.WriteCount(kAutomationFuel), 2);
    QCOMPARE(gateway.Written(kAutomationFuel), 0.0);
    QCOMPARE(gateway.Written(kAutomationPayload), 0.0);
}

void GsxInterfaceTest::theResumeReadingsHaveArrivedOnlyWhenEveryOneHas()
{
    FakeVariableGateway gateway;
    const GsxStateService gsx(&gateway);

    for (const char* reading : kResumeReadings)
    {
        QVERIFY2(!gsx.HaveResumeReadingsArrived(), reading);

        gateway.lvars[reading] = 1.0;
    }

    QVERIFY(gsx.HaveResumeReadingsArrived());
}

void GsxInterfaceTest::theResumeReadingsHaveNotArrivedWhenAnyOneIsMissingAlone()
{
    for (const char* missing : kResumeReadings)
    {
        FakeVariableGateway gateway;
        const GsxStateService gsx(&gateway);

        for (const char* reading : kResumeReadings)
        {
            if (reading != missing)
            {
                gateway.lvars[reading] = 1.0;
            }
        }

        QVERIFY2(!gsx.HaveResumeReadingsArrived(), missing);
    }
}

void GsxInterfaceTest::askingIfTheResumeReadingsArrivedRequestsEveryOne()
{
    FakeVariableGateway gateway;
    const GsxStateService gsx(&gateway);

    QVERIFY(!gsx.HaveResumeReadingsArrived());

    for (const char* reading : kResumeReadings)
    {
        QVERIFY2(gateway.requestedLVars.contains(reading), reading);
    }
}

QTEST_APPLESS_MAIN(GsxInterfaceTest)

#include "tst_gsx_interface.moc"
