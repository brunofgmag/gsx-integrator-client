#include <algorithm>
#include <optional>
#include <string>
#include <QtTest/QTest>

#include "../TurnaroundStateFixture.h"
#include "../../../src/domain/turnaround/states/LoadingState.h"

namespace
{
    std::optional<TurnaroundTransition> Tick(LoadingState& state, TurnaroundStateFixture& f)
    {
        auto transition = state.Evaluate(f.ctx);
        if (!transition)
        {
            ++f.ctx.data.stateTickCount;
        }

        return transition;
    }

    void ArrangeClientRefuel(TurnaroundStateFixture& f, const double rateKgs, const double initialKg,
                             const double plannedKg)
    {
        f.settings.fuelRateKgs = rateKgs;
        f.aircraft.refuelMethod = RefuelBy::Client;
        f.aircraft.currentFuelKg = initialKg;
        f.ctx.data.plannedFuelKg = plannedKg;
        f.ctx.data.initialFuelKg = initialKg;
        f.ctx.data.loadedFuelKg = initialKg;
        f.gsxService.refuelingState = GsxStateStatus::Active;
        f.gsxService.hoseConnected = true;
        f.gsxService.boardingState = GsxStateStatus::Callable;
    }

    void ArrangePassengerBoarding(TurnaroundStateFixture& f)
    {
        f.aircraft.cargo = false;
        f.aircraft.boardMethod = BoardBy::Client;
        f.aircraft.emptyZfwKg = 40000.0;
        f.ctx.data.plannedZfwKg = 60000.0;
        f.ctx.data.plannedPassengers = 100;
    }

    bool Logged(const TurnaroundStateFixture& f, const std::string& fragment)
    {
        return std::ranges::any_of(f.logger.messages, [&fragment](const std::string& message)
        {
            return message.find(fragment) != std::string::npos;
        });
    }
}

class LoadingStateTest final : public QObject
{
    Q_OBJECT

private slots:
    static void doesNotRequestBoardingBelowThreeQuartersOfTheFuel();
    static void requestsBoardingOnTheFirstTickAtThreeQuartersOfTheFuel();
    static void earlyBoardingRequestsBoardingAtOnePercentOfTheFuel();
    static void withoutEarlyBoardingOnePercentOfTheFuelDoesNotRequestBoarding();
    static void requestsBoardingAtOnceWhenTheWholeRefuelFitsInAMinute();
    static void waitsForThreeQuartersWhenTheRefuelTakesLongerThanAMinute();
    static void gsxRefuelRequestsBoardingOnlyOnceTheFuelMoves();
    static void selfRefuelRequestsBoardingFromTheGsxCounter();
    static void selfRefuelWithTheCounterAtZeroRequestsBoardingOnlyOnTheSix();
    static void requestsBoardingOnceTheSixIsAlreadyBackToOne();
    static void notifiesTheAircraftBeforeRequestingBoarding();
    static void rearmsTheBoardingRequestEveryTenTicks();
    static void doesNotRequestTheBoardingItSawFinish();
    static void leavesOnlyOnceBothTracksFinishAndFinishesTheBoardingOnce();
    static void aDroppedRefuelDoesNotHoldTheBoarding();
    static void aDroppedBoardingDoesNotHoldTheRefuel();
    static void holdsTheBoardingCompleteNowUntilTheRefuelFinishes();
    static void theRampNeverPausesFromTheFirstToTheLastFuelTick();
    static void logsTheRefuelEndingBeforeGsxConfirmedTheBoarding();
    static void staysQuietWhenGsxConfirmedTheBoardingBeforeTheRefuelEnded();
};

void LoadingStateTest::doesNotRequestBoardingBelowThreeQuartersOfTheFuel()
{
    TurnaroundStateFixture f;
    LoadingState state;

    ArrangeClientRefuel(f, 10.0, 1000.0, 2000.0);

    for (int tick = 0; tick < 74; ++tick)
    {
        QVERIFY(!Tick(state, f).has_value());
    }

    QCOMPARE(f.ctx.data.fuelProgress, 74.0);
    QCOMPARE(f.menuGateway.boardingCalls, 0);
    QVERIFY(!f.ctx.data.boardingRequested);
}

