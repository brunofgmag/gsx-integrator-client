#include <algorithm>
#include <QtTest/QTest>

#include "../TurnaroundStateFixture.h"
#include "../../../src/domain/turnaround/states/BoardingTrack.h"

namespace
{
    struct RefueledFixture : TurnaroundStateFixture
    {
        RefueledFixture()
        {
            ctx.data.refuelFinished = true;
        }
    };
}

class BoardingTrackTest final : public QObject
{
    Q_OBJECT

private slots:
    static void holdsUntilGsxActive();
    static void boardSelfLoadsPayloadOnceAndAnimatesBar();
    static void boardPassengersProgressively();
    static void holdsDoorsClosedOnceBoardingFinishes();
    static void doesNotHoldDoorsWhileCargoStillPending();
    static void barHoldsBelowOneHundredWhileCargoStillPending();
    static void boardCargoProgressively();
    static void snapsToPlannedWhenGsxCountersFallShort();
    static void rebaselinesInitialZfwWhenCapturedBeforeSimData();
    static void clampsRebaselineToPlannedZfw();
    static void asksGsxToCompleteBoardingStalledAtOneHundred();
    static void asksAgainWhenGsxSwallowsTheForcedCompletion();
    static void stopsAskingOnceTheServiceCloses();
    static void doesNotAskGsxToCompleteWhileTheLoaderIsStillWorking();
    static void namesTheLoaderOnceItsDoorIsLate();
    static void standsDownOnceTheLoaderGetsItsDoor();
    static void stopsNamingTheLoaderOnceGsxDropsTheBoarding();
    static void keepsTheClockRunningWhenAnotherLoaderTakesOverTheWait();
    static void staysQuietThroughTheHandoversOfAHealthyBoarding();
    static void givesUpOnALoaderThatNeverGetsItsDoor();
    static void givesUpEvenWhenTheLoadersTakeTurnsWaiting();
    static void finishesTheBoardingOnceItHasGivenUpOnTheLoader();
    static void finishesTheBoardingOnceTheCargoFlagOutlivesTheClosedService();
    static void waitsForTheCargoFlagWhileGsxHasNotClosedTheService();
    static void doesNotAskGsxToCompleteWhilePassengersAreMissing();
    static void asksGsxToCompleteWhenTheLoadersAreHeldBehindTheStairs();
    static void keepsAskingWhileTheLoadersStayHeldBehindTheStairs();
    static void doesNotAskGsxToCompleteWhenCargoIsMerelySlow();
    static void doesNotAskGsxToCompleteWhilePassengersStillUseTheKeptStairs();
    static void doesNotAskGsxToCompleteOnceTheHeldCargoStartsMoving();
    static void asksGsxToCompleteOnceTheHeldLoaderHasWaitedTooLongForADoor();
    static void asksGsxToCompleteWhenTheFreighterLoaderIsHeldBehindTheStairs();
    static void finishesTheFreighterBoardingOnlyOnceGsxConfirmsTheForcedCompletion();
    static void restartsTheFreighterCountWhenTheCargoStartsLoadingAgain();
    static void doesNotAskGsxToCompleteAFreighterWhoseStairsWereNotKept();
    static void reportsTheBoardingGsxDroppedAfterStartingIt();
};

void BoardingTrackTest::holdsUntilGsxActive()
{
    RefueledFixture f;

    f.gsxService.boardingState = GsxStateStatus::Callable;

    QVERIFY(!BoardingTrack::Advance(f.ctx));
}

void BoardingTrackTest::boardSelfLoadsPayloadOnceAndAnimatesBar()
{
    RefueledFixture f;

    f.aircraft.cargo = false;
    f.aircraft.boardMethod = BoardBy::Self;
    f.aircraft.emptyZfwKg = 130000.0;
    f.ctx.data.initialZfwKg = 130000.0;
    f.ctx.data.plannedZfwKg = 180000.0;
    f.ctx.data.plannedPassengers = 200;
    f.gsxService.boardingState = GsxStateStatus::Active;
    f.gsxService.boardedPassengers = 0;
    f.gsxService.cargoPercent = 0.0;

    QVERIFY(!BoardingTrack::Advance(f.ctx));
    QCOMPARE(f.aircraft.currentZfwKg, 180000.0);
    QCOMPARE(f.ctx.data.boardingProgress, 0.0);

    f.gsxService.boardedPassengers = 100;
    f.gsxService.cargoPercent = 100.0;

    QVERIFY(!BoardingTrack::Advance(f.ctx));
    QCOMPARE(f.aircraft.currentZfwKg, 180000.0);
    QCOMPARE(f.ctx.data.boardingProgress, 75.0);
}

