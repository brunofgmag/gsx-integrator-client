#include <QtTest/QTest>

#include <algorithm>
#include <array>
#include <string>
#include <vector>
#include "tests/doubles/FakeVariableGateway.h"
#include "tests/turnaround/TurnaroundStateFixture.h"
#include "src/domain/turnaround/PilotTouch.h"
#include "src/domain/turnaround/PilotUnlock.h"
#include "src/domain/ports/AircraftRule.h"
#include "src/domain/turnaround/TurnaroundStateMachine.h"

namespace
{
    constexpr auto kWatchedLVar = "EXT_Door_stairs_pos";

    constexpr auto kReachableWorkflowPhases = std::array{
        TurnaroundPhase::WaitingSupportedAircraft,
        TurnaroundPhase::WaitingAircraftReady,
        TurnaroundPhase::RepositionAircraft,
        TurnaroundPhase::PlaceGroundEquipment,
        TurnaroundPhase::CallServices,
        TurnaroundPhase::WaitingFlightPlan,
        TurnaroundPhase::WaitingPowerOn,
        TurnaroundPhase::CallCatering,
        TurnaroundPhase::RequestFuel,
        TurnaroundPhase::Refueling,
        TurnaroundPhase::RequestBoarding,
        TurnaroundPhase::Boarding,
        TurnaroundPhase::WaitingReadyToPush,
        TurnaroundPhase::WaitCatering,
        TurnaroundPhase::RemoveGroundEquipment,
        TurnaroundPhase::RequestPushback,
        TurnaroundPhase::WaitingPushbackToStart,
        TurnaroundPhase::WaitingForEngines,
        TurnaroundPhase::WaitingDeparture,
        TurnaroundPhase::OnFlight,
        TurnaroundPhase::WaitingEngineShutdown,
        TurnaroundPhase::PlaceArrivalGroundEquipment,
        TurnaroundPhase::RequestDeboarding,
        TurnaroundPhase::Deboarding,
        TurnaroundPhase::CabinServices,
        TurnaroundPhase::WaitingNewFlight,
        TurnaroundPhase::WaitingSupportedAircraft,
    };

    class CountingRule : public AircraftRule
    {
    public:
        int evaluateCalls = 0;
        int actCalls = 0;

        [[nodiscard]] const char* Name() const override
        {
            return "counting-rule";
        }

        [[nodiscard]] RuleVerdict Evaluate(const RuleContext&) override
        {
            ++evaluateCalls;

            return RuleVerdict::Pass();
        }

        void Act(const RuleContext&, VariableWriter&) override
        {
            ++actCalls;
        }
    };

    class ChangeQueryRule final : public AircraftRule
    {
    public:
        std::vector<bool> answers;

        ChangeQueryRule(VariableReader& reader, const char* ruleName, const char* lvar)
            : reader_(&reader), ruleName_(ruleName), lvar_(lvar)
        {
        }

        [[nodiscard]] const char* Name() const override
        {
            return ruleName_;
        }

        [[nodiscard]] RuleVerdict Evaluate(const RuleContext&) override
        {
            answers.push_back(reader_->HasLVarChangedThisTick(lvar_));

            return RuleVerdict::Pass();
        }

        void Act(const RuleContext&, VariableWriter&) override
        {
        }

    private:
        VariableReader* reader_;
        const char* ruleName_;
        const char* lvar_;
    };

    class SpanQueryRule final : public AircraftRule
    {
    public:
        std::vector<double> swings;

        SpanQueryRule(VariableReader& reader, const char* ruleName, const char* lvar)
            : reader_(&reader), ruleName_(ruleName), lvar_(lvar)
        {
        }

        [[nodiscard]] const char* Name() const override
        {
            return ruleName_;
        }

        [[nodiscard]] RuleVerdict Evaluate(const RuleContext&) override
        {
            const LVarSpan span = reader_->ConsumeLVarSpan(lvar_);
            swings.push_back(span.max - span.min);

            return RuleVerdict::Pass();
        }

        void Act(const RuleContext&, VariableWriter&) override
        {
        }

    private:
        VariableReader* reader_;
        const char* ruleName_;
        const char* lvar_;
    };

    class SlowCountingRule final : public CountingRule
    {
    public:
        [[nodiscard]] const char* Name() const override
        {
            return "slow-counting-rule";
        }

        [[nodiscard]] RuleCadence Cadence() const override
        {
            return RuleCadence::Slow;
        }
    };

    void PrepareFlightPlan(FakeAircraft& aircraft)
    {
        aircraft.flightPlanLoaded = true;
        aircraft.refuelMethod = RefuelBy::Self;
        aircraft.boardMethod = BoardBy::Self;
        aircraft.plannedFuelKg = 12000.0;
        aircraft.plannedZfwKg = 180000.0;
        aircraft.emptyZfwKg = 130000.0;
        aircraft.plannedPax = 210;
        aircraft.currentFuelKg = 5000.0;
        aircraft.currentZfwKg = 160000.0;
    }