void LoadingStateTest::requestsBoardingOnTheFirstTickAtThreeQuartersOfTheFuel()
{
    TurnaroundStateFixture f;
    LoadingState state;

    ArrangeClientRefuel(f, 10.0, 1000.0, 2000.0);

    for (int tick = 0; tick < 74; ++tick)
    {
        QVERIFY(!Tick(state, f).has_value());
    }

    QCOMPARE(f.menuGateway.boardingCalls, 0);

    QVERIFY(!Tick(state, f).has_value());
    QCOMPARE(f.ctx.data.fuelProgress, 75.0);
    QCOMPARE(f.menuGateway.boardingCalls, 1);
    QVERIFY(f.ctx.data.boardingRequested);
    QVERIFY(Logged(f, "Loading: requesting boarding because the refueling reached 75%"));
}

void LoadingStateTest::earlyBoardingRequestsBoardingAtOnePercentOfTheFuel()
{
    TurnaroundStateFixture f;
    LoadingState state;

    ArrangeClientRefuel(f, 10.0, 1000.0, 11000.0);
    f.settings.callBoardingEarly = true;

    for (int tick = 0; tick < 9; ++tick)
    {
        QVERIFY(!Tick(state, f).has_value());
    }

    QVERIFY(qFuzzyCompare(f.ctx.data.fuelProgress, 0.9));
    QCOMPARE(f.menuGateway.boardingCalls, 0);

    QVERIFY(!Tick(state, f).has_value());
    QCOMPARE(f.ctx.data.fuelProgress, 1.0);
    QCOMPARE(f.menuGateway.boardingCalls, 1);
    QVERIFY(f.ctx.data.boardingRequested);
    QVERIFY(Logged(f, "Loading: requesting boarding because the refueling reached 1%"));
}

void LoadingStateTest::withoutEarlyBoardingOnePercentOfTheFuelDoesNotRequestBoarding()
{
    TurnaroundStateFixture f;
    LoadingState state;

    ArrangeClientRefuel(f, 10.0, 1000.0, 11000.0);
    f.settings.callBoardingEarly = false;

    for (int tick = 0; tick < 10; ++tick)
    {
        QVERIFY(!Tick(state, f).has_value());
    }

    QCOMPARE(f.ctx.data.fuelProgress, 1.0);
    QCOMPARE(f.menuGateway.boardingCalls, 0);
    QVERIFY(!f.ctx.data.boardingRequested);
}

void LoadingStateTest::requestsBoardingAtOnceWhenTheWholeRefuelFitsInAMinute()
{
    TurnaroundStateFixture f;
    LoadingState state;

    ArrangeClientRefuel(f, 15.0, 1000.0, 1900.0);

    QVERIFY(!Tick(state, f).has_value());

    QCOMPARE(f.menuGateway.boardingCalls, 1);
    QVERIFY(Logged(f, "Loading: requesting boarding because only 900 kg of fuel is loading"));
}

void LoadingStateTest::waitsForThreeQuartersWhenTheRefuelTakesLongerThanAMinute()
{
    TurnaroundStateFixture f;
    LoadingState state;

    ArrangeClientRefuel(f, 15.0, 1000.0, 1901.0);

    QVERIFY(!Tick(state, f).has_value());

    QCOMPARE(f.menuGateway.boardingCalls, 0);
}

void LoadingStateTest::gsxRefuelRequestsBoardingOnlyOnceTheFuelMoves()
{
    TurnaroundStateFixture f;
    LoadingState state;

    ArrangeClientRefuel(f, 0.0, 1000.0, 2000.0);
    f.aircraft.refuelMethod = RefuelBy::Gsx;

    for (int tick = 0; tick < 5; ++tick)
    {
        QVERIFY(!Tick(state, f).has_value());
    }

    QCOMPARE(f.menuGateway.boardingCalls, 0);

    f.aircraft.currentFuelKg = 1100.0;

    QVERIFY(!Tick(state, f).has_value());
    QCOMPARE(f.menuGateway.boardingCalls, 1);
    QVERIFY(Logged(f, "only 1000 kg of fuel is loading"));
}