void BoardingTrackTest::boardPassengersProgressively()
{
    RefueledFixture f;

    f.aircraft.cargo = false;
    f.aircraft.boardMethod = BoardBy::Client;
    f.aircraft.emptyZfwKg = 100000.0;
    f.ctx.data.initialZfwKg = 100000.0;
    f.ctx.data.plannedZfwKg = 200000.0;
    f.ctx.data.plannedPassengers = 100;
    f.gsxService.boardingState = GsxStateStatus::Active;
    f.gsxService.cargoPercent = 0;
    f.gsxService.boardedPassengers = 0;

    for (int tick = 0; tick < 20; ++tick)
    {
        f.gsxService.boardedPassengers += 5;
        f.gsxService.cargoPercent += 5.0;
        QVERIFY(!BoardingTrack::Advance(f.ctx));
    }

    f.gsxService.boardingState = GsxStateStatus::Completed;

    QVERIFY(BoardingTrack::Advance(f.ctx));
    QCOMPARE(f.aircraft.currentZfwKg, 200000.0);
    QCOMPARE(f.gsxService.boardedPassengers, 100);
    QCOMPARE(f.gsxService.cargoPercent, 100.0);
    QCOMPARE(f.ctx.data.boardedPassengers, 100);
}

void BoardingTrackTest::holdsDoorsClosedOnceBoardingFinishes()
{
    RefueledFixture f;

    f.aircraft.boardMethod = BoardBy::Client;
    f.aircraft.emptyZfwKg = 100000.0;
    f.ctx.data.initialZfwKg = 100000.0;
    f.ctx.data.plannedZfwKg = 200000.0;
    f.ctx.data.plannedPassengers = 100;
    f.gsxService.boardingState = GsxStateStatus::Active;

    QVERIFY(!BoardingTrack::Advance(f.ctx));
    QVERIFY(!f.aircraft.doorsHeldClosed);

    f.gsxService.boardingState = GsxStateStatus::Completed;

    QVERIFY(BoardingTrack::Advance(f.ctx));
    QVERIFY(f.aircraft.doorsHeldClosed);
}

void BoardingTrackTest::doesNotHoldDoorsWhileCargoStillPending()
{
    RefueledFixture f;

    f.aircraft.boardMethod = BoardBy::Client;
    f.aircraft.emptyZfwKg = 100000.0;
    f.ctx.data.initialZfwKg = 100000.0;
    f.ctx.data.plannedZfwKg = 200000.0;
    f.ctx.data.plannedPassengers = 100;
    f.gsxService.boardingState = GsxStateStatus::Completed;
    f.gsxService.loadingCargo = true;

    QVERIFY(!BoardingTrack::Advance(f.ctx));
    QVERIFY(!f.aircraft.doorsHeldClosed);
    QCOMPARE(f.aircraft.holdDoorsClosedCalls, 0);
}

void BoardingTrackTest::barHoldsBelowOneHundredWhileCargoStillPending()
{
    RefueledFixture f;

    f.aircraft.boardMethod = BoardBy::Self;
    f.aircraft.emptyZfwKg = 100000.0;
    f.ctx.data.initialZfwKg = 100000.0;
    f.ctx.data.plannedZfwKg = 200000.0;
    f.ctx.data.plannedPassengers = 100;
    f.gsxService.boardingState = GsxStateStatus::Active;
    f.gsxService.boardedPassengers = 100;
    f.gsxService.cargoPercent = 100.0;
    f.gsxService.loadingCargo = true;

    QVERIFY(!BoardingTrack::Advance(f.ctx));
    QVERIFY(f.ctx.data.boardingProgress <= 99.0);

    f.gsxService.loadingCargo = false;

    QVERIFY(!BoardingTrack::Advance(f.ctx));
    QCOMPARE(f.ctx.data.boardingProgress, 100.0);
}