    void ComparePhases(const std::vector<TurnaroundPhase>& actual,
                       const std::array<TurnaroundPhase, kReachableWorkflowPhases.size()>& expected)
    {
        QCOMPARE(actual.size(), expected.size());

        for (std::size_t index = 0; index < expected.size(); ++index)
        {
            QCOMPARE(actual[index], expected[index]);
        }
    }

    class TurnaroundWorkflow
    {
    public:
        TurnaroundStateFixture f;
        TurnaroundStateMachine machine;
        std::vector<TurnaroundPhase> visitedPhases;

        TurnaroundWorkflow()
            : machine(&f.status, &f.settings, &f.gsxService, &f.menuGateway, &f.logger, &f.variableWriter)
        {
            visitedPhases.push_back(machine.GetPhase());
        }

        void TickHolding(const TurnaroundPhase expected)
        {
            machine.Tick();
            QCOMPARE(machine.GetPhase(), expected);
        }

        void TickTo(const TurnaroundPhase expected)
        {
            machine.Tick();
            QCOMPARE(machine.GetPhase(), expected);
            RecordCurrentPhase();
        }

        void FinishDelay(const int ticks, const TurnaroundPhase expected)
        {
            for (int tick = 0; tick < ticks; ++tick)
            {
                machine.Tick();
            }

            QCOMPARE(machine.GetPhase(), expected);
            RecordCurrentPhase();
        }

        void AttachAircraft()
        {
            f.aircraft.engineRunning = true;
            machine.AttachAircraft(&f.aircraft);

            TickTo(TurnaroundPhase::WaitingAircraftReady);
            TickHolding(TurnaroundPhase::WaitingAircraftReady);

            f.aircraft.engineRunning = false;
            TickTo(TurnaroundPhase::RepositionAircraft);
        }

        void LoadFlightPlan()
        {
            PrepareFlightPlan(f.aircraft);
            f.gsxService.simbriefLoaded = true;

            TickTo(TurnaroundPhase::WaitingPowerOn);

            f.aircraft.powered = true;
            TickTo(TurnaroundPhase::CallCatering);
            TickTo(TurnaroundPhase::RequestFuel);
        }

        void CompleteReposition()
        {
            TickHolding(TurnaroundPhase::RepositionAircraft);

            f.gsxService.repositioning = true;
            TickHolding(TurnaroundPhase::RepositionAircraft);

            f.gsxService.repositioning = false;
            TickTo(TurnaroundPhase::PlaceGroundEquipment);
            TickTo(TurnaroundPhase::CallServices);
        }

        void CompleteGroundServiceSetup()
        {
            f.gsxService.stairsAvailable = true;
            TickHolding(TurnaroundPhase::CallServices);

            f.gsxService.stairsAvailable = false;
            f.gsxService.stairsInPlace = true;
            TickHolding(TurnaroundPhase::CallServices);
            TickHolding(TurnaroundPhase::CallServices);
            TickTo(TurnaroundPhase::WaitingFlightPlan);
        }

        void RequestFuel()
        {
            f.gsxService.refuelingState = GsxStateStatus::Callable;

            TickHolding(TurnaroundPhase::RequestFuel);
        }

        void StartRefueling()
        {
            f.gsxService.refuelingState = GsxStateStatus::Active;
            f.gsxService.hoseConnected = true;

            TickTo(TurnaroundPhase::Refueling);
        }

        void BeginRefuelingDelay()
        {
            f.gsxService.refuelingState = GsxStateStatus::Completed;
            f.gsxService.hoseConnected = false;

            TickHolding(TurnaroundPhase::Refueling);
        }

        void CompleteRefueling()
        {
            BeginRefuelingDelay();

            f.gsxService.boardingState = GsxStateStatus::Callable;
            FinishDelay(30, TurnaroundPhase::RequestBoarding);
        }

        void StartBoarding()
        {
            f.gsxService.boardingState = GsxStateStatus::Active;
            f.gsxService.boardedPassengers = f.aircraft.plannedPax;
            f.gsxService.cargoPercent = 100.0;

            TickTo(TurnaroundPhase::Boarding);
        }

        void BeginBoardingDelay()
        {
            f.gsxService.boardingState = GsxStateStatus::Completed;

            TickHolding(TurnaroundPhase::Boarding);
        }

        void CompleteBoarding()
        {
            BeginBoardingDelay();
            FinishDelay(60, TurnaroundPhase::WaitingReadyToPush);
        }

        void RequestPushback()
        {
            f.aircraft.readyToPush = true;
            f.aircraft.parkingBrakeSet = true;
            TickTo(TurnaroundPhase::WaitCatering);
            TickTo(TurnaroundPhase::RemoveGroundEquipment);
            TickTo(TurnaroundPhase::RequestPushback);

            f.gsxService.departureState = GsxStateStatus::Callable;
            TickHolding(TurnaroundPhase::RequestPushback);
        }