void LoadingStateTest::selfRefuelRequestsBoardingFromTheGsxCounter()
{
    TurnaroundStateFixture f;
    LoadingState state;

    ArrangeClientRefuel(f, 0.0, 1000.0, 5000.0);
    f.aircraft.refuelMethod = RefuelBy::Self;
    f.gsxService.refuelCounterGallons = 900.0;

    QVERIFY(!Tick(state, f).has_value());
    QCOMPARE(f.menuGateway.boardingCalls, 0);

    f.gsxService.refuelCounterGallons = 1000.0;

    QVERIFY(!Tick(state, f).has_value());
    QCOMPARE(f.ctx.data.fuelProgress, 76.0);
    QCOMPARE(f.menuGateway.boardingCalls, 1);
}

void LoadingStateTest::selfRefuelWithTheCounterAtZeroRequestsBoardingOnlyOnTheSix()
{
    TurnaroundStateFixture f;
    LoadingState state;

    ArrangeClientRefuel(f, 0.0, 1000.0, 2000.0);
    f.aircraft.refuelMethod = RefuelBy::Self;

    for (int tick = 0; tick < 30; ++tick)
    {
        QVERIFY(!Tick(state, f).has_value());
    }

    QCOMPARE(f.ctx.data.fuelProgress, 0.0);
    QCOMPARE(f.menuGateway.boardingCalls, 0);

    f.gsxService.refuelingState = GsxStateStatus::Completed;
    f.gsxService.refuelingCompleted = true;

    QVERIFY(!Tick(state, f).has_value());
    QCOMPARE(f.menuGateway.boardingCalls, 1);
    QVERIFY(!f.ctx.data.refuelFinished);
    QVERIFY(Logged(f, "Loading: requesting boarding because the refueling ended"));
}

void LoadingStateTest::requestsBoardingOnceTheSixIsAlreadyBackToOne()
{
    TurnaroundStateFixture f;
    LoadingState state;

    ArrangeClientRefuel(f, 0.0, 1000.0, 5000.0);
    f.aircraft.refuelMethod = RefuelBy::Self;
    f.gsxService.refuelingState = GsxStateStatus::Callable;
    f.gsxService.refuelingCompleted = true;
    f.gsxService.hoseConnected = false;

    QVERIFY(!Tick(state, f).has_value());

    QVERIFY(f.ctx.data.refuelFinished);
    QCOMPARE(f.menuGateway.boardingCalls, 1);
    QVERIFY(!f.ctx.data.serviceInterrupted);
}

void LoadingStateTest::notifiesTheAircraftBeforeRequestingBoarding()
{
    TurnaroundStateFixture f;
    LoadingState state;

    ArrangeClientRefuel(f, 15.0, 1000.0, 1900.0);
    f.gsxService.remoteApiConnected = false;

    for (int tick = 0; tick < 5; ++tick)
    {
        QVERIFY(!Tick(state, f).has_value());
    }

    QCOMPARE(f.aircraft.onLoadingStartedCalls, 0);
    QCOMPARE(f.menuGateway.boardingCalls, 0);

    f.gsxService.remoteApiConnected = true;

    QVERIFY(!Tick(state, f).has_value());
    QCOMPARE(f.aircraft.onLoadingStartedCalls, 1);
    QCOMPARE(f.menuGateway.boardingCalls, 1);
}

void LoadingStateTest::rearmsTheBoardingRequestEveryTenTicks()
{
    TurnaroundStateFixture f;
    LoadingState state;

    ArrangeClientRefuel(f, 15.0, 1000.0, 1900.0);

    for (int tick = 0; tick < 12; ++tick)
    {
        QVERIFY(!Tick(state, f).has_value());
    }

    QCOMPARE(f.menuGateway.boardingCalls, 2);
    QVERIFY(f.ctx.data.boardingRequested);

    f.gsxService.boardingState = GsxStateStatus::Requested;

    for (int tick = 0; tick < 30; ++tick)
    {
        QVERIFY(!Tick(state, f).has_value());
    }

    QCOMPARE(f.menuGateway.boardingCalls, 2);
    QVERIFY(f.ctx.data.boardingConfirmed);
}