void BoardingTrackTest::boardCargoProgressively()
{
    RefueledFixture f;

    f.aircraft.cargo = true;
    f.aircraft.boardMethod = BoardBy::Client;
    f.aircraft.currentZfwKg = 130000.0;
    f.aircraft.emptyZfwKg = 130000.0;
    f.ctx.data.initialZfwKg = 130000.0;
    f.ctx.data.plannedZfwKg = 180000.0;
    f.ctx.data.plannedPassengers = 3;
    f.gsxService.boardingState = GsxStateStatus::Active;

    for (int tick = 0; tick < 20; ++tick)
    {
        f.gsxService.cargoPercent += 5.0;
        QVERIFY(!BoardingTrack::Advance(f.ctx));
    }
    f.gsxService.boardingState = GsxStateStatus::Completed;
    f.gsxService.boardedPassengers = 3;

    QVERIFY(BoardingTrack::Advance(f.ctx));
    QCOMPARE(f.aircraft.currentZfwKg, 180000.0);
}

void BoardingTrackTest::snapsToPlannedWhenGsxCountersFallShort()
{
    RefueledFixture f;

    f.aircraft.cargo = false;
    f.aircraft.boardMethod = BoardBy::Client;
    f.ctx.data.initialZfwKg = 100000.0;
    f.ctx.data.plannedZfwKg = 200000.0;
    f.ctx.data.plannedPassengers = 200;
    f.gsxService.boardedPassengers = 195;
    f.gsxService.cargoPercent = 0.0;
    f.gsxService.boardingState = GsxStateStatus::Completed;

    QVERIFY(BoardingTrack::Advance(f.ctx));
    QCOMPARE(f.aircraft.currentZfwKg, 200000.0);
    QCOMPARE(f.ctx.data.boardingProgress, 100.0);
    QCOMPARE(f.ctx.data.boardedPassengers, 200);
}

void BoardingTrackTest::rebaselinesInitialZfwWhenCapturedBeforeSimData()
{
    RefueledFixture f;

    f.aircraft.boardMethod = BoardBy::Client;
    f.aircraft.emptyZfwKg = 45000.0;
    f.ctx.data.initialZfwKg = 0.0;
    f.ctx.data.plannedZfwKg = 65000.0;
    f.ctx.data.plannedPassengers = 100;
    f.gsxService.boardingState = GsxStateStatus::Active;
    f.gsxService.boardedPassengers = 50;
    f.gsxService.cargoPercent = 50.0;

    QVERIFY(!BoardingTrack::Advance(f.ctx));

    QCOMPARE(f.ctx.data.initialZfwKg, 45000.0);
    QCOMPARE(f.aircraft.currentZfwKg, 55000.0);
}

void BoardingTrackTest::clampsRebaselineToPlannedZfw()
{
    RefueledFixture f;

    f.aircraft.boardMethod = BoardBy::Client;
    f.aircraft.emptyZfwKg = 42000.0;
    f.ctx.data.initialZfwKg = 0.0;
    f.ctx.data.plannedZfwKg = 20000.0;
    f.ctx.data.plannedPassengers = 100;
    f.gsxService.boardingState = GsxStateStatus::Active;

    QVERIFY(!BoardingTrack::Advance(f.ctx));

    QCOMPARE(f.ctx.data.initialZfwKg, 20000.0);
}

void BoardingTrackTest::asksGsxToCompleteBoardingStalledAtOneHundred()
{
    RefueledFixture f;

    f.aircraft.cargo = true;
    f.ctx.data.plannedZfwKg = 180000.0;
    f.gsxService.boardingState = GsxStateStatus::Active;
    f.gsxService.cargoPercent = 100.0;

    for (int tick = 0; tick < 89; ++tick)
    {
        QVERIFY(!BoardingTrack::Advance(f.ctx));
    }

    QCOMPARE(f.menuGateway.completeBoardingCalls, 0);

    QVERIFY(!BoardingTrack::Advance(f.ctx));
    QCOMPARE(f.menuGateway.completeBoardingCalls, 1);
}