        void StartPushback()
        {
            f.gsxService.departureState = GsxStateStatus::Requested;

            TickTo(TurnaroundPhase::WaitingPushbackToStart);
        }

        void StartPushbackMovement()
        {
            f.gsxService.departureState = GsxStateStatus::Active;
            f.gsxService.pushbackStarted = true;

            TickTo(TurnaroundPhase::WaitingForEngines);
        }

        void ConfirmEngineStart()
        {
            f.aircraft.engineRunning = true;
            f.aircraft.parkingBrakeSet = true;
            f.gsxService.goodEngineStartConfirmation = true;
            f.gsxService.waitingForEngines = true;
            f.aircraft.smartSwitchActivated = true;

            TickTo(TurnaroundPhase::WaitingDeparture);

            f.aircraft.smartSwitchActivated = false;
            f.gsxService.waitingForEngines = false;
        }

        void Depart()
        {
            f.gsxService.onGround = false;

            TickHolding(TurnaroundPhase::WaitingDeparture);
            FinishDelay(30, TurnaroundPhase::OnFlight);
        }

        void Land()
        {
            f.gsxService.onGround = true;

            TickTo(TurnaroundPhase::WaitingEngineShutdown);

            f.aircraft.engineRunning = false;
            TickTo(TurnaroundPhase::PlaceArrivalGroundEquipment);
        }

        void RequestDeboarding()
        {
            f.aircraft.readyToDeboard = true;

            TickTo(TurnaroundPhase::RequestDeboarding);

            f.gsxService.deboardingState = GsxStateStatus::Callable;
            TickHolding(TurnaroundPhase::RequestDeboarding);
        }

        void StartDeboarding()
        {
            f.gsxService.deboardingState = GsxStateStatus::Active;
            f.gsxService.deboardedPassengers = 10;

            TickTo(TurnaroundPhase::Deboarding);
        }

        void CompleteDeboarding()
        {
            f.gsxService.deboardingState = GsxStateStatus::Completed;
            f.gsxService.deboardedPassengers = f.aircraft.plannedPax;
            f.gsxService.deboardingCargoPercent = 100.0;

            TickTo(TurnaroundPhase::CabinServices);
            TickHolding(TurnaroundPhase::CabinServices);
            FinishDelay(60, TurnaroundPhase::WaitingNewFlight);
        }

        void StartNewFlightCycle()
        {
            f.aircraft.smartSwitchActivated = true;

            TickTo(TurnaroundPhase::WaitingSupportedAircraft);
        }

    private:
        void RecordCurrentPhase()
        {
            const TurnaroundPhase current = machine.GetPhase();
            if (visitedPhases.empty() || visitedPhases.back() != current)
            {
                visitedPhases.push_back(current);
            }
        }
    };

    void ReachRequestFuel(TurnaroundWorkflow& workflow)
    {
        workflow.AttachAircraft();
        workflow.CompleteReposition();
        workflow.CompleteGroundServiceSetup();
        workflow.LoadFlightPlan();
    }

    void ReachRefueling(TurnaroundWorkflow& workflow)
    {
        ReachRequestFuel(workflow);
        workflow.RequestFuel();
        workflow.StartRefueling();
    }

    void ReachBoarding(TurnaroundWorkflow& workflow)
    {
        ReachRefueling(workflow);
        workflow.CompleteRefueling();
        workflow.StartBoarding();
    }

    bool Logged(const TurnaroundWorkflow& workflow, const std::string& fragment)
    {
        return std::ranges::any_of(workflow.f.logger.messages, [&fragment](const std::string& message)
        {
            return message.find(fragment) != std::string::npos;
        });
    }
}