void LoadingStateTest::doesNotRequestTheBoardingItSawFinish()
{
    TurnaroundStateFixture f;
    LoadingState state;

    ArrangeClientRefuel(f, 15.0, 1000.0, 1900.0);
    ArrangePassengerBoarding(f);
    f.gsxService.boardingCompleted = true;

    QVERIFY(!Tick(state, f).has_value());

    QVERIFY(f.ctx.data.boardingFinished);
    QCOMPARE(f.menuGateway.boardingCalls, 0);
    QVERIFY(!f.ctx.data.boardingRequested);
}

void LoadingStateTest::leavesOnlyOnceBothTracksFinishAndFinishesTheBoardingOnce()
{
    TurnaroundStateFixture f;
    LoadingState state;

    ArrangeClientRefuel(f, 10.0, 1000.0, 2000.0);
    ArrangePassengerBoarding(f);
    f.gsxService.boardingState = GsxStateStatus::Active;

    QVERIFY(!Tick(state, f).has_value());

    f.gsxService.boardingState = GsxStateStatus::Completed;

    QVERIFY(!Tick(state, f).has_value());
    QVERIFY(f.ctx.data.boardingFinished);
    QCOMPARE(f.aircraft.holdDoorsClosedCalls, 1);

    for (int tick = 0; tick < 98; ++tick)
    {
        QVERIFY(!Tick(state, f).has_value());
    }

    QCOMPARE(f.aircraft.currentFuelKg, 2000.0);
    QVERIFY(!f.ctx.data.refuelFinished);

    f.gsxService.refuelingState = GsxStateStatus::Completed;
    f.gsxService.hoseConnected = false;

    const auto transition = Tick(state, f);

    QVERIFY(transition.has_value());
    QCOMPARE(transition->next, TurnaroundPhase::WaitingReadyToPush);
    QCOMPARE(transition->delayTicks, 60);
    QCOMPARE(f.aircraft.holdDoorsClosedCalls, 1);
}

void LoadingStateTest::aDroppedRefuelDoesNotHoldTheBoarding()
{
    TurnaroundStateFixture f;
    LoadingState state;

    ArrangeClientRefuel(f, 10.0, 1000.0, 5000.0);
    ArrangePassengerBoarding(f);

    QVERIFY(!Tick(state, f).has_value());

    f.gsxService.refuelingState = GsxStateStatus::Callable;
    f.gsxService.hoseConnected = false;
    f.gsxService.boardingState = GsxStateStatus::Active;
    f.gsxService.boardedPassengers = 50;
    f.gsxService.cargoPercent = 50.0;

    QVERIFY(!Tick(state, f).has_value());
    QVERIFY(f.ctx.data.serviceInterrupted);
    QVERIFY(Logged(f, "GSX dropped the refueling it had already started"));
    QCOMPARE(f.ctx.data.boardingProgress, 50.0);

    f.gsxService.boardingState = GsxStateStatus::Completed;

    QVERIFY(!Tick(state, f).has_value());
    QVERIFY(f.ctx.data.boardingFinished);
    QVERIFY(!f.ctx.data.refuelFinished);
}

void LoadingStateTest::aDroppedBoardingDoesNotHoldTheRefuel()
{
    TurnaroundStateFixture f;
    LoadingState state;

    ArrangeClientRefuel(f, 10.0, 1000.0, 5000.0);
    ArrangePassengerBoarding(f);
    f.gsxService.boardingState = GsxStateStatus::Active;

    QVERIFY(!Tick(state, f).has_value());

    f.gsxService.boardingState = GsxStateStatus::Callable;

    for (int tick = 0; tick < 5; ++tick)
    {
        QVERIFY(!Tick(state, f).has_value());
    }

    QVERIFY(f.ctx.data.serviceInterrupted);
    QVERIFY(Logged(f, "GSX dropped the boarding it had already started"));
    QCOMPARE(f.aircraft.currentFuelKg, 1060.0);
}