void BoardingTrackTest::asksAgainWhenGsxSwallowsTheForcedCompletion()
{
    RefueledFixture f;

    f.aircraft.cargo = true;
    f.ctx.data.plannedZfwKg = 180000.0;
    f.gsxService.boardingState = GsxStateStatus::Active;
    f.gsxService.cargoPercent = 100.0;

    for (int tick = 0; tick < 90; ++tick)
    {
        QVERIFY(!BoardingTrack::Advance(f.ctx));
    }

    QCOMPARE(f.menuGateway.completeBoardingCalls, 1);

    for (int tick = 0; tick < 29; ++tick)
    {
        QVERIFY(!BoardingTrack::Advance(f.ctx));
    }

    QCOMPARE(f.menuGateway.completeBoardingCalls, 1);

    QVERIFY(!BoardingTrack::Advance(f.ctx));
    QCOMPARE(f.menuGateway.completeBoardingCalls, 2);

    for (int tick = 0; tick < 30; ++tick)
    {
        QVERIFY(!BoardingTrack::Advance(f.ctx));
    }

    QCOMPARE(f.menuGateway.completeBoardingCalls, 3);
}

void BoardingTrackTest::stopsAskingOnceTheServiceCloses()
{
    RefueledFixture f;

    f.aircraft.cargo = true;
    f.ctx.data.plannedZfwKg = 180000.0;
    f.gsxService.boardingState = GsxStateStatus::Active;
    f.gsxService.cargoPercent = 100.0;

    for (int tick = 0; tick < 90; ++tick)
    {
        QVERIFY(!BoardingTrack::Advance(f.ctx));
    }

    QCOMPARE(f.menuGateway.completeBoardingCalls, 1);

    f.gsxService.boardingState = GsxStateStatus::Completed;

    QVERIFY(BoardingTrack::Advance(f.ctx));

    for (int tick = 0; tick < 120; ++tick)
    {
        (void)BoardingTrack::Advance(f.ctx);
    }

    QCOMPARE(f.menuGateway.completeBoardingCalls, 1);
}

void BoardingTrackTest::doesNotAskGsxToCompleteWhileTheLoaderIsStillWorking()
{
    RefueledFixture f;

    f.aircraft.cargo = true;
    f.ctx.data.plannedZfwKg = 180000.0;
    f.gsxService.boardingState = GsxStateStatus::Active;
    f.gsxService.cargoPercent = 100.0;
    f.gsxService.loadingCargo = true;

    for (int tick = 0; tick < 400; ++tick)
    {
        QVERIFY(!BoardingTrack::Advance(f.ctx));
    }

    QCOMPARE(f.menuGateway.completeBoardingCalls, 0);
}

namespace
{
    void ArrangeLoaderWaitingForItsDoor(TurnaroundStateFixture& f)
    {
        f.aircraft.cargo = true;
        f.ctx.data.plannedZfwKg = 180000.0;
        f.gsxService.boardingState = GsxStateStatus::Active;
        f.gsxService.cargoPercent = 40.0;
        f.gsxService.loaderWaitingForDoor = CargoLoader::MainDeck;
    }
}

void BoardingTrackTest::namesTheLoaderOnceItsDoorIsLate()
{
    RefueledFixture f;

    ArrangeLoaderWaitingForItsDoor(f);

    for (int tick = 0; tick < 44; ++tick)
    {
        QVERIFY(!BoardingTrack::Advance(f.ctx));
    }

    QCOMPARE(f.ctx.data.loaderHoldingBoarding, CargoLoader::None);

    QVERIFY(!BoardingTrack::Advance(f.ctx));
    QCOMPARE(f.ctx.data.loaderHoldingBoarding, CargoLoader::MainDeck);
}

void BoardingTrackTest::standsDownOnceTheLoaderGetsItsDoor()
{
    RefueledFixture f;

    ArrangeLoaderWaitingForItsDoor(f);

    for (int tick = 0; tick < 50; ++tick)
    {
        QVERIFY(!BoardingTrack::Advance(f.ctx));
    }

    QCOMPARE(f.ctx.data.loaderHoldingBoarding, CargoLoader::MainDeck);

    f.gsxService.loaderWaitingForDoor = CargoLoader::None;

    QVERIFY(!BoardingTrack::Advance(f.ctx));
    QCOMPARE(f.ctx.data.loaderHoldingBoarding, CargoLoader::None);
    QCOMPARE(f.ctx.data.loaderDoorWaitTicks, 0);
}

