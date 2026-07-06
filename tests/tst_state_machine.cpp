#include <QtTest/QTest>

#include <array>
#include <vector>
#include "tests/turnaround/TurnaroundStateFixture.h"
#include "src/domain/turnaround/TurnaroundStateMachine.h"

namespace
{
    constexpr auto kReachableWorkflowPhases = std::array{
        TurnaroundPhase::WaitingSupportedAircraft,
        TurnaroundPhase::WaitingAircraftReady,
        TurnaroundPhase::WaitingFlightPlan,
        TurnaroundPhase::RepositionAircraft,
        TurnaroundPhase::CallStairsOrJetway,
        TurnaroundPhase::WaitingPowerOn,
        TurnaroundPhase::RequestFuel,
        TurnaroundPhase::Refueling,
        TurnaroundPhase::RequestBoarding,
        TurnaroundPhase::Boarding,
        TurnaroundPhase::WaitingReadyToPush,
        TurnaroundPhase::RequestPushback,
        TurnaroundPhase::WaitingPushbackToStart,
        TurnaroundPhase::WaitingForEngines,
        TurnaroundPhase::WaitingDeparture,
        TurnaroundPhase::OnFlight,
        TurnaroundPhase::WaitingEngineShutdown,
        TurnaroundPhase::RequestDeboarding,
        TurnaroundPhase::Deboarding,
        TurnaroundPhase::WaitingNewFlight,
        TurnaroundPhase::WaitingSupportedAircraft,
    };

    void PrepareFlightPlan(FakeAircraft& aircraft)
    {
        aircraft.flightPlanLoaded = true;
        aircraft.progressiveFuel = false;
        aircraft.progressiveLoad = false;
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
            : machine(&f.status, &f.settings, &f.gsxService, &f.menuGateway, &f.logger)
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
            TickHolding(TurnaroundPhase::WaitingAircraftReady);
            FinishDelay(10, TurnaroundPhase::WaitingFlightPlan);
        }

        void LoadFlightPlan()
        {
            PrepareFlightPlan(f.aircraft);
            f.gsxService.simbriefLoaded = true;

            TickTo(TurnaroundPhase::RepositionAircraft);
        }

        void CompleteReposition()
        {
            TickHolding(TurnaroundPhase::RepositionAircraft);

            f.gsxService.repositioning = true;
            TickHolding(TurnaroundPhase::RepositionAircraft);

            f.gsxService.repositioning = false;
            TickTo(TurnaroundPhase::CallStairsOrJetway);
        }

        void CompleteGroundServiceSetup()
        {
            f.gsxService.stairsAvailable = true;
            TickHolding(TurnaroundPhase::CallStairsOrJetway);

            f.gsxService.stairsAvailable = false;
            f.gsxService.stairsInPlace = true;
            TickTo(TurnaroundPhase::WaitingPowerOn);

            f.aircraft.powered = true;
            TickTo(TurnaroundPhase::RequestFuel);
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

            TickHolding(TurnaroundPhase::Deboarding);
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
        workflow.LoadFlightPlan();
        workflow.CompleteReposition();
        workflow.CompleteGroundServiceSetup();
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
    static void resetReturnsToWaitingSupportedAircraft();
    static void waitsForRefuelingTransitionDelay();
    static void waitsForBoardingTransitionDelay();
    static void completesReachableWorkflowAndReturnsToStart();
};

void TurnaroundStateMachineTest::startsInWaitingSupportedAircraft()
{
    TurnaroundWorkflow workflow;

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

    workflow.TickHolding(TurnaroundPhase::WaitingFlightPlan);

    QVERIFY(workflow.f.aircraft.consumeSmartSwitchCalls > 0);
    bool logged = false;
    for (const std::string& message : workflow.f.logger.messages)
    {
        if (message.find("SmartSwitch pressed") != std::string::npos)
        {
            logged = true;
        }
    }
    QVERIFY(logged);
}

void TurnaroundStateMachineTest::unconsumedSmartSwitchPressIsDiscarded()
{
    TurnaroundWorkflow workflow;
    workflow.AttachAircraft();

    // Press em fase que nao consome (WaitingFlightPlan): a informacao vive um tick e e' descartada.
    workflow.f.aircraft.smartSwitchActivated = true;
    workflow.TickHolding(TurnaroundPhase::WaitingFlightPlan);

    workflow.f.aircraft.smartSwitchActivated = false;
    workflow.TickHolding(TurnaroundPhase::WaitingFlightPlan);

    // Se o press antigo tivesse ficado retido, o fluxo abaixo confirmaria motores sem um press novo.
    workflow.LoadFlightPlan();
    workflow.CompleteReposition();
    workflow.CompleteGroundServiceSetup();
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

    QCOMPARE(workflow.machine.GetPhase(), TurnaroundPhase::WaitingFlightPlan);
}

void TurnaroundStateMachineTest::resetReturnsToWaitingSupportedAircraft()
{
    TurnaroundWorkflow workflow;
    workflow.AttachAircraft();

    QVERIFY(workflow.machine.GetPhase() != TurnaroundPhase::WaitingSupportedAircraft);

    workflow.machine.Reset();

    QCOMPARE(workflow.machine.GetPhase(), TurnaroundPhase::WaitingSupportedAircraft);
}

void TurnaroundStateMachineTest::waitsForRefuelingTransitionDelay()
{
    TurnaroundWorkflow workflow;
    ReachRefueling(workflow);

    workflow.BeginRefuelingDelay();
    workflow.FinishDelay(29, TurnaroundPhase::Refueling);
    workflow.TickTo(TurnaroundPhase::RequestBoarding);
}

void TurnaroundStateMachineTest::waitsForBoardingTransitionDelay()
{
    TurnaroundWorkflow workflow;
    ReachBoarding(workflow);

    workflow.BeginBoardingDelay();
    workflow.FinishDelay(59, TurnaroundPhase::Boarding);
    workflow.TickTo(TurnaroundPhase::WaitingReadyToPush);
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

QTEST_APPLESS_MAIN(TurnaroundStateMachineTest)

#include "tst_state_machine.moc"