void LoadingStateTest::holdsTheBoardingCompleteNowUntilTheRefuelFinishes()
{
    TurnaroundStateFixture f;
    LoadingState state;

    ArrangeClientRefuel(f, 0.0, 1000.0, 2000.0);
    f.aircraft.refuelMethod = RefuelBy::Gsx;
    f.aircraft.cargo = true;
    f.ctx.data.plannedZfwKg = 180000.0;
    f.gsxService.boardingState = GsxStateStatus::Active;
    f.gsxService.cargoPercent = 100.0;

    for (int tick = 0; tick < 200; ++tick)
    {
        QVERIFY(!Tick(state, f).has_value());
    }

    QCOMPARE(f.menuGateway.completeBoardingCalls, 0);

    f.aircraft.currentFuelKg = 2000.0;
    f.gsxService.refuelingState = GsxStateStatus::Completed;
    f.gsxService.hoseConnected = false;

    for (int tick = 0; tick < 89; ++tick)
    {
        QVERIFY(!Tick(state, f).has_value());
    }

    QVERIFY(f.ctx.data.refuelFinished);
    QCOMPARE(f.menuGateway.completeBoardingCalls, 0);

    QVERIFY(!Tick(state, f).has_value());
    QCOMPARE(f.menuGateway.completeBoardingCalls, 1);
}

void LoadingStateTest::theRampNeverPausesFromTheFirstToTheLastFuelTick()
{
    TurnaroundStateFixture f;
    LoadingState state;

    ArrangeClientRefuel(f, 10.0, 1000.0, 2000.0);
    ArrangePassengerBoarding(f);

    for (int tick = 1; tick <= 100; ++tick)
    {
        if (tick == 30)
        {
            f.gsxService.hoseConnected = false;
        }
        else if (tick == 74)
        {
            f.gsxService.hoseConnected = true;
        }
        else if (tick == 80)
        {
            f.gsxService.boardingState = GsxStateStatus::Active;
            f.gsxService.boardedPassengers = 10;
        }

        QVERIFY(!Tick(state, f).has_value());
        QCOMPARE(f.aircraft.currentFuelKg, 1000.0 + tick * 10.0);
    }

    QCOMPARE(f.menuGateway.boardingCalls, 1);
    QVERIFY(f.ctx.data.boardingBaselined);
    QVERIFY(!f.ctx.data.refuelFinished);
}

void LoadingStateTest::logsTheRefuelEndingBeforeGsxConfirmedTheBoarding()
{
    TurnaroundStateFixture f;
    LoadingState state;

    ArrangeClientRefuel(f, 0.0, 1000.0, 2000.0);
    f.aircraft.refuelMethod = RefuelBy::Gsx;

    QVERIFY(!Tick(state, f).has_value());

    f.aircraft.currentFuelKg = 2000.0;
    f.gsxService.refuelingState = GsxStateStatus::Completed;
    f.gsxService.hoseConnected = false;

    QVERIFY(!Tick(state, f).has_value());

    QVERIFY(f.ctx.data.refuelFinished);
    QVERIFY(Logged(f, "Loading: the refueling finished before GSX confirmed the boarding (boarding state 1)"));
}

void LoadingStateTest::staysQuietWhenGsxConfirmedTheBoardingBeforeTheRefuelEnded()
{
    TurnaroundStateFixture f;
    LoadingState state;

    ArrangeClientRefuel(f, 0.0, 1000.0, 2000.0);
    f.aircraft.refuelMethod = RefuelBy::Gsx;
    f.gsxService.boardingState = GsxStateStatus::Requested;

    QVERIFY(!Tick(state, f).has_value());

    f.gsxService.boardingState = GsxStateStatus::Callable;
    f.aircraft.currentFuelKg = 2000.0;
    f.gsxService.refuelingState = GsxStateStatus::Completed;
    f.gsxService.hoseConnected = false;

    QVERIFY(!Tick(state, f).has_value());

    QVERIFY(f.ctx.data.refuelFinished);
    QVERIFY(!Logged(f, "before GSX confirmed the boarding"));
}

QTEST_APPLESS_MAIN(LoadingStateTest)

#include "tst_loading_state.moc"