void BoardingTrackTest::stopsNamingTheLoaderOnceGsxDropsTheBoarding()
{
    RefueledFixture f;

    ArrangeLoaderWaitingForItsDoor(f);

    for (int tick = 0; tick < 50; ++tick)
    {
        QVERIFY(!BoardingTrack::Advance(f.ctx));
    }

    QCOMPARE(f.ctx.data.loaderHoldingBoarding, CargoLoader::MainDeck);

    f.gsxService.boardingState = GsxStateStatus::Callable;

    QVERIFY(!BoardingTrack::Advance(f.ctx));
    QVERIFY(BoardingTrack::IsInterrupted(f.ctx));
    QCOMPARE(f.ctx.data.loaderHoldingBoarding, CargoLoader::None);
}

void BoardingTrackTest::keepsTheClockRunningWhenAnotherLoaderTakesOverTheWait()
{
    RefueledFixture f;

    ArrangeLoaderWaitingForItsDoor(f);
    f.gsxService.loaderWaitingForDoor = CargoLoader::Front;

    for (int tick = 0; tick < 50; ++tick)
    {
        QVERIFY(!BoardingTrack::Advance(f.ctx));
    }

    QCOMPARE(f.ctx.data.loaderHoldingBoarding, CargoLoader::Front);

    f.gsxService.loaderWaitingForDoor = CargoLoader::Rear;

    QVERIFY(!BoardingTrack::Advance(f.ctx));
    QCOMPARE(f.ctx.data.loaderHoldingBoarding, CargoLoader::Rear);
    QCOMPARE(f.ctx.data.loaderDoorWaitTicks, 51);
}

void BoardingTrackTest::staysQuietThroughTheHandoversOfAHealthyBoarding()
{
    RefueledFixture f;

    ArrangeLoaderWaitingForItsDoor(f);

    for (int tick = 0; tick < 35; ++tick)
    {
        if (tick == 12)
        {
            f.gsxService.loaderWaitingForDoor = CargoLoader::Rear;
        }
        else if (tick == 23)
        {
            f.gsxService.loaderWaitingForDoor = CargoLoader::Front;
        }

        QVERIFY(!BoardingTrack::Advance(f.ctx));
    }

    QCOMPARE(f.ctx.data.loaderDoorWaitTicks, 35);
    QCOMPARE(f.ctx.data.loaderHoldingBoarding, CargoLoader::None);
}

void BoardingTrackTest::givesUpOnALoaderThatNeverGetsItsDoor()
{
    RefueledFixture f;

    ArrangeLoaderWaitingForItsDoor(f);

    for (int tick = 0; tick < 208; ++tick)
    {
        QVERIFY(!BoardingTrack::Advance(f.ctx));
    }

    QCOMPARE(f.menuGateway.completeBoardingCalls, 0);

    QVERIFY(!BoardingTrack::Advance(f.ctx));
    QCOMPARE(f.menuGateway.completeBoardingCalls, 1);
    QCOMPARE(f.ctx.data.loaderHoldingBoarding, CargoLoader::MainDeck);
}

void BoardingTrackTest::givesUpEvenWhenTheLoadersTakeTurnsWaiting()
{
    RefueledFixture f;

    ArrangeLoaderWaitingForItsDoor(f);

    for (int tick = 0; tick < 208; ++tick)
    {
        f.gsxService.loaderWaitingForDoor = tick % 2 == 0 ? CargoLoader::MainDeck : CargoLoader::Rear;

        QVERIFY(!BoardingTrack::Advance(f.ctx));
    }

    QCOMPARE(f.menuGateway.completeBoardingCalls, 0);

    QVERIFY(!BoardingTrack::Advance(f.ctx));
    QCOMPARE(f.menuGateway.completeBoardingCalls, 1);
}

void BoardingTrackTest::finishesTheBoardingOnceItHasGivenUpOnTheLoader()
{
    RefueledFixture f;

    ArrangeLoaderWaitingForItsDoor(f);
    f.gsxService.boardingState = GsxStateStatus::Completed;

    for (int tick = 0; tick < 119; ++tick)
    {
        QVERIFY(!BoardingTrack::Advance(f.ctx));
    }

    QVERIFY(BoardingTrack::Advance(f.ctx));
    QCOMPARE(f.aircraft.currentZfwKg, 180000.0);
}