class TurnaroundStateMachineTest final : public QObject
{
    Q_OBJECT

private slots:
    static void startsInWaitingSupportedAircraft();
    static void tickWithoutAircraftDoesNotPollSmartSwitch();
    static void smartSwitchPressIsLoggedEveryTick();
    static void unconsumedSmartSwitchPressIsDiscarded();
    static void attachAircraftAllowsLeavingWaitingSupportedAircraft();
    static void holdsRepositionUntilGsxAvailable();
    static void resetReturnsToWaitingSupportedAircraft();
    static void holdsAtRequestFuelUntilLoadingConfirmed();
    static void waitsForRefuelingTransitionDelay();
    static void aDelayedTransitionKeepsTheFastRulesRunning();
    static void theSlowTickActsOnlyWhenTheMachineIsDriving();
    static void waitsForBoardingTransitionDelay();
    static void holdsBoardingWhileCargoIsPending();
    static void completesReachableWorkflowAndReturnsToStart();
    static void theTurnaroundTurnNotifiesTheMenuGateway();
    static void theStartOfThePushMovementNotifiesTheMenuGateway();
    static void publishesCurrentTankFuelBeforeRefuel();
    static void publishesLoadingTargetsAfterFlightPlanCapture();
    static void debugSkipPhaseClampsToEnumRange();
    static void theClosedListHoldsOnlyTheGateTheSliceNamed();
    static void theSmartSwitchUnlocksThePushbackGateHeldByADoor();
    static void aTouchOnTheTickTheGateIsReachedIsNotSwallowed();
    static void resetClearsThePilotOrigin();
    static void aPhaseOutsideTheClosedListIgnoresTheSmartSwitch();
    static void advancingByReadingPublishesTheReadingOrigin();
    static void theTouchListHoldsThePhasesTheSwitchActsOn();
    static void theUnlockListIsContainedInTheTouchList();
    static void theAppTouchUnlocksTheGateLikeTheSwitchDoes();
    static void bothSurfacesTouchingOnTheSameTickSpendOneEdge();
    static void theLogNamesTheSurfaceTheTouchCameFrom();
    static void resetDiscardsAPendingAppTouch();
    static void twoRulesAskingAboutOneVariableGetTheSameAnswer();
    static void theSpanPairAnswersTheSecondRuleDifferently();
};

void TurnaroundStateMachineTest::publishesCurrentTankFuelBeforeRefuel()
{
    TurnaroundWorkflow workflow;
    workflow.f.aircraft.currentFuelKg = 5000.0;
    workflow.machine.AttachAircraft(&workflow.f.aircraft);

    workflow.machine.Tick();

    QCOMPARE(workflow.f.status.loadedFuelKg, 5000.0);

    workflow.f.aircraft.currentFuelKg = 5100.0;
    workflow.machine.Tick();

    QCOMPARE(workflow.f.status.loadedFuelKg, 5100.0);
}

void TurnaroundStateMachineTest::publishesLoadingTargetsAfterFlightPlanCapture()
{
    TurnaroundWorkflow workflow;
    workflow.AttachAircraft();
    workflow.CompleteReposition();
    workflow.CompleteGroundServiceSetup();
    workflow.LoadFlightPlan();

    QCOMPARE(workflow.f.status.targetFuelKg, 12000.0);
    QCOMPARE(workflow.f.status.targetZfwKg, 180000.0);
    QCOMPARE(workflow.f.status.targetPassengers, 210);
}

void TurnaroundStateMachineTest::startsInWaitingSupportedAircraft()
{
    const TurnaroundWorkflow workflow;

    QCOMPARE(workflow.machine.GetPhase(), TurnaroundPhase::WaitingSupportedAircraft);
}

void TurnaroundStateMachineTest::tickWithoutAircraftDoesNotPollSmartSwitch()
{
    TurnaroundWorkflow workflow;

    workflow.machine.Tick();

    QCOMPARE(workflow.f.aircraft.consumeSmartSwitchCalls, 0);
    QVERIFY(workflow.f.logger.messages.empty());
}

void TurnaroundStateMachineTest::smartSwitchPressIsLoggedEveryTick()
{
    TurnaroundWorkflow workflow;
    workflow.AttachAircraft();

    workflow.f.logger.messages.clear();
    workflow.f.aircraft.smartSwitchActivated = true;

    workflow.TickHolding(TurnaroundPhase::RepositionAircraft);

    QVERIFY(workflow.f.aircraft.consumeSmartSwitchCalls > 0);
    QVERIFY(Logged(workflow, "Pilot touch from the SmartSwitch"));
}

void TurnaroundStateMachineTest::unconsumedSmartSwitchPressIsDiscarded()
{
    TurnaroundWorkflow workflow;
    workflow.AttachAircraft();

    workflow.f.aircraft.smartSwitchActivated = true;
    workflow.TickHolding(TurnaroundPhase::RepositionAircraft);

    workflow.f.aircraft.smartSwitchActivated = false;
    workflow.TickHolding(TurnaroundPhase::RepositionAircraft);

    workflow.CompleteReposition();
    workflow.CompleteGroundServiceSetup();
    workflow.LoadFlightPlan();
    workflow.RequestFuel();
    workflow.StartRefueling();
    workflow.CompleteRefueling();
    workflow.StartBoarding();
    workflow.CompleteBoarding();
    workflow.RequestPushback();
    workflow.StartPushback();
    workflow.StartPushbackMovement();

    workflow.f.aircraft.engineRunning = true;
    workflow.f.aircraft.parkingBrakeSet = true;
    workflow.f.gsxService.goodEngineStartConfirmation = true;
    workflow.f.gsxService.waitingForEngines = true;

    workflow.TickHolding(TurnaroundPhase::WaitingForEngines);
    QCOMPARE(workflow.f.menuGateway.confirmGoodEnginesCalls, 0);
}