namespace
{
    void ArrangeCargoFlagLeftUpByACompleteNow(TurnaroundStateFixture& f)
    {
        f.aircraft.cargo = true;
        f.aircraft.boardMethod = BoardBy::Client;
        f.ctx.data.initialZfwKg = 42000.0;
        f.ctx.data.plannedZfwKg = 58000.0;
        f.gsxService.boardingState = GsxStateStatus::Callable;
        f.gsxService.boardingCompleted = true;
        f.gsxService.loadingCargo = true;
        f.gsxService.cargoPercent = 0.0;
    }
}

void BoardingTrackTest::finishesTheBoardingOnceTheCargoFlagOutlivesTheClosedService()
{
    RefueledFixture f;

    ArrangeCargoFlagLeftUpByACompleteNow(f);

    for (int tick = 0; tick < 119; ++tick)
    {
        QVERIFY(!BoardingTrack::Advance(f.ctx));
    }

    QVERIFY(!f.aircraft.doorsHeldClosed);

    QVERIFY(BoardingTrack::Advance(f.ctx));
    QVERIFY(f.aircraft.doorsHeldClosed);
    QCOMPARE(f.aircraft.currentZfwKg, 58000.0);
    QCOMPARE(f.menuGateway.completeBoardingCalls, 0);
    QVERIFY(std::ranges::any_of(f.logger.messages, [](const std::string& message)
    {
        return message.find("still flags cargo loading") != std::string::npos;
    }));
}

void BoardingTrackTest::waitsForTheCargoFlagWhileGsxHasNotClosedTheService()
{
    RefueledFixture f;

    ArrangeCargoFlagLeftUpByACompleteNow(f);
    f.gsxService.boardingState = GsxStateStatus::Active;
    f.gsxService.boardingCompleted = false;

    for (int tick = 0; tick < 400; ++tick)
    {
        QVERIFY(!BoardingTrack::Advance(f.ctx));
    }

    f.gsxService.boardingState = GsxStateStatus::Completed;

    QVERIFY(!BoardingTrack::Advance(f.ctx));
    QVERIFY(!f.aircraft.doorsHeldClosed);
}

void BoardingTrackTest::doesNotAskGsxToCompleteWhilePassengersAreMissing()
{
    RefueledFixture f;

    f.aircraft.cargo = false;
    f.ctx.data.plannedZfwKg = 180000.0;
    f.ctx.data.plannedPassengers = 174;
    f.gsxService.boardingState = GsxStateStatus::Active;
    f.gsxService.cargoPercent = 100.0;
    f.gsxService.boardedPassengers = 173;

    for (int tick = 0; tick < 400; ++tick)
    {
        QVERIFY(!BoardingTrack::Advance(f.ctx));
    }

    QCOMPARE(f.menuGateway.completeBoardingCalls, 0);

    f.gsxService.boardedPassengers = 174;
    for (int tick = 0; tick < 90; ++tick)
    {
        QVERIFY(!BoardingTrack::Advance(f.ctx));
    }

    QCOMPARE(f.menuGateway.completeBoardingCalls, 1);
}

namespace
{
    void ArrangeCargoHeldBehindTheStairs(TurnaroundStateFixture& f)
    {
        f.aircraft.cargo = false;
        f.aircraft.boardMethod = BoardBy::Client;
        f.ctx.data.initialZfwKg = 40000.0;
        f.ctx.data.plannedZfwKg = 60000.0;
        f.ctx.data.plannedPassengers = 151;
        f.gsxService.boardingState = GsxStateStatus::Active;
        f.gsxService.boardedPassengers = 151;
        f.gsxService.cargoPercent = 0.0;
        f.menuGateway.stairsKeptInPlace = true;
    }
}

void BoardingTrackTest::asksGsxToCompleteWhenTheLoadersAreHeldBehindTheStairs()
{
    RefueledFixture f;

    ArrangeCargoHeldBehindTheStairs(f);

    for (int tick = 0; tick < 89; ++tick)
    {
        QVERIFY(!BoardingTrack::Advance(f.ctx));
    }

    QCOMPARE(f.menuGateway.completeBoardingCalls, 0);

    QVERIFY(!BoardingTrack::Advance(f.ctx));
    QCOMPARE(f.menuGateway.completeBoardingCalls, 1);
    QVERIFY(std::ranges::any_of(f.logger.messages, [](const std::string& message)
    {
        return message.find("held behind the stairs") != std::string::npos;
    }));
}

void BoardingTrackTest::keepsAskingWhileTheLoadersStayHeldBehindTheStairs()
{
    RefueledFixture f;

    ArrangeCargoHeldBehindTheStairs(f);

    for (int tick = 0; tick < 120; ++tick)
    {
        QVERIFY(!BoardingTrack::Advance(f.ctx));
    }

    QCOMPARE(f.menuGateway.completeBoardingCalls, 2);

    f.gsxService.boardingState = GsxStateStatus::Completed;

    QVERIFY(BoardingTrack::Advance(f.ctx));
    QCOMPARE(f.aircraft.currentZfwKg, 60000.0);
}

void BoardingTrackTest::doesNotAskGsxToCompleteWhenCargoIsMerelySlow()
{
    RefueledFixture f;

    ArrangeCargoHeldBehindTheStairs(f);
    f.menuGateway.stairsKeptInPlace = false;

    for (int tick = 0; tick < 400; ++tick)
    {
        QVERIFY(!BoardingTrack::Advance(f.ctx));
    }

    QCOMPARE(f.menuGateway.completeBoardingCalls, 0);
}

void BoardingTrackTest::doesNotAskGsxToCompleteWhilePassengersStillUseTheKeptStairs()
{
    RefueledFixture f;

    ArrangeCargoHeldBehindTheStairs(f);
    f.gsxService.boardedPassengers = 150;

    for (int tick = 0; tick < 400; ++tick)
    {
        QVERIFY(!BoardingTrack::Advance(f.ctx));
    }

    QCOMPARE(f.menuGateway.completeBoardingCalls, 0);
}

void BoardingTrackTest::doesNotAskGsxToCompleteOnceTheHeldCargoStartsMoving()
{
    RefueledFixture f;

    ArrangeCargoHeldBehindTheStairs(f);

    for (int tick = 0; tick < 80; ++tick)
    {
        QVERIFY(!BoardingTrack::Advance(f.ctx));
    }

    f.gsxService.cargoPercent = 5.0;

    for (int tick = 0; tick < 400; ++tick)
    {
        QVERIFY(!BoardingTrack::Advance(f.ctx));
    }

    QCOMPARE(f.menuGateway.completeBoardingCalls, 0);
    QCOMPARE(f.ctx.data.boardingStallTicks, 0);
}

void BoardingTrackTest::asksGsxToCompleteOnceTheHeldLoaderHasWaitedTooLongForADoor()
{
    RefueledFixture f;

    ArrangeCargoHeldBehindTheStairs(f);
    f.gsxService.loaderWaitingForDoor = CargoLoader::Rear;

    for (int tick = 0; tick < 208; ++tick)
    {
        QVERIFY(!BoardingTrack::Advance(f.ctx));
    }

    QCOMPARE(f.menuGateway.completeBoardingCalls, 0);

    QVERIFY(!BoardingTrack::Advance(f.ctx));
    QCOMPARE(f.menuGateway.completeBoardingCalls, 1);
}

namespace
{
    void ArrangeFreighterLoaderHeldBehindTheStairs(TurnaroundStateFixture& f)
    {
        f.aircraft.cargo = true;
        f.aircraft.boardMethod = BoardBy::Client;
        f.ctx.data.initialZfwKg = 42000.0;
        f.ctx.data.plannedZfwKg = 58000.0;
        f.ctx.data.plannedPassengers = 0;
        f.gsxService.boardingState = GsxStateStatus::Active;
        f.gsxService.boardedPassengers = 0;
        f.gsxService.cargoPercent = 67.0;
        f.menuGateway.stairsKeptInPlace = true;
    }
}