void TurnaroundStateMachineTest::attachAircraftAllowsLeavingWaitingSupportedAircraft()
{
    TurnaroundWorkflow workflow;

    workflow.AttachAircraft();

    QCOMPARE(workflow.machine.GetPhase(), TurnaroundPhase::RepositionAircraft);
}

void TurnaroundStateMachineTest::holdsRepositionUntilGsxAvailable()
{
    TurnaroundWorkflow workflow;
    workflow.f.status.gsxAvailable = false;
    workflow.AttachAircraft();

    workflow.TickHolding(TurnaroundPhase::RepositionAircraft);
    workflow.TickHolding(TurnaroundPhase::RepositionAircraft);

    QCOMPARE(workflow.f.menuGateway.repositionCalls, 0);

    workflow.f.status.gsxAvailable = true;
    workflow.TickHolding(TurnaroundPhase::RepositionAircraft);

    QCOMPARE(workflow.f.menuGateway.repositionCalls, 1);
}

void TurnaroundStateMachineTest::resetReturnsToWaitingSupportedAircraft()
{
    TurnaroundWorkflow workflow;
    workflow.AttachAircraft();

    QVERIFY(workflow.machine.GetPhase() != TurnaroundPhase::WaitingSupportedAircraft);

    workflow.machine.Reset();

    QCOMPARE(workflow.machine.GetPhase(), TurnaroundPhase::WaitingSupportedAircraft);
}

void TurnaroundStateMachineTest::holdsAtRequestFuelUntilLoadingConfirmed()
{
    TurnaroundWorkflow workflow;
    workflow.f.settings.autoStartLoading = false;

    ReachRequestFuel(workflow);
    workflow.f.gsxService.refuelingState = GsxStateStatus::Callable;

    workflow.TickHolding(TurnaroundPhase::RequestFuel);
    workflow.TickHolding(TurnaroundPhase::RequestFuel);

    QCOMPARE(workflow.f.menuGateway.refuelingCalls, 0);
    QVERIFY(!workflow.machine.IsLoadingConfirmed());

    workflow.machine.ConfirmLoading();

    QVERIFY(workflow.machine.IsLoadingConfirmed());

    workflow.TickHolding(TurnaroundPhase::RequestFuel);

    QCOMPARE(workflow.f.menuGateway.refuelingCalls, 1);

    workflow.StartRefueling();
}

void TurnaroundStateMachineTest::waitsForRefuelingTransitionDelay()
{
    TurnaroundWorkflow workflow;
    ReachRefueling(workflow);

    workflow.BeginRefuelingDelay();

    QCOMPARE(workflow.machine.GetDelayTicksRemaining(), 30);

    workflow.FinishDelay(29, TurnaroundPhase::Refueling);

    QCOMPARE(workflow.machine.GetDelayTicksRemaining(), 1);

    workflow.TickTo(TurnaroundPhase::RequestBoarding);

    QCOMPARE(workflow.machine.GetDelayTicksRemaining(), 0);
}

void TurnaroundStateMachineTest::waitsForBoardingTransitionDelay()
{
    TurnaroundWorkflow workflow;
    ReachBoarding(workflow);

    workflow.BeginBoardingDelay();
    workflow.FinishDelay(59, TurnaroundPhase::Boarding);
    workflow.TickTo(TurnaroundPhase::WaitingReadyToPush);
}

void TurnaroundStateMachineTest::holdsBoardingWhileCargoIsPending()
{
    TurnaroundWorkflow workflow;
    ReachBoarding(workflow);

    workflow.f.gsxService.loaderWaitingForDoor = true;
    workflow.f.gsxService.cargoPercent = 67.0;
    workflow.f.gsxService.boardingState = GsxStateStatus::Completed;

    workflow.TickHolding(TurnaroundPhase::Boarding);
    QCOMPARE(workflow.machine.GetDelayTicksRemaining(), 0);

    workflow.f.gsxService.loaderWaitingForDoor = false;
    workflow.f.gsxService.loadingCargo = true;

    workflow.TickHolding(TurnaroundPhase::Boarding);
    QCOMPARE(workflow.machine.GetDelayTicksRemaining(), 0);

    workflow.f.gsxService.loadingCargo = false;
    workflow.f.gsxService.cargoPercent = 100.0;

    workflow.CompleteBoarding();
}

void TurnaroundStateMachineTest::theTurnaroundTurnNotifiesTheMenuGateway()
{
    TurnaroundWorkflow workflow;

    ReachBoarding(workflow);
    workflow.CompleteBoarding();
    workflow.RequestPushback();
    workflow.StartPushback();
    workflow.StartPushbackMovement();
    workflow.ConfirmEngineStart();
    workflow.Depart();
    workflow.Land();
    workflow.RequestDeboarding();
    workflow.StartDeboarding();

    QCOMPARE(workflow.f.menuGateway.turnaroundTurnedCalls, 0);

    workflow.CompleteDeboarding();

    QCOMPARE(workflow.f.menuGateway.turnaroundTurnedCalls, 1);
}

void TurnaroundStateMachineTest::theStartOfThePushMovementNotifiesTheMenuGateway()
{
    TurnaroundWorkflow workflow;

    ReachBoarding(workflow);
    workflow.CompleteBoarding();
    workflow.RequestPushback();
    workflow.StartPushback();

    QCOMPARE(workflow.f.menuGateway.pushbackStartedCalls, 0);

    workflow.StartPushbackMovement();

    QCOMPARE(workflow.f.menuGateway.pushbackStartedCalls, 1);
}

void TurnaroundStateMachineTest::completesReachableWorkflowAndReturnsToStart()
{
    TurnaroundWorkflow workflow;

    ReachBoarding(workflow);
    workflow.CompleteBoarding();
    workflow.RequestPushback();
    workflow.StartPushback();
    workflow.StartPushbackMovement();
    workflow.ConfirmEngineStart();
    workflow.Depart();
    workflow.Land();
    workflow.RequestDeboarding();
    workflow.StartDeboarding();
    workflow.CompleteDeboarding();
    workflow.StartNewFlightCycle();

    ComparePhases(workflow.visitedPhases, kReachableWorkflowPhases);
    QCOMPARE(workflow.f.menuGateway.repositionCalls, 1);
    QCOMPARE(workflow.f.menuGateway.callStairsCalls, 1);
    QCOMPARE(workflow.f.menuGateway.refuelingCalls, 1);
    QCOMPARE(workflow.f.menuGateway.boardingCalls, 1);
    QCOMPARE(workflow.f.menuGateway.pushbackCalls, 1);
    QCOMPARE(workflow.f.menuGateway.deboardingCalls, 1);
}

void TurnaroundStateMachineTest::debugSkipPhaseClampsToEnumRange()
{
#ifndef NDEBUG
    TurnaroundWorkflow workflow;

    workflow.machine.DebugSkipPhase(1);

    QCOMPARE(workflow.machine.GetPhase(), TurnaroundPhase::WaitingAircraftReady);

    workflow.machine.DebugSkipPhase(-5);

    QCOMPARE(workflow.machine.GetPhase(), TurnaroundPhase::WaitingSupportedAircraft);

    workflow.machine.DebugSkipPhase(static_cast<int>(TurnaroundPhase::Count));

    QCOMPARE(workflow.machine.GetPhase(), TurnaroundPhase::WaitingNewFlight);

    workflow.machine.DebugSkipPhase(1);

    QCOMPARE(workflow.machine.GetPhase(), TurnaroundPhase::WaitingNewFlight);
#else
    QSKIP("DebugSkipPhase is compiled out of Release builds");
#endif
}

void TurnaroundStateMachineTest::theClosedListHoldsOnlyTheGateTheSliceNamed()
{
    for (int index = 0; index < static_cast<int>(TurnaroundPhase::Count); ++index)
    {
        const auto phase = static_cast<TurnaroundPhase>(index);

        QCOMPARE(PilotUnlock::Accepts(phase), phase == TurnaroundPhase::WaitingReadyToPush);
    }
}

void TurnaroundStateMachineTest::theSmartSwitchUnlocksThePushbackGateHeldByADoor()
{
    TurnaroundWorkflow workflow;
    ReachBoarding(workflow);
    workflow.CompleteBoarding();

    workflow.f.aircraft.readyToPush = true;
    workflow.f.aircraft.parkingBrakeSet = true;
    workflow.f.aircraft.doorStatus = DoorStatus::AnyOpen;

    workflow.TickHolding(TurnaroundPhase::WaitingReadyToPush);

    workflow.f.aircraft.smartSwitchActivated = true;

    workflow.TickTo(TurnaroundPhase::WaitCatering);

    QCOMPARE(workflow.machine.GetLastTransitionOrigin(), TransitionOrigin::Pilot);
}

void TurnaroundStateMachineTest::aTouchOnTheTickTheGateIsReachedIsNotSwallowed()
{
    TurnaroundWorkflow workflow;
    ReachBoarding(workflow);
    workflow.BeginBoardingDelay();

    workflow.f.aircraft.readyToPush = true;
    workflow.f.aircraft.parkingBrakeSet = true;
    workflow.f.aircraft.doorStatus = DoorStatus::AnyOpen;

    for (int tick = 0; tick < 59; ++tick)
    {
        workflow.machine.Tick();
    }

    workflow.f.aircraft.smartSwitchActivated = true;
    workflow.machine.Tick();

    QCOMPARE(workflow.machine.GetPhase(), TurnaroundPhase::WaitCatering);
    QCOMPARE(workflow.machine.GetLastTransitionOrigin(), TransitionOrigin::Pilot);
}