void BoardingTrackTest::asksGsxToCompleteWhenTheFreighterLoaderIsHeldBehindTheStairs()
{
    RefueledFixture f;

    ArrangeFreighterLoaderHeldBehindTheStairs(f);

    for (int tick = 0; tick < 89; ++tick)
    {
        QVERIFY(!BoardingTrack::Advance(f.ctx));
    }

    QCOMPARE(f.menuGateway.completeBoardingCalls, 0);

    QVERIFY(!BoardingTrack::Advance(f.ctx));
    QCOMPARE(f.menuGateway.completeBoardingCalls, 1);

    const auto heldLine = std::ranges::find_if(f.logger.messages, [](const std::string& message)
    {
        return message.find("held behind the stairs") != std::string::npos;
    });
    QVERIFY(heldLine != f.logger.messages.end());
    QCOMPARE(heldLine->find("passenger"), std::string::npos);
}

void BoardingTrackTest::finishesTheFreighterBoardingOnlyOnceGsxConfirmsTheForcedCompletion()
{
    RefueledFixture f;

    ArrangeFreighterLoaderHeldBehindTheStairs(f);

    for (int tick = 0; tick < 120; ++tick)
    {
        QVERIFY(!BoardingTrack::Advance(f.ctx));
    }

    QCOMPARE(f.menuGateway.completeBoardingCalls, 2);

    f.gsxService.boardingState = GsxStateStatus::Completed;

    QVERIFY(BoardingTrack::Advance(f.ctx));
    QCOMPARE(f.aircraft.currentZfwKg, 58000.0);
}

void BoardingTrackTest::restartsTheFreighterCountWhenTheCargoStartsLoadingAgain()
{
    RefueledFixture f;

    ArrangeFreighterLoaderHeldBehindTheStairs(f);

    for (int tick = 0; tick < 80; ++tick)
    {
        QVERIFY(!BoardingTrack::Advance(f.ctx));
    }

    f.gsxService.loadingCargo = true;

    QVERIFY(!BoardingTrack::Advance(f.ctx));
    QCOMPARE(f.ctx.data.boardingStallTicks, 0);

    f.gsxService.loadingCargo = false;

    for (int tick = 0; tick < 89; ++tick)
    {
        QVERIFY(!BoardingTrack::Advance(f.ctx));
    }

    QCOMPARE(f.menuGateway.completeBoardingCalls, 0);

    QVERIFY(!BoardingTrack::Advance(f.ctx));
    QCOMPARE(f.menuGateway.completeBoardingCalls, 1);
}

void BoardingTrackTest::doesNotAskGsxToCompleteAFreighterWhoseStairsWereNotKept()
{
    RefueledFixture f;

    ArrangeFreighterLoaderHeldBehindTheStairs(f);
    f.menuGateway.stairsKeptInPlace = false;

    for (int tick = 0; tick < 400; ++tick)
    {
        QVERIFY(!BoardingTrack::Advance(f.ctx));
    }

    QCOMPARE(f.menuGateway.completeBoardingCalls, 0);
}

void BoardingTrackTest::reportsTheBoardingGsxDroppedAfterStartingIt()
{
    RefueledFixture f;

    f.aircraft.cargo = false;
    f.aircraft.boardMethod = BoardBy::Self;
    f.ctx.data.initialZfwKg = 130000.0;
    f.ctx.data.plannedZfwKg = 180000.0;
    f.ctx.data.plannedPassengers = 55;
    f.gsxService.boardingState = GsxStateStatus::Active;
    f.gsxService.boardedPassengers = 55;
    f.gsxService.cargoPercent = 67.0;

    QVERIFY(!BoardingTrack::Advance(f.ctx));
    QVERIFY(!BoardingTrack::IsInterrupted(f.ctx));

    f.gsxService.boardingState = GsxStateStatus::Callable;
    f.gsxService.boardedPassengers = 0;
    f.gsxService.cargoPercent = 0.0;

    QVERIFY(!BoardingTrack::Advance(f.ctx));
    QVERIFY(BoardingTrack::IsInterrupted(f.ctx));

    f.gsxService.boardingState = GsxStateStatus::Active;

    QVERIFY(!BoardingTrack::Advance(f.ctx));
    QVERIFY(!BoardingTrack::IsInterrupted(f.ctx));
}

QTEST_APPLESS_MAIN(BoardingTrackTest)

#include "tst_boarding_track.moc"