void TurnaroundStateMachineTest::resetClearsThePilotOrigin()
{
    TurnaroundWorkflow workflow;
    ReachBoarding(workflow);
    workflow.CompleteBoarding();

    workflow.f.aircraft.readyToPush = true;
    workflow.f.aircraft.parkingBrakeSet = true;
    workflow.f.aircraft.doorStatus = DoorStatus::AnyOpen;
    workflow.f.aircraft.smartSwitchActivated = true;

    workflow.TickTo(TurnaroundPhase::WaitCatering);

    QCOMPARE(workflow.machine.GetLastTransitionOrigin(), TransitionOrigin::Pilot);

    workflow.machine.Reset();

    QCOMPARE(workflow.machine.GetLastTransitionOrigin(), TransitionOrigin::Reading);
}

void TurnaroundStateMachineTest::aPhaseOutsideTheClosedListIgnoresTheSmartSwitch()
{
    TurnaroundWorkflow workflow;
    ReachRefueling(workflow);
    workflow.CompleteRefueling();

    workflow.f.gsxService.boardingState = GsxStateStatus::Active;
    workflow.f.gsxService.boardedPassengers = workflow.f.aircraft.plannedPax;
    workflow.f.gsxService.cargoPercent = 50.0;

    workflow.TickTo(TurnaroundPhase::Boarding);

    workflow.f.aircraft.smartSwitchActivated = true;

    workflow.TickHolding(TurnaroundPhase::Boarding);
    workflow.TickHolding(TurnaroundPhase::Boarding);
}

void TurnaroundStateMachineTest::advancingByReadingPublishesTheReadingOrigin()
{
    TurnaroundWorkflow workflow;
    ReachBoarding(workflow);
    workflow.CompleteBoarding();

    workflow.f.aircraft.readyToPush = true;
    workflow.f.aircraft.parkingBrakeSet = true;
    workflow.f.aircraft.doorStatus = DoorStatus::AllClosed;

    workflow.TickTo(TurnaroundPhase::WaitCatering);

    QCOMPARE(workflow.machine.GetLastTransitionOrigin(), TransitionOrigin::Reading);
}

void TurnaroundStateMachineTest::theTouchListHoldsThePhasesTheSwitchActsOn()
{
    constexpr auto kAccepting = std::array{
        TurnaroundPhase::RequestFuel,
        TurnaroundPhase::WaitingReadyToPush,
        TurnaroundPhase::WaitingForEngines,
        TurnaroundPhase::WaitingNewFlight,
    };

    for (int index = 0; index < static_cast<int>(TurnaroundPhase::Count); ++index)
    {
        const auto phase = static_cast<TurnaroundPhase>(index);
        const bool expected = std::ranges::find(kAccepting, phase) != kAccepting.end();

        QCOMPARE(PilotTouch::Accepts(phase), expected);
    }
}

void TurnaroundStateMachineTest::theUnlockListIsContainedInTheTouchList()
{
    for (const TurnaroundPhase phase : PilotUnlock::kPhases)
    {
        QVERIFY(PilotTouch::Accepts(phase));
    }
}

void TurnaroundStateMachineTest::theAppTouchUnlocksTheGateLikeTheSwitchDoes()
{
    TurnaroundWorkflow workflow;
    ReachBoarding(workflow);
    workflow.CompleteBoarding();

    workflow.f.aircraft.readyToPush = true;
    workflow.f.aircraft.parkingBrakeSet = true;
    workflow.f.aircraft.doorStatus = DoorStatus::AnyOpen;

    workflow.TickHolding(TurnaroundPhase::WaitingReadyToPush);

    workflow.machine.AcceptAppTouch();

    workflow.TickTo(TurnaroundPhase::WaitCatering);

    QCOMPARE(workflow.machine.GetLastTransitionOrigin(), TransitionOrigin::Pilot);
    QCOMPARE(workflow.f.aircraft.consumeSmartSwitchCalls > 0, true);
}

void TurnaroundStateMachineTest::bothSurfacesTouchingOnTheSameTickSpendOneEdge()
{
    TurnaroundWorkflow workflow;
    ReachBoarding(workflow);
    workflow.CompleteBoarding();
    workflow.RequestPushback();
    workflow.StartPushback();
    workflow.StartPushbackMovement();

    workflow.f.aircraft.engineRunning = true;
    workflow.f.aircraft.parkingBrakeSet = true;
    workflow.f.gsxService.goodEngineStartConfirmation = true;
    workflow.f.gsxService.waitingForEngines = true;
    workflow.f.menuGateway.confirmGoodEnginesResult = false;

    workflow.f.aircraft.smartSwitchActivated = true;
    workflow.machine.AcceptAppTouch();

    workflow.TickHolding(TurnaroundPhase::WaitingForEngines);

    QCOMPARE(workflow.f.menuGateway.confirmGoodEnginesCalls, 1);

    workflow.f.aircraft.smartSwitchActivated = false;

    workflow.TickHolding(TurnaroundPhase::WaitingForEngines);

    QCOMPARE(workflow.f.menuGateway.confirmGoodEnginesCalls, 1);
}

void TurnaroundStateMachineTest::theLogNamesTheSurfaceTheTouchCameFrom()
{
    TurnaroundWorkflow workflow;
    workflow.AttachAircraft();

    workflow.f.logger.messages.clear();
    workflow.machine.AcceptAppTouch();
    workflow.TickHolding(TurnaroundPhase::RepositionAircraft);

    QVERIFY(Logged(workflow, "Pilot touch from the EFB app"));

    workflow.f.logger.messages.clear();
    workflow.f.aircraft.smartSwitchActivated = true;
    workflow.TickHolding(TurnaroundPhase::RepositionAircraft);

    QVERIFY(Logged(workflow, "Pilot touch from the SmartSwitch"));
}

void TurnaroundStateMachineTest::resetDiscardsAPendingAppTouch()
{
    TurnaroundWorkflow workflow;
    workflow.AttachAircraft();

    workflow.machine.AcceptAppTouch();
    workflow.machine.Reset();

    workflow.f.logger.messages.clear();
    workflow.machine.Tick();

    QVERIFY(!Logged(workflow, "Pilot touch"));
}

void TurnaroundStateMachineTest::aDelayedTransitionKeepsTheFastRulesRunning()
{
    TurnaroundWorkflow workflow;
    ReachRefueling(workflow);

    CountingRule rule;
    workflow.f.aircraft.rules = {&rule};

    workflow.BeginRefuelingDelay();

    QCOMPARE(workflow.machine.GetDelayTicksRemaining(), 30);

    const int actsAtDelayStart = rule.actCalls;

    workflow.FinishDelay(29, TurnaroundPhase::Refueling);

    QCOMPARE(rule.actCalls, actsAtDelayStart + 29);
}

void TurnaroundStateMachineTest::theSlowTickActsOnlyWhenTheMachineIsDriving()
{
    TurnaroundWorkflow workflow;
    SlowCountingRule rule;

    workflow.f.aircraft.rules = {&rule};
    workflow.machine.AttachAircraft(&workflow.f.aircraft);

    workflow.machine.ObserveSlowRules();

    QCOMPARE(rule.evaluateCalls, 1);
    QCOMPARE(rule.actCalls, 0);

    workflow.machine.TickSlowRules();

    QCOMPARE(rule.evaluateCalls, 2);
    QCOMPARE(rule.actCalls, 1);
}

void TurnaroundStateMachineTest::twoRulesAskingAboutOneVariableGetTheSameAnswer()
{
    FakeVariableGateway gateway;
    TurnaroundWorkflow workflow;
    ChangeQueryRule first(gateway, "first-change-query", kWatchedLVar);
    ChangeQueryRule second(gateway, "second-change-query", kWatchedLVar);

    workflow.f.aircraft.rules = {&first, &second};
    workflow.machine.AttachAircraft(&workflow.f.aircraft);

    gateway.lvars[kWatchedLVar] = 50.0;
    gateway.MarkTick();
    workflow.machine.Tick();

    QCOMPARE(first.answers, std::vector<bool>{true});
    QCOMPARE(second.answers, first.answers);

    gateway.MarkTick();
    workflow.machine.Tick();

    QCOMPARE(first.answers, (std::vector<bool>{true, false}));
    QCOMPARE(second.answers, first.answers);

    gateway.lvars[kWatchedLVar] = 120.0;
    gateway.MarkTick();
    workflow.machine.Tick();

    QCOMPARE(first.answers, (std::vector<bool>{true, false, true}));
    QCOMPARE(second.answers, first.answers);
}

void TurnaroundStateMachineTest::theSpanPairAnswersTheSecondRuleDifferently()
{
    FakeVariableGateway gateway;
    TurnaroundWorkflow workflow;
    SpanQueryRule first(gateway, "first-span-query", kWatchedLVar);
    SpanQueryRule second(gateway, "second-span-query", kWatchedLVar);

    workflow.f.aircraft.rules = {&first, &second};
    workflow.machine.AttachAircraft(&workflow.f.aircraft);

    gateway.lvars[kWatchedLVar] = 120.0;
    gateway.lvarSpans[kWatchedLVar] = LVarSpan{50.0, 120.0, true};
    gateway.MarkTick();
    workflow.machine.Tick();

    QCOMPARE(first.swings, std::vector<double>{70.0});
    QCOMPARE(second.swings, std::vector<double>{0.0});
}

QTEST_APPLESS_MAIN(TurnaroundStateMachineTest)

#include "tst_state_machine.moc"
