#include <QtTest/QTest>

#include <algorithm>
#include <array>
#include <format>
#include <optional>
#include <ranges>
#include <set>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>
#include "tests/doubles/FakeVariableGateway.h"
#include "tests/turnaround/TurnaroundStateFixture.h"
#include "tests/turnaround/TurnaroundDataFill.h"
#include "src/domain/model/MemoryBag.h"
#include "src/domain/model/PlanConversion.h"
#include "src/domain/turnaround/PilotTouch.h"
#include "src/domain/turnaround/PilotUnlock.h"
#include "src/domain/ports/AircraftRule.h"
#include "src/domain/turnaround/TurnaroundCheckpoint.h"
#include "src/domain/turnaround/TurnaroundMath.h"
#include "src/domain/turnaround/TurnaroundRestore.h"
#include "src/domain/turnaround/TurnaroundStateMachine.h"

namespace
{
    constexpr auto kWatchedLVar = "EXT_Door_stairs_pos";
    constexpr double kPushbackGroundSpeedKnots = 12.0;
    constexpr std::size_t kResumablePhaseCount = 21;
    constexpr double kFastFuelRateKgs = 1.0e6;
    constexpr int kTicksAfterTheResume = 3;
    constexpr int kTicksPastTheRepositionRetry = 12;
    constexpr int kTicksPastTheInterruptionWarning = 5;
    constexpr int kWalkTickLimit = 400;

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
        TurnaroundPhase::Loading,
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

    class HoldingRule final : public AircraftRule
    {
    public:
        static constexpr int kHoldTicksAllowed = 2;

        [[nodiscard]] const char* Name() const override
        {
            return "holding-rule";
        }

        [[nodiscard]] RuleVerdict Evaluate(const RuleContext&) override
        {
            return RuleVerdict::Hold(kHoldTicksAllowed, "held by the test rule");
        }

        void Act(const RuleContext&, VariableWriter&) override
        {
        }
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

    struct CheckpointWorld
    {
        std::size_t requestsSoFar = 0;
        std::size_t aircraftCallsSoFar = 0;
        bool departureLatchHeld = false;
        bool passengerLatchHeld = false;
    };

    class TurnaroundWorkflow
    {
    public:
        TurnaroundStateFixture f;
        TurnaroundStateMachine machine;
        std::vector<TurnaroundPhase> visitedPhases;
        std::vector<TurnaroundCheckpoint> checkpoints;
        std::vector<CheckpointWorld> worlds;

        TurnaroundWorkflow()
            : machine(&f.status, &f.settings, &f.gsxService, &f.menuGateway, &f.logger, &f.variableWriter)
        {
            visitedPhases.push_back(machine.GetPhase());
        }

        void Tick()
        {
            machine.Tick();

            if (const auto checkpoint = machine.TakeCheckpoint(); checkpoint.has_value())
            {
                checkpoints.push_back(*checkpoint);
                worlds.push_back({.requestsSoFar = f.menuGateway.requestLog.size(),
                                  .aircraftCallsSoFar = f.aircraft.callLog.size(),
                                  .departureLatchHeld = f.aircraft.doorsHeldClosed,
                                  .passengerLatchHeld = f.aircraft.passengerDoorsHeldClosed});
            }
        }

        void TickHolding(const TurnaroundPhase expected)
        {
            Tick();
            QCOMPARE(machine.GetPhase(), expected);
        }

        void TickTo(const TurnaroundPhase expected)
        {
            Tick();
            QCOMPARE(machine.GetPhase(), expected);
            RecordCurrentPhase();
        }

        void FinishDelay(const int ticks, const TurnaroundPhase expected)
        {
            for (int tick = 0; tick < ticks; ++tick)
            {
                Tick();
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

            TickTo(TurnaroundPhase::Loading);
        }

        void CompleteRefueling()
        {
            f.gsxService.refuelingState = GsxStateStatus::Completed;
            f.gsxService.hoseConnected = false;
            f.gsxService.boardingState = GsxStateStatus::Callable;

            TickHolding(TurnaroundPhase::Loading);
        }

        void StartBoarding()
        {
            f.gsxService.boardingState = GsxStateStatus::Active;
            f.gsxService.boardedPassengers = f.aircraft.plannedPax;
            f.gsxService.cargoPercent = 100.0;

            TickHolding(TurnaroundPhase::Loading);
        }

        void BeginBoardingDelay()
        {
            f.gsxService.boardingState = GsxStateStatus::Completed;

            TickHolding(TurnaroundPhase::Loading);
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

            TickHolding(TurnaroundPhase::WaitingForEngines);

            f.aircraft.smartSwitchActivated = false;
            f.gsxService.waitingForEngines = false;
            f.gsxService.pushbackFinished = true;

            TickTo(TurnaroundPhase::WaitingDeparture);

            f.gsxService.pushbackFinished = false;
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

#ifndef NDEBUG
    void SkipTo(TurnaroundWorkflow& workflow, const TurnaroundPhase target)
    {
        workflow.machine.DebugSkipPhase(static_cast<int>(target) - static_cast<int>(workflow.machine.GetPhase()));
    }
#endif

    bool Logged(const TurnaroundWorkflow& workflow, const std::string& fragment)
    {
        return std::ranges::any_of(workflow.f.logger.messages, [&fragment](const std::string& message)
        {
            return message.find(fragment) != std::string::npos;
        });
    }

    int LoggedCount(const TurnaroundWorkflow& workflow, const std::string& fragment)
    {
        return static_cast<int>(std::ranges::count_if(workflow.f.logger.messages, [&fragment](const std::string& message)
        {
            return message.find(fragment) != std::string::npos;
        }));
    }

    void WalkTheWholeTurnaround(TurnaroundWorkflow& workflow)
    {
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
    }

    std::vector<TurnaroundPhase> ResumablePhases()
    {
        const auto indices = std::views::iota(0, static_cast<int>(TurnaroundPhase::Count));
        std::vector<TurnaroundPhase> phases;

        for (const int index : indices)
        {
            if (IsResumablePhase(static_cast<TurnaroundPhase>(index)))
            {
                phases.push_back(static_cast<TurnaroundPhase>(index));
            }
        }

        return phases;
    }

    struct Stand
    {
        RefuelBy refuelBy = RefuelBy::Self;
        BoardBy boardBy = BoardBy::Self;
        bool jetway = false;
        bool pushbackViaInterruptMenu = false;
    };

    constexpr Stand kStairsStand{};
    constexpr Stand kJetwayClientStand{.refuelBy = RefuelBy::Client, .boardBy = BoardBy::Client, .jetway = true, .pushbackViaInterruptMenu = true};

    void EquipTheStand(TurnaroundWorkflow& workflow, const Stand& stand)
    {
        AutomationSettings& settings = workflow.f.settings;
        settings.callGpu = true;
        settings.callGpuOnArrival = true;
        settings.placeChocks = true;
        settings.placeChocksOnArrival = true;
        settings.callCatering = true;
        settings.callLavatory = true;
        settings.callWater = true;
        settings.callCleaning = true;

        if (stand.refuelBy == RefuelBy::Client)
        {
            settings.fuelRateKgs = kFastFuelRateKgs;
        }

        FakeAircraft& aircraft = workflow.f.aircraft;
        aircraft.supportsChocksControl = true;
        aircraft.completesPushbackViaInterruptMenu = stand.pushbackViaInterruptMenu;

        FakeGsxService& gsx = workflow.f.gsxService;
        gsx.jetwayAvailable = stand.jetway;
        gsx.stairsAvailable = !stand.jetway;
    }

    void PrepareTheStandFlightPlan(FakeAircraft& aircraft, const Stand& stand)
    {
        PrepareFlightPlan(aircraft);
        aircraft.refuelMethod = stand.refuelBy;
        aircraft.boardMethod = stand.boardBy;
    }

    void ReturnTheWorldToAvailable(TurnaroundWorkflow& workflow, const Stand& stand, const TurnaroundPhase phase)
    {
        EquipTheStand(workflow, stand);
        PrepareTheStandFlightPlan(workflow.f.aircraft, stand);
        workflow.f.aircraft.powered = true;
        workflow.f.aircraft.readyToDeboard = true;

        FakeGsxService& gsx = workflow.f.gsxService;
        gsx.simbriefLoaded = true;
        gsx.refuelingState = GsxStateStatus::Callable;
        gsx.boardingState = GsxStateStatus::Callable;
        gsx.departureState = GsxStateStatus::Callable;
        gsx.deboardingState = GsxStateStatus::Callable;

        if (phase == TurnaroundPhase::Loading)
        {
            gsx.refuelingState = GsxStateStatus::Active;
            gsx.hoseConnected = true;
        }
    }

    void TickThrough(TurnaroundWorkflow& workflow, const TurnaroundPhase expected)
    {
        for (int tick = 0; tick < kWalkTickLimit && workflow.machine.GetPhase() != expected; ++tick)
        {
            workflow.Tick();
        }

        QCOMPARE(workflow.machine.GetPhase(), expected);
    }

    void WalkTheStandToTheServices(TurnaroundWorkflow& workflow, const Stand& stand)
    {
        FakeGsxService& gsx = workflow.f.gsxService;

        workflow.AttachAircraft();
        workflow.TickHolding(TurnaroundPhase::RepositionAircraft);
        gsx.repositioning = true;
        workflow.TickHolding(TurnaroundPhase::RepositionAircraft);
        gsx.repositioning = false;
        workflow.TickTo(TurnaroundPhase::PlaceGroundEquipment);
        workflow.TickHolding(TurnaroundPhase::PlaceGroundEquipment);
        gsx.gpuStatus = GroundPowerStatus::Connected;
        workflow.TickTo(TurnaroundPhase::CallServices);
        workflow.TickHolding(TurnaroundPhase::CallServices);

        if (stand.jetway)
        {
            gsx.jetwayInPlace = true;
        }
        else
        {
            gsx.stairsAvailable = false;
            gsx.stairsInPlace = true;
        }

        TickThrough(workflow, TurnaroundPhase::WaitingFlightPlan);
        PrepareTheStandFlightPlan(workflow.f.aircraft, stand);
        gsx.simbriefLoaded = false;
        workflow.TickHolding(TurnaroundPhase::WaitingFlightPlan);
        gsx.simbriefLoaded = true;
        workflow.TickTo(TurnaroundPhase::WaitingPowerOn);
        workflow.f.aircraft.powered = true;
        workflow.TickTo(TurnaroundPhase::CallCatering);
        workflow.TickHolding(TurnaroundPhase::CallCatering);
        gsx.cateringInProgress = true;
        workflow.TickTo(TurnaroundPhase::RequestFuel);
    }

    void WalkTheStandToTheDeparture(TurnaroundWorkflow& workflow)
    {
        FakeGsxService& gsx = workflow.f.gsxService;

        workflow.RequestFuel();
        workflow.StartRefueling();
        workflow.CompleteRefueling();
        workflow.StartBoarding();
        workflow.CompleteBoarding();

        workflow.f.aircraft.readyToPush = true;
        workflow.f.aircraft.parkingBrakeSet = true;
        gsx.cateringInProgress = false;
        workflow.TickTo(TurnaroundPhase::WaitCatering);
        workflow.TickTo(TurnaroundPhase::RemoveGroundEquipment);
        workflow.TickHolding(TurnaroundPhase::RemoveGroundEquipment);
        gsx.gpuStatus = GroundPowerStatus::Disconnected;
        workflow.TickTo(TurnaroundPhase::RequestPushback);
        gsx.departureState = GsxStateStatus::Callable;
        workflow.TickHolding(TurnaroundPhase::RequestPushback);
        workflow.StartPushback();
        workflow.StartPushbackMovement();
        workflow.ConfirmEngineStart();
        workflow.Depart();
        workflow.Land();
    }

    void WalkTheStandThroughTheCabinServices(TurnaroundWorkflow& workflow)
    {
        FakeGsxService& gsx = workflow.f.gsxService;

        workflow.RequestDeboarding();
        workflow.StartDeboarding();

        gsx.deboardingState = GsxStateStatus::Completed;
        gsx.deboardedPassengers = workflow.f.aircraft.plannedPax;
        gsx.deboardingCargoPercent = 100.0;
        workflow.TickTo(TurnaroundPhase::CabinServices);
        workflow.TickHolding(TurnaroundPhase::CabinServices);
        gsx.lavatoryInProgress = true;
        workflow.TickHolding(TurnaroundPhase::CabinServices);
        workflow.TickHolding(TurnaroundPhase::CabinServices);
        gsx.waterInProgress = true;
        workflow.TickHolding(TurnaroundPhase::CabinServices);
        workflow.TickHolding(TurnaroundPhase::CabinServices);
        gsx.cleaningInProgress = true;
        workflow.TickHolding(TurnaroundPhase::CabinServices);
        gsx.lavatoryInProgress = false;
        gsx.waterInProgress = false;
        gsx.cleaningInProgress = false;
        workflow.TickHolding(TurnaroundPhase::CabinServices);
        workflow.FinishDelay(60, TurnaroundPhase::WaitingNewFlight);
    }

    void WalkTheStand(TurnaroundWorkflow& workflow, const Stand& stand)
    {
        EquipTheStand(workflow, stand);
        WalkTheStandToTheServices(workflow, stand);
        WalkTheStandToTheDeparture(workflow);
        WalkTheStandThroughTheCabinServices(workflow);
    }

    TurnaroundCheckpoint CheckpointAt(const TurnaroundPhase phase)
    {
        TurnaroundCheckpoint checkpoint;
        checkpoint.phase = phase;

        return checkpoint;
    }

    QString Joined(const std::vector<std::string>& log)
    {
        QStringList parts;
        for (const std::string& entry : log)
        {
            parts << QString::fromStdString(entry);
        }

        return parts.join(QStringLiteral(","));
    }

    template <typename Value>
    std::string Render(const Value& value)
    {
        if constexpr (std::is_enum_v<Value>)
        {
            return std::to_string(static_cast<int>(value));
        }
        else if constexpr (std::is_same_v<Value, std::optional<int>>)
        {
            return value.has_value() ? std::to_string(*value) : std::string("none");
        }
        else if constexpr (std::is_same_v<Value, std::set<std::string>>)
        {
            std::string text;
            for (const std::string& entry : value)
            {
                text += entry + ";";
            }

            return text;
        }
        else
        {
            return std::format("{}", value);
        }
    }

    struct RenderedField
    {
        std::string name;
        turnaround::FieldRestore restore = turnaround::FieldRestore::Raw;
        std::string text;
    };

    std::vector<RenderedField> RenderFields(const TurnaroundData& data)
    {
        std::vector<RenderedField> fields;
        turnaround::VisitFields([&fields, &data](const std::string_view name, const auto field, const turnaround::FieldRestore restore)
        {
            fields.push_back({std::string(name), restore, Render(field(data))});
        });

        return fields;
    }

    TurnaroundData SavedLoadingData()
    {
        TurnaroundData data;
        data.plannedFuelKg = 12000.0;
        data.plannedZfwKg = 180000.0;
        data.plannedPassengers = 210;
        data.initialFuelKg = 5000.0;
        data.initialZfwKg = 130000.0;

        return data;
    }

    FakeAircraft AircraftReading(const RefuelBy method, const double currentFuelKg)
    {
        FakeAircraft aircraft;
        aircraft.refuelMethod = method;
        aircraft.currentFuelKg = currentFuelKg;

        return aircraft;
    }

    TurnaroundData RestoredAt(const TurnaroundData& saved, const TurnaroundPhase phase, const Aircraft& aircraft)
    {
        return turnaround::RestoreTurnaroundData(saved, phase, aircraft, false);
    }

    void VerifyTheRestoreByClass(const TurnaroundData& saved, const TurnaroundData& restored)
    {
        const std::vector<RenderedField> before = RenderFields(saved);
        const std::vector<RenderedField> after = RenderFields(restored);
        const std::vector<RenderedField> defaults = RenderFields(TurnaroundData{});

        QCOMPARE(after.size(), before.size());

        for (std::size_t index = 0; index < before.size(); ++index)
        {
            const char* const name = before[index].name.c_str();

            switch (before[index].restore)
            {
            case turnaround::FieldRestore::Raw:
                QVERIFY2(after[index].text == before[index].text, name);
                break;
            case turnaround::FieldRestore::Restart:
            case turnaround::FieldRestore::NotSaved:
                QVERIFY2(after[index].text == defaults[index].text, name);
                break;
            case turnaround::FieldRestore::Rebuilt:
                break;
            }
        }
    }

    TurnaroundCheckpoint BoardingFinishedCheckpoint(const TurnaroundPhase phase)
    {
        TurnaroundCheckpoint checkpoint = CheckpointAt(phase);
        checkpoint.data.boardingFinished = true;

        return checkpoint;
    }

    ResumeOutcome Resume(TurnaroundWorkflow& workflow, const TurnaroundCheckpoint& checkpoint)
    {
        return workflow.machine.ResumeFrom(checkpoint, workflow.f.aircraft, false);
    }

    TurnaroundCheckpoint PushbackPendingCheckpoint()
    {
        TurnaroundCheckpoint checkpoint = CheckpointAt(TurnaroundPhase::WaitingPushbackToStart);
        checkpoint.data.pushbackRequested = true;
        checkpoint.data.pushbackPending = true;

        return checkpoint;
    }

    void TickHoldingInThePushbackWait(TurnaroundWorkflow& workflow)
    {
        for (int tick = 0; tick < kTicksPastTheInterruptionWarning; ++tick)
        {
            workflow.TickHolding(TurnaroundPhase::WaitingPushbackToStart);
        }
    }

    constexpr std::array kRequestKindNames{
        "CallJetway", "CallStairs", "Reposition", "SimbriefLoad", "Boarding", "Deboarding",
        "Pushback", "DepartureClearance", "Refueling", "ConfirmGoodEngines", "CompletePushback",
        "CompleteRefuel", "CompleteBoarding", "ToggleGpu", "Catering", "Lavatory", "Water", "Cleaning"};

    static_assert(kRequestKindNames.size() == static_cast<std::size_t>(RequestKind::Count));

    void VerifyTheWalkRequested(const TurnaroundWorkflow& walked, const std::vector<RequestKind>& expected)
    {
        for (const RequestKind kind : expected)
        {
            const std::string message = std::format("the walk never requested {}", kRequestKindNames[static_cast<std::size_t>(kind)]);
            QVERIFY2(std::ranges::count(walked.f.menuGateway.requestLog, kind) > 0, message.c_str());
        }
    }

    std::vector<std::string> GroundEffectsFrom(const std::vector<std::string>& calls, const std::size_t from)
    {
        constexpr std::array kGroundEffects{"CloseAllDoors", "ClearOwnGroundEquipment", "SetChocks(true)", "SetChocks(false)"};
        std::vector<std::string> effects;

        for (std::size_t index = from; index < calls.size(); ++index)
        {
            if (std::ranges::find(kGroundEffects, calls[index]) != kGroundEffects.end())
            {
                effects.push_back(calls[index]);
            }
        }

        return effects;
    }

    template <typename Entry>
    bool IsAPrefixOfWhatTheWalkDidNext(const std::vector<Entry>& walkAfterTheCheckpoint, const std::vector<Entry>& resumed)
    {
        return resumed.size() <= walkAfterTheCheckpoint.size()
            && std::equal(resumed.begin(), resumed.end(), walkAfterTheCheckpoint.begin());
    }

    std::vector<RequestKind> RequestsTheWalkMadeAfter(const TurnaroundWorkflow& walked, const CheckpointWorld& world)
    {
        const std::vector<RequestKind>& log = walked.f.menuGateway.requestLog;

        return {log.begin() + static_cast<std::ptrdiff_t>(world.requestsSoFar), log.end()};
    }

    void TickAfterTheResume(TurnaroundWorkflow& resumed, const TurnaroundPhase phase)
    {
        for (int tick = 0; tick < kTicksAfterTheResume && resumed.machine.GetPhase() == phase; ++tick)
        {
            resumed.machine.Tick();
        }
    }

    void VerifyEveryResumableCheckpointRepeatsNothing(const TurnaroundWorkflow& walked, const Stand& stand)
    {
        QVERIFY(!walked.f.aircraft.fuelWrites.empty());
        QVERIFY(!walked.f.aircraft.zfwWrites.empty());

        std::set<TurnaroundPhase> resumedPhases;
        std::size_t fuelWritesAfterResumes = 0;
        std::size_t zfwWritesAfterResumes = 0;

        for (std::size_t point = 0; point < walked.checkpoints.size(); ++point)
        {
            const TurnaroundCheckpoint& checkpoint = walked.checkpoints[point];
            const CheckpointWorld& world = walked.worlds[point];
            const std::string label = std::format("{} (checkpoint {})", TurnaroundPhaseToString(checkpoint.phase), point);

            TurnaroundWorkflow resumed;
            ReturnTheWorldToAvailable(resumed, stand, checkpoint.phase);

            QCOMPARE(Resume(resumed, checkpoint), ResumeOutcome::Resumed);
            QVERIFY2(resumed.machine.GetPhase() == checkpoint.phase, label.c_str());

            TickAfterTheResume(resumed, checkpoint.phase);

            QVERIFY2(IsAPrefixOfWhatTheWalkDidNext(RequestsTheWalkMadeAfter(walked, world), resumed.f.menuGateway.requestLog),
                     std::format("{}: the resume asked GSX for something the walk had already asked", label).c_str());

            QVERIFY2(IsAPrefixOfWhatTheWalkDidNext(GroundEffectsFrom(walked.f.aircraft.callLog, world.aircraftCallsSoFar),
                                                   GroundEffectsFrom(resumed.f.aircraft.callLog, 0)),
                     std::format("{}: the resume repeated a ground effect on the aircraft", label).c_str());

            QVERIFY2(std::ranges::all_of(resumed.f.aircraft.fuelWrites, [](const double kilograms) { return kilograms > 0.0; }), label.c_str());
            QVERIFY2(std::ranges::all_of(resumed.f.aircraft.zfwWrites, [](const double kilograms) { return kilograms > 0.0; }), label.c_str());
            fuelWritesAfterResumes += resumed.f.aircraft.fuelWrites.size();
            zfwWritesAfterResumes += resumed.f.aircraft.zfwWrites.size();
            resumedPhases.insert(checkpoint.phase);
        }

        QCOMPARE(resumedPhases.size(), kResumablePhaseCount);
        QVERIFY(fuelWritesAfterResumes > 0);
        QVERIFY(zfwWritesAfterResumes > 0);
    }

    struct TransitionSideEffects
    {
        int pushbackStarted = 0;
        int menuTurnaroundTurned = 0;
        int pushbackPanelClosed = 0;
        int gsxTurnaroundTurned = 0;
        int turnaroundStarted = 0;
        int transitionsLogged = 0;

        bool operator==(const TransitionSideEffects&) const = default;
    };

    TransitionSideEffects SideEffectsOf(const TurnaroundWorkflow& workflow)
    {
        return {.pushbackStarted = workflow.f.menuGateway.pushbackStartedCalls,
                .menuTurnaroundTurned = workflow.f.menuGateway.turnaroundTurnedCalls,
                .pushbackPanelClosed = workflow.f.menuGateway.closePushbackPanelCalls,
                .gsxTurnaroundTurned = workflow.f.gsxService.turnaroundTurnedCalls,
                .turnaroundStarted = workflow.f.aircraft.onTurnaroundStartedCalls,
                .transitionsLogged = LoggedCount(workflow, "Transitioning")};
    }

    void VerifyNoResumeFiresATransitionSideEffect(TurnaroundWorkflow& workflow)
    {
        const TransitionSideEffects before = SideEffectsOf(workflow);

        for (const TurnaroundPhase phase : ResumablePhases())
        {
            QCOMPARE(Resume(workflow, CheckpointAt(phase)), ResumeOutcome::Resumed);
            QVERIFY2(SideEffectsOf(workflow) == before, TurnaroundPhaseToString(phase));
        }
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
    static void theRefuelEndingAloneKeepsTheLoadingPhase();
    static void waitsWithTheWarningWhenTheCouatlDropsTheRefueling();
    static void aDelayedTransitionKeepsTheFastRulesRunning();
    static void theSlowTickActsOnlyWhenTheMachineIsDriving();
    static void aHoldThatExpiredInOnePhaseHoldsAgainInTheNextPhase();
    static void waitsForBoardingTransitionDelay();
    static void holdsBoardingWhileCargoIsPending();
    static void theLoaderCountdownRunsOutOnTheTickTheClientGivesUpOnTheDoor();
    static void thePilotEndingTheBoardingFromTheGsxMenuCompletesIt();
    static void aCompleteNowThatLeavesTheCargoFlagUpStillEndsTheBoarding();
    static void theCouatlDyingDuringTheBoardingHoldsTheFlowWithTheWarning();
    static void completesReachableWorkflowAndReturnsToStart();
    static void theNextTurnaroundOfTheSessionSkipsTheRepositionWhenTheNewTurnaroundOptionIsOn();
    static void theNextTurnaroundOfTheSessionRepositionsAgainWhenTheNewTurnaroundOptionIsOff();
    static void theRepositionMarkSurvivesTheEndOfTheTurnaround();
    static void aResetForgetsTheRepositionMark();
    static void aResumeKeepsTheRepositionMarkTheRuntimeDelivered();
    static void aResumeMidRepositionMarksTheSession();
    static void aResumeAfterTheRetryClearedTheRequestStillTakesTheRepositionAsAsked();
    static void aSavedPointWithoutTheAttemptFieldStillTakesTheRequestAsAsked();
    static void theRepositionMarkIsNotPartOfTheCheckpoint();
    static void theTurnaroundTurnNotifiesTheMenuGateway();
    static void theTurnaroundTurnForgetsTheGsxCompletions();
    static void theSecondTurnaroundAsksForBoardingAgain();
    static void theStartOfThePushMovementNotifiesTheMenuGateway();
    static void thePushbackFinishingBeforeTheMovementClosesThePushbackPanelOnce();
    static void theAircraftLeavingWithoutAPushbackClosesThePushbackPanelOnce();
    static void thePushMovementLeavesTheClosingToOnPushbackStarted();
    static void theDepartureWaitReturnsToTheEngineWaitWhileGsxStillAsksAndLeavesOnceItFinishes();
    static void aGsxRestartThatDropsThePushbackWarnsAndTheTaxiStillReachesTheArrival();
    static void publishesThatTheDeboardingWaitsForGsxUntilTheTurnaroundTurns();
    static void publishesCurrentTankFuelBeforeRefuel();
    static void publishesLoadingTargetsAfterFlightPlanCapture();
    static void publishesTheCrewThePlanLeftOutAfterFlightPlanCapture();
    static void debugSkipPhaseClampsToEnumRange();
    static void skippingPastTheFlightPlanKeepsThePlan();
    static void theBoardingBaselineAfterSkippingTheFlightPlanKeepsTheEmptyWeight();
    static void aSkipThatDoesNotCrossTheFlightPlanDoesNotCaptureAgain();
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
    static void theFuelStayAdvisoryClearsWhenThePilotDismissesIt();
    static void theVisitorNamesEveryLeafOnceWithAClass();
    static void aSavedPointRestoresRawFieldsAndRestartsTheCounters();
    static void theTickCountersRestartAndTheAttemptCountsReturnRaw();
    static void aFinishedRefuelRebuildsTheFuelProgress();
    static void aFinishedBoardingRebuildsTheBoardingProgress();
    static void aFinishedBoardingRebuildsTheBoardedPassengers();
    static void aFinishedBoardingRebuildsTheLoadedWeight();
    static void cabinServicesRebuildsTheDeboardingProgress();
    static void cabinServicesRebuildsTheEmptyWeightAsTheLoadedWeight();
    static void aSelfRefuelInProgressRebuildsTheLoadedFuelFromTheBaseline();
    static void aFinishedSelfOrClientRefuelRebuildsTheLoadedFuelFromThePlan();
    static void anyOtherRefuelRebuildsTheLoadedFuelFromTheTanks();
    static void theFactsCarryWhatTheSavedDataSays();
    static void aResumeAtTheHighTickCountDoesNotCloseTheDoorsOverAnUnknownService();
    static void aClientRefuelResumesFromWhatTheTanksHold();
    static void aResumedLoadingDoesNotNotifyTheLoadingStartAgain();
    static void aResumeDiscardsAPendingAppTouch();
    static void aRepositionAlreadyAskedIsNeverAskedAgainAfterAResume();
    static void aGsxRestartDuringTheCloseDropsThePendingPushbackAndWarns();
    static void aPushbackStillPendingOnTheSameGsxIsNotFlagged();
    static void thePushbackEdgeDetectorKeepsItsMemoryAcrossAResume();
    static void aLatestFlightPlanFetchOfTheDeadProcessIsAskedAgain();
    static void theCheckpointHoldsOnlyWhatIsSaved();
    static void theFactsFollowTheLatchesTheAircraftHolds();
    static void everyResumablePhaseResumesWithoutRepeatingARequest();
    static void everyResumablePhaseResumesWithoutRepeatingARequestWhenTheClientDrivesTheLoading();
    static void aResumeFiresNoneOfTheTransitionSideEffects();
    static void theDepartureLatchIsRearmedWhileBoardingIsFinishedAndTheDoorsAreNotReleased();
    static void theDoorLatchesAreRearmedInOrderAndNeverReleased();
    static void theArrivalDoorsClosingRearmsNeitherLatch();
    static void theWindowWithCargoStillLoadingRearmsOnlyThePassengerLatch();
    static void aResumeThatRearmsNoLatchLeavesBothAlone();
    static void theTurnaroundStartIsNotifiedOnTheEdgeIntoTheFirstWait();
    static void aResumeNotifiesTheResumeAndNeverTheStart();
    static void anUnknownPhaseRefusesTheResumeAndLeavesTheMachineAlone();
    static void anUnreachableAircraftRefusesTheResumeAndLeavesTheMachineAlone();
    static void theUnknownPhaseIsRefusedBeforeTheReachabilityIsAsked();
    static void onlyTheResumablePhasesGiveACheckpoint();
    static void theCheckpointCarriesTheMemoryOfTheAircraft();
    static void aResumeHandsTheAircraftTheMemoryItSaved();
    static void aCheckpointRoundTripsThroughAResume();
    static void resetForgetsTheVerdictsTheRulesAlreadyLogged();
    static void theMemoryBagRoundTripsAFlagANumberAndATextExactly();
    static void theMemoryBagAnswersTheFallbackForAMissingNameOrAWrongKind();
    static void theMemoryBagKeepsTheOrderAndReplacesInPlace();
    static void theMemoryBagBuiltFromEntriesEqualsTheOneThatWasFilled();
    static void thePlanRoundTripsThroughTheStatus();
    static void everyPhaseNameParsesBackAndAnUnknownNameDoesNot();
    static void theResumablePhasesRunFromRepositionToCabinServices();
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
    QCOMPARE(workflow.f.status.emptyZfwKg, 130000.0);
    QCOMPARE(workflow.f.status.targetPassengers, 210);
}

void TurnaroundStateMachineTest::publishesTheCrewThePlanLeftOutAfterFlightPlanCapture()
{
    TurnaroundWorkflow workflow;
    workflow.f.aircraft.crewOnBoardKg = 195.0;
    workflow.f.aircraft.plannedOperatingEmptyKg = 130000.0;
    workflow.AttachAircraft();
    workflow.CompleteReposition();
    workflow.CompleteGroundServiceSetup();

    QVERIFY(!workflow.f.status.planOmitsCrew);

    workflow.LoadFlightPlan();

    QVERIFY(workflow.f.status.planOmitsCrew);
    QCOMPARE(workflow.f.status.omittedCrewKg, 195.0);
    QCOMPARE(workflow.f.status.operatingEmptyWithCrewKg, 130195.0);
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

void TurnaroundStateMachineTest::theRefuelEndingAloneKeepsTheLoadingPhase()
{
    TurnaroundWorkflow workflow;
    ReachRefueling(workflow);

    workflow.CompleteRefueling();

    QCOMPARE(workflow.machine.GetDelayTicksRemaining(), 0);
    QCOMPARE(workflow.f.menuGateway.boardingCalls, 1);

    for (int tick = 0; tick < 60; ++tick)
    {
        workflow.TickHolding(TurnaroundPhase::Loading);
    }
}

void TurnaroundStateMachineTest::waitsForBoardingTransitionDelay()
{
    TurnaroundWorkflow workflow;
    ReachBoarding(workflow);

    workflow.BeginBoardingDelay();
    workflow.FinishDelay(59, TurnaroundPhase::Loading);
    workflow.TickTo(TurnaroundPhase::WaitingReadyToPush);
}

void TurnaroundStateMachineTest::holdsBoardingWhileCargoIsPending()
{
    TurnaroundWorkflow workflow;
    ReachBoarding(workflow);

    workflow.f.gsxService.loaderWaitingForDoor = CargoLoader::Rear;
    workflow.f.gsxService.cargoPercent = 67.0;
    workflow.f.gsxService.boardingState = GsxStateStatus::Completed;

    workflow.TickHolding(TurnaroundPhase::Loading);
    QCOMPARE(workflow.machine.GetDelayTicksRemaining(), 0);

    workflow.f.gsxService.loaderWaitingForDoor = CargoLoader::None;
    workflow.f.gsxService.loadingCargo = true;

    workflow.TickHolding(TurnaroundPhase::Loading);
    QCOMPARE(workflow.machine.GetDelayTicksRemaining(), 0);

    workflow.f.gsxService.loadingCargo = false;
    workflow.f.gsxService.cargoPercent = 100.0;

    workflow.CompleteBoarding();
}

void TurnaroundStateMachineTest::theLoaderCountdownRunsOutOnTheTickTheClientGivesUpOnTheDoor()
{
    TurnaroundWorkflow workflow;
    ReachBoarding(workflow);

    workflow.f.gsxService.loaderWaitingForDoor = CargoLoader::Front;
    workflow.TickHolding(TurnaroundPhase::Loading);

    QVERIFY(workflow.f.status.boardingProgress < 100.0);

    int secondsLeft = workflow.f.status.loaderDoorWaitSeconds;
    int ticks = 1;

    while (workflow.f.status.boardingProgress < 100.0 && ticks < 600)
    {
        workflow.TickHolding(TurnaroundPhase::Loading);
        ++ticks;

        QCOMPARE(workflow.f.status.loaderDoorWaitSeconds, secondsLeft - 1);

        secondsLeft = workflow.f.status.loaderDoorWaitSeconds;
    }

    QVERIFY(ticks < 600);
    QCOMPARE(secondsLeft, 0);
}

void TurnaroundStateMachineTest::thePilotEndingTheBoardingFromTheGsxMenuCompletesIt()
{
    TurnaroundWorkflow workflow;
    ReachBoarding(workflow);
    workflow.TickHolding(TurnaroundPhase::Loading);

    workflow.f.gsxService.boardingState = GsxStateStatus::Callable;
    workflow.TickHolding(TurnaroundPhase::Loading);

    QVERIFY(!workflow.f.status.serviceInterrupted);
    QVERIFY(!Logged(workflow, "GSX dropped the boarding it had already started"));

    workflow.FinishDelay(60, TurnaroundPhase::WaitingReadyToPush);

    QCOMPARE(workflow.f.status.boardingProgress, 100.0);
}

void TurnaroundStateMachineTest::aCompleteNowThatLeavesTheCargoFlagUpStillEndsTheBoarding()
{
    TurnaroundWorkflow workflow;
    ReachBoarding(workflow);

    workflow.f.aircraft.cargo = true;
    workflow.f.gsxService.cargoPercent = 0.0;
    workflow.f.gsxService.loadingCargo = true;
    workflow.TickHolding(TurnaroundPhase::Loading);

    workflow.f.gsxService.boardingState = GsxStateStatus::Callable;

    int ticks = 0;
    while (workflow.machine.GetDelayTicksRemaining() == 0 && ticks < 600)
    {
        workflow.TickHolding(TurnaroundPhase::Loading);
        ++ticks;
    }

    QCOMPARE(ticks, 120);
    QVERIFY(workflow.f.aircraft.doorsHeldClosed);
    QVERIFY(!workflow.f.status.serviceInterrupted);

    workflow.FinishDelay(60, TurnaroundPhase::WaitingReadyToPush);

    QCOMPARE(workflow.f.status.boardingProgress, 100.0);
}

void TurnaroundStateMachineTest::theCouatlDyingDuringTheBoardingHoldsTheFlowWithTheWarning()
{
    TurnaroundWorkflow workflow;
    ReachBoarding(workflow);
    workflow.TickHolding(TurnaroundPhase::Loading);

    workflow.f.gsxService.couatlAlive = false;
    workflow.f.gsxService.boardingState = GsxStateStatus::Callable;

    for (int tick = 0; tick < 61; ++tick)
    {
        workflow.TickHolding(TurnaroundPhase::Loading);
    }

    QVERIFY(workflow.f.status.serviceInterrupted);
    QVERIFY(Logged(workflow, "GSX dropped the boarding it had already started"));
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

void TurnaroundStateMachineTest::theTurnaroundTurnForgetsTheGsxCompletions()
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

    QCOMPARE(workflow.f.gsxService.turnaroundTurnedCalls, 0);
    QVERIFY(workflow.f.gsxService.WasStateCompleted(GsxState::Refueling));
    QVERIFY(workflow.f.gsxService.WasStateCompleted(GsxState::Boarding));

    workflow.CompleteDeboarding();

    QCOMPARE(workflow.f.gsxService.turnaroundTurnedCalls, 1);
    QVERIFY(!workflow.f.gsxService.WasStateCompleted(GsxState::Refueling));
    QVERIFY(!workflow.f.gsxService.WasStateCompleted(GsxState::Boarding));
}

void TurnaroundStateMachineTest::theSecondTurnaroundAsksForBoardingAgain()
{
    TurnaroundWorkflow workflow;
    workflow.f.settings.skipRepositionOnNewTurnaround = false;

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

    workflow.f.gsxService.refuelingState = GsxStateStatus::Callable;
    workflow.f.gsxService.boardingState = GsxStateStatus::Callable;
    workflow.f.gsxService.boardedPassengers = 0;
    workflow.f.gsxService.cargoPercent = 0.0;

    workflow.CompleteDeboarding();
    workflow.StartNewFlightCycle();

    QCOMPARE(workflow.f.menuGateway.boardingCalls, 1);

    workflow.f.aircraft.smartSwitchActivated = false;
    workflow.f.aircraft.powered = false;
    workflow.f.aircraft.readyToPush = false;
    workflow.f.aircraft.readyToDeboard = false;
    workflow.f.gsxService.stairsInPlace = false;

    workflow.AttachAircraft();
    workflow.CompleteReposition();
    workflow.CompleteGroundServiceSetup();
    workflow.LoadFlightPlan();
    workflow.RequestFuel();
    workflow.StartRefueling();

    for (int tick = 0; tick < 5; ++tick)
    {
        workflow.TickHolding(TurnaroundPhase::Loading);
    }

    QCOMPARE(workflow.f.menuGateway.boardingCalls, 1);
    QVERIFY(workflow.f.status.boardingProgress < 100.0);

    workflow.CompleteRefueling();

    QCOMPARE(workflow.f.menuGateway.boardingCalls, 2);

    workflow.StartBoarding();
    workflow.CompleteBoarding();
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

void TurnaroundStateMachineTest::thePushbackFinishingBeforeTheMovementClosesThePushbackPanelOnce()
{
    TurnaroundWorkflow workflow;

    ReachBoarding(workflow);
    workflow.CompleteBoarding();
    workflow.RequestPushback();
    workflow.StartPushback();

    workflow.TickHolding(TurnaroundPhase::WaitingPushbackToStart);

    QCOMPARE(workflow.f.menuGateway.closePushbackPanelCalls, 0);

    workflow.f.gsxService.pushbackFinished = true;
    workflow.TickTo(TurnaroundPhase::WaitingDeparture);

    QCOMPARE(workflow.f.menuGateway.closePushbackPanelCalls, 1);
    QCOMPARE(workflow.f.menuGateway.pushbackStartedCalls, 0);

    workflow.TickHolding(TurnaroundPhase::WaitingDeparture);

    QCOMPARE(workflow.f.menuGateway.closePushbackPanelCalls, 1);
}

void TurnaroundStateMachineTest::theAircraftLeavingWithoutAPushbackClosesThePushbackPanelOnce()
{
    TurnaroundWorkflow workflow;

    ReachBoarding(workflow);
    workflow.CompleteBoarding();
    workflow.RequestPushback();
    workflow.StartPushback();

    workflow.TickHolding(TurnaroundPhase::WaitingPushbackToStart);

    QCOMPARE(workflow.f.menuGateway.closePushbackPanelCalls, 0);

    workflow.f.aircraft.engineRunning = true;
    workflow.f.gsxService.groundSpeedKnots = kPushbackGroundSpeedKnots;
    workflow.TickTo(TurnaroundPhase::WaitingDeparture);

    QCOMPARE(workflow.f.menuGateway.closePushbackPanelCalls, 1);
    QCOMPARE(workflow.f.menuGateway.pushbackStartedCalls, 0);

    workflow.TickHolding(TurnaroundPhase::WaitingDeparture);

    QCOMPARE(workflow.f.menuGateway.closePushbackPanelCalls, 1);
}

void TurnaroundStateMachineTest::theDepartureWaitReturnsToTheEngineWaitWhileGsxStillAsksAndLeavesOnceItFinishes()
{
    TurnaroundWorkflow workflow;

    ReachBoarding(workflow);
    workflow.CompleteBoarding();
    workflow.RequestPushback();
    workflow.StartPushback();
    workflow.StartPushbackMovement();

    workflow.f.gsxService.goodEngineStartConfirmation = true;
    workflow.f.gsxService.pushbackFinished = true;
    workflow.TickTo(TurnaroundPhase::WaitingDeparture);

    workflow.f.gsxService.pushbackFinished = false;
    workflow.f.gsxService.waitingForEngines = true;
    workflow.TickTo(TurnaroundPhase::WaitingForEngines);

    workflow.f.gsxService.waitingForEngines = false;
    workflow.f.gsxService.pushbackFinished = true;
    workflow.TickTo(TurnaroundPhase::WaitingDeparture);

    workflow.f.gsxService.pushbackFinished = false;
    workflow.f.gsxService.waitingForEngines = true;
    workflow.TickHolding(TurnaroundPhase::WaitingDeparture);
}

void TurnaroundStateMachineTest::thePushMovementLeavesTheClosingToOnPushbackStarted()
{
    TurnaroundWorkflow workflow;

    ReachBoarding(workflow);
    workflow.CompleteBoarding();
    workflow.RequestPushback();
    workflow.StartPushback();
    workflow.StartPushbackMovement();

    QCOMPARE(workflow.f.menuGateway.pushbackStartedCalls, 1);
    QCOMPARE(workflow.f.menuGateway.closePushbackPanelCalls, 0);

    workflow.ConfirmEngineStart();

    QCOMPARE(workflow.f.menuGateway.closePushbackPanelCalls, 0);
}

void TurnaroundStateMachineTest::aGsxRestartThatDropsThePushbackWarnsAndTheTaxiStillReachesTheArrival()
{
    TurnaroundWorkflow workflow;

    ReachBoarding(workflow);
    workflow.CompleteBoarding();
    workflow.RequestPushback();
    workflow.StartPushback();

    workflow.f.gsxService.departureInProgress = true;
    workflow.TickHolding(TurnaroundPhase::WaitingPushbackToStart);

    workflow.f.gsxService.couatlRestartedBetweenTicks = true;
    workflow.TickHolding(TurnaroundPhase::WaitingPushbackToStart);

    workflow.f.gsxService.departureInProgress = false;

    for (int tick = 0; tick < 60; ++tick)
    {
        workflow.TickHolding(TurnaroundPhase::WaitingPushbackToStart);
    }

    QVERIFY(workflow.f.status.serviceInterrupted);
    QVERIFY(Logged(workflow, "GSX dropped the pushback it had already started"));
    QCOMPARE(workflow.f.menuGateway.pushbackCalls, 1);

    workflow.f.aircraft.engineRunning = true;
    workflow.f.gsxService.groundSpeedKnots = 12.0;
    workflow.TickTo(TurnaroundPhase::WaitingDeparture);

    QVERIFY(!workflow.f.status.serviceInterrupted);

    workflow.f.gsxService.groundSpeedKnots = 0.0;
    workflow.Depart();
    workflow.Land();

    QCOMPARE(workflow.machine.GetPhase(), TurnaroundPhase::PlaceArrivalGroundEquipment);
}

void TurnaroundStateMachineTest::publishesThatTheDeboardingWaitsForGsxUntilTheTurnaroundTurns()
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

    QVERIFY(!workflow.f.status.deboardingAwaitsGsx);

    workflow.RequestDeboarding();

    QVERIFY(workflow.f.status.deboardingAwaitsGsx);

    workflow.StartDeboarding();
    workflow.CompleteDeboarding();

    QVERIFY(!workflow.f.status.deboardingAwaitsGsx);
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

void TurnaroundStateMachineTest::theNextTurnaroundOfTheSessionSkipsTheRepositionWhenTheNewTurnaroundOptionIsOn()
{
    TurnaroundWorkflow workflow;
    workflow.f.settings.skipRepositionOnNewTurnaround = true;

    WalkTheWholeTurnaround(workflow);
    workflow.StartNewFlightCycle();

    QCOMPARE(workflow.f.menuGateway.repositionCalls, 1);

    for (int tick = 0; tick < 12; ++tick)
    {
        workflow.Tick();
    }

    QVERIFY(workflow.machine.GetPhase() > TurnaroundPhase::RepositionAircraft);
    QCOMPARE(workflow.f.menuGateway.repositionCalls, 1);
}

void TurnaroundStateMachineTest::theNextTurnaroundOfTheSessionRepositionsAgainWhenTheNewTurnaroundOptionIsOff()
{
    TurnaroundWorkflow workflow;
    workflow.f.settings.skipRepositionOnNewTurnaround = false;

    WalkTheWholeTurnaround(workflow);
    workflow.StartNewFlightCycle();

    for (int tick = 0; tick < 12; ++tick)
    {
        workflow.Tick();
    }

    QCOMPARE(workflow.machine.GetPhase(), TurnaroundPhase::RepositionAircraft);
    QCOMPARE(workflow.f.menuGateway.repositionCalls, 2);
}

void TurnaroundStateMachineTest::theRepositionMarkSurvivesTheEndOfTheTurnaround()
{
    TurnaroundWorkflow workflow;

    QVERIFY(!workflow.machine.HasRepositionedThisSession());

    WalkTheWholeTurnaround(workflow);

    QCOMPARE(workflow.machine.GetPhase(), TurnaroundPhase::WaitingNewFlight);
    QVERIFY(workflow.machine.HasRepositionedThisSession());
}

void TurnaroundStateMachineTest::aResetForgetsTheRepositionMark()
{
    TurnaroundWorkflow workflow;
    workflow.f.settings.skipRepositionOnNewTurnaround = true;

    WalkTheWholeTurnaround(workflow);

    QVERIFY(workflow.machine.HasRepositionedThisSession());

    workflow.machine.Reset();

    QVERIFY(!workflow.machine.HasRepositionedThisSession());

    workflow.AttachAircraft();
    workflow.Tick();

    QCOMPARE(workflow.machine.GetPhase(), TurnaroundPhase::RepositionAircraft);
    QCOMPARE(workflow.f.menuGateway.repositionCalls, 2);
}

void TurnaroundStateMachineTest::aResumeKeepsTheRepositionMarkTheRuntimeDelivered()
{
    TurnaroundWorkflow workflow;
    workflow.machine.NoteRepositionedThisSession();

    QCOMPARE(Resume(workflow, CheckpointAt(TurnaroundPhase::PlaceGroundEquipment)), ResumeOutcome::Resumed);

    QVERIFY(workflow.machine.HasRepositionedThisSession());
}

void TurnaroundStateMachineTest::aResumeMidRepositionMarksTheSession()
{
    TurnaroundWorkflow workflow;
    workflow.f.settings.skipRepositionOnNewTurnaround = false;
    TurnaroundCheckpoint checkpoint = CheckpointAt(TurnaroundPhase::RepositionAircraft);
    checkpoint.data.repositionRequested = true;
    checkpoint.data.repositionAttempted = true;

    QCOMPARE(Resume(workflow, checkpoint), ResumeOutcome::Resumed);
    QVERIFY(!workflow.machine.HasRepositionedThisSession());

    workflow.machine.Tick();

    QCOMPARE(workflow.machine.GetPhase(), TurnaroundPhase::PlaceGroundEquipment);
    QVERIFY(workflow.machine.HasRepositionedThisSession());
    QCOMPARE(workflow.f.menuGateway.repositionCalls, 0);
}

void TurnaroundStateMachineTest::aResumeAfterTheRetryClearedTheRequestStillTakesTheRepositionAsAsked()
{
    TurnaroundWorkflow workflow;
    workflow.f.settings.skipRepositionOnNewTurnaround = false;
    TurnaroundCheckpoint checkpoint = CheckpointAt(TurnaroundPhase::RepositionAircraft);
    checkpoint.data.repositionRequested = false;
    checkpoint.data.repositionAttempted = true;
    checkpoint.data.repositionCompleted = false;

    QCOMPARE(Resume(workflow, checkpoint), ResumeOutcome::Resumed);

    workflow.machine.Tick();

    QCOMPARE(workflow.machine.GetPhase(), TurnaroundPhase::PlaceGroundEquipment);
    QVERIFY(workflow.machine.HasRepositionedThisSession());
    QCOMPARE(workflow.f.menuGateway.repositionCalls, 0);
}

void TurnaroundStateMachineTest::aSavedPointWithoutTheAttemptFieldStillTakesTheRequestAsAsked()
{
    TurnaroundWorkflow workflow;
    workflow.f.settings.skipRepositionOnNewTurnaround = false;
    TurnaroundCheckpoint checkpoint = CheckpointAt(TurnaroundPhase::RepositionAircraft);
    checkpoint.data.repositionRequested = true;
    checkpoint.data.repositionAttempted = false;
    checkpoint.data.repositionCompleted = false;

    QCOMPARE(Resume(workflow, checkpoint), ResumeOutcome::Resumed);

    workflow.machine.Tick();

    QCOMPARE(workflow.machine.GetPhase(), TurnaroundPhase::PlaceGroundEquipment);
    QCOMPARE(workflow.f.menuGateway.repositionCalls, 0);
}

void TurnaroundStateMachineTest::theRepositionMarkIsNotPartOfTheCheckpoint()
{
    TurnaroundWorkflow workflow;
    workflow.machine.NoteRepositionedThisSession();

    QCOMPARE(Resume(workflow, CheckpointAt(TurnaroundPhase::PlaceGroundEquipment)), ResumeOutcome::Resumed);

    const std::optional<TurnaroundCheckpoint> checkpoint = workflow.machine.TakeCheckpoint();

    QVERIFY(checkpoint.has_value());
    QVERIFY(!checkpoint->data.repositionedThisSession);
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

void TurnaroundStateMachineTest::skippingPastTheFlightPlanKeepsThePlan()
{
#ifndef NDEBUG
    TurnaroundWorkflow workflow;
    workflow.AttachAircraft();
    PrepareFlightPlan(workflow.f.aircraft);

    SkipTo(workflow, TurnaroundPhase::RequestFuel);
    workflow.machine.Tick();

    QCOMPARE(workflow.f.status.targetZfwKg, 180000.0);
    QCOMPARE(workflow.f.status.targetFuelKg, 12000.0);
    QCOMPARE(workflow.f.status.targetPassengers, 210);
    QCOMPARE(workflow.f.status.emptyZfwKg, 130000.0);
#else
    QSKIP("DebugSkipPhase is compiled out of Release builds");
#endif
}

void TurnaroundStateMachineTest::theBoardingBaselineAfterSkippingTheFlightPlanKeepsTheEmptyWeight()
{
#ifndef NDEBUG
    TurnaroundWorkflow workflow;
    workflow.AttachAircraft();
    PrepareFlightPlan(workflow.f.aircraft);
    workflow.f.aircraft.powered = true;

    SkipTo(workflow, TurnaroundPhase::RequestFuel);
    workflow.RequestFuel();
    workflow.StartRefueling();
    workflow.CompleteRefueling();
    workflow.StartBoarding();

    QCOMPARE(workflow.f.status.emptyZfwKg, 130000.0);
    QCOMPARE(workflow.f.aircraft.currentZfwKg, 180000.0);
#else
    QSKIP("DebugSkipPhase is compiled out of Release builds");
#endif
}

void TurnaroundStateMachineTest::aSkipThatDoesNotCrossTheFlightPlanDoesNotCaptureAgain()
{
#ifndef NDEBUG
    TurnaroundWorkflow workflow;
    ReachRequestFuel(workflow);
    workflow.f.aircraft.plannedZfwKg = 200000.0;

    SkipTo(workflow, TurnaroundPhase::Loading);
    workflow.machine.Tick();

    QCOMPARE(workflow.f.status.targetZfwKg, 180000.0);
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

    workflow.TickHolding(TurnaroundPhase::Loading);

    workflow.f.aircraft.smartSwitchActivated = true;

    workflow.TickHolding(TurnaroundPhase::Loading);
    workflow.TickHolding(TurnaroundPhase::Loading);
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
    ReachBoarding(workflow);

    CountingRule rule;
    workflow.f.aircraft.rules = {&rule};

    workflow.BeginBoardingDelay();

    QCOMPARE(workflow.machine.GetDelayTicksRemaining(), 60);

    const int actsAtDelayStart = rule.actCalls;

    workflow.FinishDelay(59, TurnaroundPhase::Loading);

    QCOMPARE(rule.actCalls, actsAtDelayStart + 59);
}

void TurnaroundStateMachineTest::aHoldThatExpiredInOnePhaseHoldsAgainInTheNextPhase()
{
    TurnaroundWorkflow workflow;
    workflow.AttachAircraft();

    HoldingRule rule;
    workflow.f.aircraft.rules = {&rule};

    for (int tick = 0; tick < HoldingRule::kHoldTicksAllowed; ++tick)
    {
        workflow.TickHolding(TurnaroundPhase::RepositionAircraft);
    }

    workflow.TickHolding(TurnaroundPhase::RepositionAircraft);

    workflow.f.gsxService.repositioning = true;
    workflow.TickHolding(TurnaroundPhase::RepositionAircraft);

    workflow.f.gsxService.repositioning = false;
    workflow.TickTo(TurnaroundPhase::PlaceGroundEquipment);

    for (int tick = 0; tick < HoldingRule::kHoldTicksAllowed; ++tick)
    {
        workflow.TickHolding(TurnaroundPhase::PlaceGroundEquipment);
    }

    workflow.TickTo(TurnaroundPhase::CallServices);
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
    gateway.lvarSpans[kWatchedLVar] = LVarSpan{.min = 50.0, .max = 120.0, .received = true};
    gateway.MarkTick();
    workflow.machine.Tick();

    QCOMPARE(first.swings, std::vector<double>{70.0});
    QCOMPARE(second.swings, std::vector<double>{0.0});
}

void TurnaroundStateMachineTest::theFuelStayAdvisoryClearsWhenThePilotDismissesIt()
{
    TurnaroundWorkflow workflow;

    ReachRefueling(workflow);

    workflow.f.aircraft.refuelMethod = RefuelBy::Gsx;
    workflow.f.aircraft.fuelCapacityKg = 12000.0;
    workflow.f.aircraft.currentFuelKg = 11000.0;
    workflow.CompleteRefueling();

    QVERIFY(workflow.f.status.fuelDidNotStay);
    QCOMPARE(workflow.f.status.fuelShortfallKg, 1000.0);

    workflow.machine.DismissFuelStayAdvisory();
    workflow.machine.Tick();

    QVERIFY(!workflow.f.status.fuelDidNotStay);
}

void TurnaroundStateMachineTest::waitsWithTheWarningWhenTheCouatlDropsTheRefueling()
{
    TurnaroundWorkflow workflow;
    ReachRefueling(workflow);

    const int refuelRequests = workflow.f.menuGateway.refuelingCalls;

    for (int tick = 0; tick < 30; ++tick)
    {
        workflow.TickHolding(TurnaroundPhase::Loading);
    }

    workflow.f.gsxService.hoseConnected = false;

    for (int tick = 0; tick < 17; ++tick)
    {
        workflow.TickHolding(TurnaroundPhase::Loading);
    }

    QVERIFY(!workflow.f.status.serviceInterrupted);

    workflow.f.gsxService.couatlAlive = false;
    workflow.f.gsxService.refuelingState = GsxStateStatus::Callable;

    for (int tick = 0; tick < 120; ++tick)
    {
        workflow.TickHolding(TurnaroundPhase::Loading);
    }

    QVERIFY(workflow.f.status.serviceInterrupted);
    QVERIFY(Logged(workflow, "GSX dropped the refueling it had already started"));
    QCOMPARE(workflow.f.menuGateway.refuelingCalls, refuelRequests);

    workflow.f.gsxService.couatlAlive = true;
    workflow.f.gsxService.refuelingState = GsxStateStatus::Requested;
    workflow.TickHolding(TurnaroundPhase::Loading);

    QVERIFY(!workflow.f.status.serviceInterrupted);

    workflow.StartRefueling();
    workflow.CompleteRefueling();
}

void TurnaroundStateMachineTest::theVisitorNamesEveryLeafOnceWithAClass()
{
    const std::vector<RenderedField> fields = RenderFields(TurnaroundData{});

    std::set<std::string> names;
    for (const RenderedField& field : fields)
    {
        names.insert(field.name);
    }

    QCOMPARE(names.size(), fields.size());
    QVERIFY(names.contains("lavatory.asked"));
    QVERIFY(names.contains("cleaning.activeSeen"));
    QVERIFY(names.contains("expiredRuleHolds"));

    for (const turnaround::FieldRestore restore : {turnaround::FieldRestore::Raw, turnaround::FieldRestore::Restart,
                                                   turnaround::FieldRestore::Rebuilt, turnaround::FieldRestore::NotSaved})
    {
        QVERIFY(std::ranges::any_of(fields, [restore](const RenderedField& field) { return field.restore == restore; }));
    }

    const TurnaroundData filled = EveryFieldNonDefault();
    const CabinServiceProgress everyStepDone{.asked = true, .requested = true, .activeSeen = true};

    QVERIFY(filled.lavatory == everyStepDone);
    QVERIFY(filled.water == everyStepDone);
    QVERIFY(filled.cleaning == everyStepDone);
}

void TurnaroundStateMachineTest::aSavedPointRestoresRawFieldsAndRestartsTheCounters()
{
    const TurnaroundData saved = EveryFieldNonDefault();
    const FakeAircraft aircraft;

    VerifyTheRestoreByClass(saved, RestoredAt(saved, TurnaroundPhase::Loading, aircraft));
}

void TurnaroundStateMachineTest::theTickCountersRestartAndTheAttemptCountsReturnRaw()
{
    TurnaroundData saved;
    saved.stairsInPlaceTicks = 4;
    saved.servicesOperatingTicks = 5;
    saved.refuelStallTicks = 6;
    saved.refuelStallSampleKg = 100.0;
    saved.boardingStallTicks = 7;
    saved.loaderDoorWaitTicks = 8;
    saved.cargoFlagAfterServiceTicks = 9;
    saved.loaderAtHoldAfterServiceTicks = 10;
    saved.fuelRequestStallTicks = 11;
    saved.flightPlanRequestTicks = 12;
    saved.differingFlightPlanTicks = 13;
    saved.stateTickCount = 300;
    saved.ruleHoldTicks = 5;
    saved.expiredRuleHolds = {"a-rule"};
    saved.cateringWaitIntervals = 2;
    saved.cabinWaitIntervals = 3;
    saved.boardingCompletionAttempts = 4;

    const FakeAircraft aircraft;
    const TurnaroundData restored = RestoredAt(saved, TurnaroundPhase::Loading, aircraft);

    QCOMPARE(restored.stairsInPlaceTicks, 0);
    QCOMPARE(restored.servicesOperatingTicks, 0);
    QCOMPARE(restored.refuelStallTicks, 0);
    QCOMPARE(restored.refuelStallSampleKg, 0.0);
    QCOMPARE(restored.boardingStallTicks, 0);
    QCOMPARE(restored.loaderDoorWaitTicks, 0);
    QCOMPARE(restored.cargoFlagAfterServiceTicks, 0);
    QCOMPARE(restored.loaderAtHoldAfterServiceTicks, 0);
    QCOMPARE(restored.fuelRequestStallTicks, 0);
    QCOMPARE(restored.flightPlanRequestTicks, 0);
    QCOMPARE(restored.differingFlightPlanTicks, 0);
    QCOMPARE(restored.stateTickCount, 0);
    QCOMPARE(restored.ruleHoldTicks, 0);
    QVERIFY(restored.expiredRuleHolds.empty());
    QCOMPARE(restored.cateringWaitIntervals, 2);
    QCOMPARE(restored.cabinWaitIntervals, 3);
    QCOMPARE(restored.boardingCompletionAttempts, 4);
}

void TurnaroundStateMachineTest::aFinishedRefuelRebuildsTheFuelProgress()
{
    TurnaroundData saved = SavedLoadingData();
    saved.fuelProgress = 50.0;
    const FakeAircraft aircraft;

    QCOMPARE(RestoredAt(saved, TurnaroundPhase::Loading, aircraft).fuelProgress, 0.0);

    saved.refuelFinished = true;

    QCOMPARE(RestoredAt(saved, TurnaroundPhase::Loading, aircraft).fuelProgress, 100.0);
}

void TurnaroundStateMachineTest::aFinishedBoardingRebuildsTheBoardingProgress()
{
    TurnaroundData saved = SavedLoadingData();
    const FakeAircraft aircraft;

    QCOMPARE(RestoredAt(saved, TurnaroundPhase::Loading, aircraft).boardingProgress, 0.0);

    saved.boardingFinished = true;

    QCOMPARE(RestoredAt(saved, TurnaroundPhase::Loading, aircraft).boardingProgress, 100.0);
}

void TurnaroundStateMachineTest::aFinishedBoardingRebuildsTheBoardedPassengers()
{
    TurnaroundData saved = SavedLoadingData();
    saved.boardedPassengers = 90;
    const FakeAircraft aircraft;

    QCOMPARE(RestoredAt(saved, TurnaroundPhase::Loading, aircraft).boardedPassengers, 0);

    saved.boardingFinished = true;

    QCOMPARE(RestoredAt(saved, TurnaroundPhase::Loading, aircraft).boardedPassengers, 210);
}

void TurnaroundStateMachineTest::aFinishedBoardingRebuildsTheLoadedWeight()
{
    TurnaroundData saved = SavedLoadingData();
    saved.loadedZfwKg = 150000.0;
    const FakeAircraft aircraft;

    QCOMPARE(RestoredAt(saved, TurnaroundPhase::Loading, aircraft).loadedZfwKg, 0.0);

    saved.boardingFinished = true;

    QCOMPARE(RestoredAt(saved, TurnaroundPhase::WaitingReadyToPush, aircraft).loadedZfwKg, 180000.0);
}

void TurnaroundStateMachineTest::cabinServicesRebuildsTheDeboardingProgress()
{
    TurnaroundData saved = SavedLoadingData();
    saved.deboardingProgress = 60.0;
    const FakeAircraft aircraft;

    QCOMPARE(RestoredAt(saved, TurnaroundPhase::Deboarding, aircraft).deboardingProgress, 0.0);
    QCOMPARE(RestoredAt(saved, TurnaroundPhase::CabinServices, aircraft).deboardingProgress, 100.0);
}

void TurnaroundStateMachineTest::cabinServicesRebuildsTheEmptyWeightAsTheLoadedWeight()
{
    TurnaroundData saved = SavedLoadingData();
    saved.boardingFinished = true;
    const FakeAircraft aircraft;

    QCOMPARE(RestoredAt(saved, TurnaroundPhase::Deboarding, aircraft).loadedZfwKg, 180000.0);
    QCOMPARE(RestoredAt(saved, TurnaroundPhase::CabinServices, aircraft).loadedZfwKg, 130000.0);
}

void TurnaroundStateMachineTest::aSelfRefuelInProgressRebuildsTheLoadedFuelFromTheBaseline()
{
    TurnaroundData saved = SavedLoadingData();
    saved.refuelBaselined = true;
    saved.loadedFuelKg = 9999.0;
    const FakeAircraft aircraft = AircraftReading(RefuelBy::Self, 7300.0);

    QCOMPARE(RestoredAt(saved, TurnaroundPhase::Loading, aircraft).loadedFuelKg, 5000.0);

    saved.refuelBaselined = false;

    QCOMPARE(RestoredAt(saved, TurnaroundPhase::Loading, aircraft).loadedFuelKg, 7300.0);
}

void TurnaroundStateMachineTest::aFinishedSelfOrClientRefuelRebuildsTheLoadedFuelFromThePlan()
{
    TurnaroundData saved = SavedLoadingData();
    saved.refuelBaselined = true;
    saved.refuelFinished = true;

    const FakeAircraft self = AircraftReading(RefuelBy::Self, 7300.0);
    const FakeAircraft client = AircraftReading(RefuelBy::Client, 7300.0);

    QCOMPARE(RestoredAt(saved, TurnaroundPhase::Loading, self).loadedFuelKg, 12000.0);
    QCOMPARE(RestoredAt(saved, TurnaroundPhase::Loading, client).loadedFuelKg, 12000.0);
}

void TurnaroundStateMachineTest::anyOtherRefuelRebuildsTheLoadedFuelFromTheTanks()
{
    TurnaroundData saved = SavedLoadingData();
    saved.refuelBaselined = true;

    const FakeAircraft client = AircraftReading(RefuelBy::Client, 7300.0);
    const FakeAircraft gsx = AircraftReading(RefuelBy::Gsx, 7300.0);

    QCOMPARE(RestoredAt(saved, TurnaroundPhase::Loading, client).loadedFuelKg, 7300.0);
    QCOMPARE(RestoredAt(saved, TurnaroundPhase::Loading, gsx).loadedFuelKg, 7300.0);

    saved.fuelTopUpStarted = true;

    QCOMPARE(RestoredAt(saved, TurnaroundPhase::Loading, gsx).loadedFuelKg, 7300.0);

    saved.refuelFinished = true;

    QCOMPARE(RestoredAt(saved, TurnaroundPhase::Loading, gsx).loadedFuelKg, 7300.0);
}

void TurnaroundStateMachineTest::theFactsCarryWhatTheSavedDataSays()
{
    TurnaroundData data = SavedLoadingData();
    data.loadingStartNotified = true;
    data.refuelFinished = true;
    data.boardingFinished = true;
    data.passengerDoorsHeldClosed = true;

    const TurnaroundFacts facts = turnaround::BuildTurnaroundFacts(TurnaroundPhase::WaitingReadyToPush, data, true);

    QCOMPARE(facts.phase, TurnaroundPhase::WaitingReadyToPush);
    QVERIFY(facts.loadingStarted);
    QVERIFY(facts.refuelFinished);
    QVERIFY(facts.boardingFinished);
    QVERIFY(facts.departureDoorsHeld);
    QVERIFY(facts.passengerDoorsHeld);
    QVERIFY(!facts.arrivalDoorsClosed);
    QVERIFY(facts.gsxRestartedSinceSave);
    QCOMPARE(facts.plannedFuelKg, 12000.0);
    QCOMPARE(facts.plannedZfwKg, 180000.0);
    QCOMPARE(facts.emptyZfwKg, 130000.0);
    QCOMPARE(facts.plannedPassengers, 210);

    data.arrivalDoorsClosed = true;
    const TurnaroundFacts afterArrival = turnaround::BuildTurnaroundFacts(TurnaroundPhase::PlaceArrivalGroundEquipment, data, false);

    QVERIFY(afterArrival.arrivalDoorsClosed);
    QVERIFY(!afterArrival.departureDoorsHeld);
    QVERIFY(!afterArrival.passengerDoorsHeld);
    QVERIFY(!afterArrival.gsxRestartedSinceSave);
}

void TurnaroundStateMachineTest::aResumeAtTheHighTickCountDoesNotCloseTheDoorsOverAnUnknownService()
{
    TurnaroundWorkflow workflow;
    workflow.f.gsxService.serviceUnderway = std::nullopt;

    TurnaroundCheckpoint checkpoint = CheckpointAt(TurnaroundPhase::PlaceGroundEquipment);
    checkpoint.data.ownGroundEquipmentCleared = true;
    checkpoint.data.stateTickCount = 300;

    QCOMPARE(Resume(workflow, checkpoint), ResumeOutcome::Resumed);

    workflow.machine.Tick();

    QCOMPARE(workflow.f.aircraft.closeAllDoorsCalls, 0);
    QCOMPARE(workflow.machine.GetPhase(), TurnaroundPhase::PlaceGroundEquipment);
}

void TurnaroundStateMachineTest::aClientRefuelResumesFromWhatTheTanksHold()
{
    TurnaroundWorkflow workflow;
    workflow.f.aircraft.refuelMethod = RefuelBy::Client;
    workflow.f.aircraft.currentFuelKg = 7300.0;
    workflow.f.gsxService.refuelingState = GsxStateStatus::Active;
    workflow.f.gsxService.hoseConnected = true;

    TurnaroundCheckpoint checkpoint = CheckpointAt(TurnaroundPhase::Loading);
    checkpoint.data = SavedLoadingData();
    checkpoint.data.loadingStartNotified = true;
    checkpoint.data.refuelingRequested = true;
    checkpoint.data.refuelBaselined = true;
    checkpoint.data.loadedFuelKg = 5000.0;

    QCOMPARE(Resume(workflow, checkpoint), ResumeOutcome::Resumed);

    workflow.machine.Tick();

    const double stepKg = workflow.f.settings.EffectiveFuelRateKgs() * turnaround::kTickSeconds;
    QVERIFY(!workflow.f.aircraft.fuelWrites.empty());
    QCOMPARE(workflow.f.aircraft.fuelWrites.front(), 7300.0 + stepKg);
}

void TurnaroundStateMachineTest::aResumedLoadingDoesNotNotifyTheLoadingStartAgain()
{
    TurnaroundWorkflow workflow;
    workflow.f.aircraft.refuelMethod = RefuelBy::Self;
    workflow.f.aircraft.currentFuelKg = 7300.0;
    workflow.f.gsxService.refuelingState = GsxStateStatus::Active;
    workflow.f.gsxService.hoseConnected = true;

    TurnaroundCheckpoint checkpoint = CheckpointAt(TurnaroundPhase::Loading);
    checkpoint.data = SavedLoadingData();
    checkpoint.data.loadingStartNotified = true;
    checkpoint.data.refuelBaselined = true;

    QCOMPARE(Resume(workflow, checkpoint), ResumeOutcome::Resumed);

    workflow.machine.Tick();
    workflow.machine.Tick();

    QCOMPARE(workflow.f.aircraft.onLoadingStartedCalls, 0);
    QCOMPARE(workflow.f.aircraft.setCurrentFuelCalls, 0);
}

void TurnaroundStateMachineTest::everyResumablePhaseResumesWithoutRepeatingARequest()
{
    TurnaroundWorkflow walked;
    WalkTheStand(walked, kStairsStand);

    VerifyTheWalkRequested(walked, {RequestKind::CallStairs, RequestKind::Reposition, RequestKind::SimbriefLoad,
                                    RequestKind::Boarding, RequestKind::Deboarding, RequestKind::Pushback,
                                    RequestKind::Refueling, RequestKind::ConfirmGoodEngines, RequestKind::ToggleGpu,
                                    RequestKind::Catering, RequestKind::Lavatory, RequestKind::Water,
                                    RequestKind::Cleaning});
    QVERIFY(walked.f.aircraft.setChocksCalls > 0);
    QVERIFY(walked.f.aircraft.closeAllDoorsCalls > 0);
    QVERIFY(walked.f.aircraft.clearOwnGroundEquipmentCalls > 0);

    VerifyEveryResumableCheckpointRepeatsNothing(walked, kStairsStand);
}

void TurnaroundStateMachineTest::everyResumablePhaseResumesWithoutRepeatingARequestWhenTheClientDrivesTheLoading()
{
    TurnaroundWorkflow walked;
    WalkTheStand(walked, kJetwayClientStand);

    VerifyTheWalkRequested(walked, {RequestKind::CallJetway, RequestKind::CompletePushback, RequestKind::Refueling,
                                    RequestKind::Boarding, RequestKind::Deboarding, RequestKind::ToggleGpu});

    VerifyEveryResumableCheckpointRepeatsNothing(walked, kJetwayClientStand);
}

void TurnaroundStateMachineTest::aResumeDiscardsAPendingAppTouch()
{
    TurnaroundWorkflow workflow;
    workflow.f.settings.autoStartLoading = false;

    workflow.machine.AcceptAppTouch();

    QCOMPARE(Resume(workflow, CheckpointAt(TurnaroundPhase::RequestFuel)), ResumeOutcome::Resumed);

    workflow.machine.Tick();

    QVERIFY(!workflow.machine.IsLoadingConfirmed());
    QVERIFY(!Logged(workflow, "Pilot touch"));
}

void TurnaroundStateMachineTest::aRepositionAlreadyAskedIsNeverAskedAgainAfterAResume()
{
    TurnaroundWorkflow workflow;
    TurnaroundCheckpoint checkpoint = CheckpointAt(TurnaroundPhase::RepositionAircraft);
    checkpoint.data.repositionRequested = true;

    QCOMPARE(Resume(workflow, checkpoint), ResumeOutcome::Resumed);

    workflow.machine.Tick();

    QCOMPARE(workflow.machine.GetPhase(), TurnaroundPhase::PlaceGroundEquipment);

    for (int tick = 0; tick < kTicksPastTheRepositionRetry; ++tick)
    {
        workflow.machine.Tick();
    }

    QCOMPARE(workflow.f.menuGateway.repositionCalls, 0);
}

void TurnaroundStateMachineTest::aGsxRestartDuringTheCloseDropsThePendingPushbackAndWarns()
{
    TurnaroundWorkflow workflow;

    QCOMPARE(workflow.machine.ResumeFrom(PushbackPendingCheckpoint(), workflow.f.aircraft, true), ResumeOutcome::Resumed);

    TickHoldingInThePushbackWait(workflow);

    QVERIFY(workflow.f.status.serviceInterrupted);
    QVERIFY(Logged(workflow, "GSX dropped the pushback it had already started"));
    QVERIFY(workflow.machine.TakeCheckpoint()->data.pushbackLostToGsxRestart);
    QCOMPARE(workflow.f.menuGateway.pushbackCalls, 0);
}

void TurnaroundStateMachineTest::aPushbackStillPendingOnTheSameGsxIsNotFlagged()
{
    TurnaroundWorkflow workflow;
    workflow.f.gsxService.departureInProgress = true;

    QCOMPARE(Resume(workflow, PushbackPendingCheckpoint()), ResumeOutcome::Resumed);

    TickHoldingInThePushbackWait(workflow);

    QVERIFY(!workflow.f.status.serviceInterrupted);
    QVERIFY(!workflow.machine.TakeCheckpoint()->data.pushbackLostToGsxRestart);
}

void TurnaroundStateMachineTest::thePushbackEdgeDetectorKeepsItsMemoryAcrossAResume()
{
    TurnaroundWorkflow workflow;

    QCOMPARE(Resume(workflow, PushbackPendingCheckpoint()), ResumeOutcome::Resumed);

    workflow.f.gsxService.couatlRestartedBetweenTicks = true;
    TickHoldingInThePushbackWait(workflow);

    QVERIFY(workflow.f.status.serviceInterrupted);
    QVERIFY(Logged(workflow, "GSX dropped the pushback it had already started"));
}

void TurnaroundStateMachineTest::aLatestFlightPlanFetchOfTheDeadProcessIsAskedAgain()
{
    TurnaroundWorkflow workflow;
    workflow.machine.AttachFlightPlanSource(&workflow.f.flightPlanSource);
    PrepareFlightPlan(workflow.f.aircraft);
    workflow.f.gsxService.simbriefLoaded = true;

    TurnaroundCheckpoint checkpoint = CheckpointAt(TurnaroundPhase::WaitingFlightPlan);
    checkpoint.data.flightPlanRefused = true;
    checkpoint.data.latestFlightPlanRequested = true;

    QCOMPARE(Resume(workflow, checkpoint), ResumeOutcome::Resumed);

    workflow.machine.Tick();

    QCOMPARE(workflow.f.flightPlanSource.latestRequests, 1);
}

void TurnaroundStateMachineTest::theCheckpointHoldsOnlyWhatIsSaved()
{
    TurnaroundWorkflow workflow;
    ReachRequestFuel(workflow);

    QCOMPARE(workflow.machine.GetPhase(), TurnaroundPhase::RequestFuel);

    workflow.machine.Tick();
    const std::optional<TurnaroundCheckpoint> first = workflow.machine.TakeCheckpoint();
    workflow.machine.Tick();
    const std::optional<TurnaroundCheckpoint> second = workflow.machine.TakeCheckpoint();

    QVERIFY(first.has_value());
    QVERIFY(second.has_value());
    QVERIFY(*first == *second);
    QCOMPARE(second->data.stateTickCount, 0);

    workflow.f.gsxService.refuelingState = GsxStateStatus::Callable;
    workflow.machine.Tick();
    const std::optional<TurnaroundCheckpoint> afterTheRequest = workflow.machine.TakeCheckpoint();

    QVERIFY(afterTheRequest.has_value());
    QVERIFY(afterTheRequest->data.refuelingRequested);
    QVERIFY(!(*afterTheRequest == *second));
}

void TurnaroundStateMachineTest::theFactsFollowTheLatchesTheAircraftHolds()
{
    TurnaroundWorkflow walked;
    WalkTheWholeTurnaround(walked);

    bool aLatchWasHeld = false;

    for (std::size_t point = 0; point < walked.checkpoints.size(); ++point)
    {
        const TurnaroundCheckpoint& checkpoint = walked.checkpoints[point];
        const CheckpointWorld& world = walked.worlds[point];
        const TurnaroundFacts facts = turnaround::BuildTurnaroundFacts(checkpoint.phase, checkpoint.data, false);
        const std::string label = std::format("{} (checkpoint {})", TurnaroundPhaseToString(checkpoint.phase), point);

        QVERIFY2(facts.departureDoorsHeld == world.departureLatchHeld, label.c_str());
        QVERIFY2(facts.passengerDoorsHeld == world.passengerLatchHeld, label.c_str());
        aLatchWasHeld = aLatchWasHeld || world.departureLatchHeld || world.passengerLatchHeld;
    }

    QVERIFY(aLatchWasHeld);
    QVERIFY(!walked.f.aircraft.doorsHeldClosed);
    QVERIFY(!walked.f.aircraft.passengerDoorsHeldClosed);
}

void TurnaroundStateMachineTest::aResumeFiresNoneOfTheTransitionSideEffects()
{
    TurnaroundWorkflow parkedOnThePushback;
    ReachBoarding(parkedOnThePushback);
    parkedOnThePushback.CompleteBoarding();
    parkedOnThePushback.RequestPushback();
    parkedOnThePushback.StartPushback();

    QCOMPARE(parkedOnThePushback.machine.GetPhase(), TurnaroundPhase::WaitingPushbackToStart);
    QVERIFY(parkedOnThePushback.f.aircraft.onTurnaroundStartedCalls > 0);

    TurnaroundWorkflow finished;
    WalkTheWholeTurnaround(finished);

    QCOMPARE(finished.machine.GetPhase(), TurnaroundPhase::WaitingNewFlight);
    QVERIFY(finished.f.menuGateway.turnaroundTurnedCalls > 0);
    QVERIFY(finished.f.menuGateway.pushbackStartedCalls > 0);

    VerifyNoResumeFiresATransitionSideEffect(parkedOnThePushback);
    VerifyNoResumeFiresATransitionSideEffect(finished);
}

void TurnaroundStateMachineTest::theDepartureLatchIsRearmedWhileBoardingIsFinishedAndTheDoorsAreNotReleased()
{
    TurnaroundWorkflow workflow;

    QCOMPARE(Resume(workflow, BoardingFinishedCheckpoint(TurnaroundPhase::WaitingReadyToPush)), ResumeOutcome::Resumed);

    QCOMPARE(Joined(workflow.f.aircraft.callLog), QStringLiteral("OnTurnaroundResumed,HoldDoorsClosed(true)"));
    QCOMPARE(workflow.f.aircraft.holdPassengerDoorsClosedCalls, 0);
}

void TurnaroundStateMachineTest::theDoorLatchesAreRearmedInOrderAndNeverReleased()
{
    TurnaroundWorkflow workflow;
    TurnaroundCheckpoint checkpoint = BoardingFinishedCheckpoint(TurnaroundPhase::WaitingReadyToPush);
    checkpoint.data.passengerDoorsHeldClosed = true;

    QCOMPARE(Resume(workflow, checkpoint), ResumeOutcome::Resumed);

    QCOMPARE(Joined(workflow.f.aircraft.callLog),
             QStringLiteral("OnTurnaroundResumed,HoldDoorsClosed(true),HoldPassengerDoorsClosed(true)"));
}

void TurnaroundStateMachineTest::theArrivalDoorsClosingRearmsNeitherLatch()
{
    TurnaroundWorkflow workflow;
    TurnaroundCheckpoint checkpoint = BoardingFinishedCheckpoint(TurnaroundPhase::Deboarding);
    checkpoint.data.passengerDoorsHeldClosed = true;
    checkpoint.data.arrivalDoorsClosed = true;

    QCOMPARE(Resume(workflow, checkpoint), ResumeOutcome::Resumed);

    QCOMPARE(Joined(workflow.f.aircraft.callLog), QStringLiteral("OnTurnaroundResumed"));
}

void TurnaroundStateMachineTest::theWindowWithCargoStillLoadingRearmsOnlyThePassengerLatch()
{
    TurnaroundWorkflow workflow;
    TurnaroundCheckpoint checkpoint = CheckpointAt(TurnaroundPhase::Loading);
    checkpoint.data.passengerDoorsHeldClosed = true;

    QCOMPARE(Resume(workflow, checkpoint), ResumeOutcome::Resumed);

    QCOMPARE(Joined(workflow.f.aircraft.callLog), QStringLiteral("OnTurnaroundResumed,HoldPassengerDoorsClosed(true)"));
    QCOMPARE(workflow.f.aircraft.holdDoorsClosedCalls, 0);
}

void TurnaroundStateMachineTest::aResumeThatRearmsNoLatchLeavesBothAlone()
{
    TurnaroundWorkflow workflow;

    QCOMPARE(Resume(workflow, CheckpointAt(TurnaroundPhase::CallServices)), ResumeOutcome::Resumed);

    QCOMPARE(Joined(workflow.f.aircraft.callLog), QStringLiteral("OnTurnaroundResumed"));
    QCOMPARE(workflow.f.aircraft.holdDoorsClosedCalls, 0);
    QCOMPARE(workflow.f.aircraft.holdPassengerDoorsClosedCalls, 0);
}

void TurnaroundStateMachineTest::theTurnaroundStartIsNotifiedOnTheEdgeIntoTheFirstWait()
{
    TurnaroundWorkflow first;
    first.f.aircraft.engineRunning = true;
    first.machine.AttachAircraft(&first.f.aircraft);

    QCOMPARE(first.f.aircraft.onTurnaroundStartedCalls, 0);

    first.TickTo(TurnaroundPhase::WaitingAircraftReady);

    QCOMPARE(first.f.aircraft.onTurnaroundStartedCalls, 1);

    TurnaroundWorkflow workflow;

    WalkTheWholeTurnaround(workflow);

    QCOMPARE(workflow.f.aircraft.onTurnaroundStartedCalls, 1);

    workflow.StartNewFlightCycle();

    QCOMPARE(workflow.f.aircraft.onTurnaroundStartedCalls, 1);

    workflow.f.aircraft.smartSwitchActivated = false;
    workflow.f.aircraft.engineRunning = false;
    workflow.TickTo(TurnaroundPhase::WaitingAircraftReady);

    QCOMPARE(workflow.f.aircraft.onTurnaroundStartedCalls, 2);

    workflow.machine.Reset();
    workflow.machine.AttachAircraft(&workflow.f.aircraft);
    workflow.TickTo(TurnaroundPhase::WaitingAircraftReady);

    QCOMPARE(workflow.f.aircraft.onTurnaroundStartedCalls, 3);
    QCOMPARE(workflow.f.aircraft.onTurnaroundResumedCalls, 0);
}

void TurnaroundStateMachineTest::aResumeNotifiesTheResumeAndNeverTheStart()
{
    TurnaroundWorkflow workflow;

    QCOMPARE(Resume(workflow, CheckpointAt(TurnaroundPhase::Loading)), ResumeOutcome::Resumed);

    for (int tick = 0; tick < 5; ++tick)
    {
        workflow.machine.Tick();
    }

    QCOMPARE(workflow.f.aircraft.onTurnaroundResumedCalls, 1);
    QCOMPARE(workflow.f.aircraft.onTurnaroundStartedCalls, 0);
}

void TurnaroundStateMachineTest::anUnknownPhaseRefusesTheResumeAndLeavesTheMachineAlone()
{
    TurnaroundWorkflow workflow;
    workflow.AttachAircraft();
    const std::optional<TurnaroundCheckpoint> before = workflow.machine.TakeCheckpoint();
    FakeAircraft other;

    for (const TurnaroundPhase phase : {TurnaroundPhase::WaitingSupportedAircraft, TurnaroundPhase::WaitingAircraftReady,
                                        TurnaroundPhase::WaitingNewFlight, TurnaroundPhase::Count,
                                        static_cast<TurnaroundPhase>(99), static_cast<TurnaroundPhase>(-1)})
    {
        QCOMPARE(workflow.machine.ResumeFrom(CheckpointAt(phase), other, true), ResumeOutcome::UnknownPhase);
    }

    QVERIFY(before == workflow.machine.TakeCheckpoint());
    QCOMPARE(workflow.machine.GetPhase(), TurnaroundPhase::RepositionAircraft);
    QCOMPARE(other.onTurnaroundResumedCalls, 0);
    QVERIFY(other.callLog.empty());
}

void TurnaroundStateMachineTest::anUnreachableAircraftRefusesTheResumeAndLeavesTheMachineAlone()
{
    TurnaroundWorkflow workflow;
    FakeAircraft unreachable;
    unreachable.reachable = false;

    QCOMPARE(workflow.machine.ResumeFrom(BoardingFinishedCheckpoint(TurnaroundPhase::Loading), unreachable, false),
             ResumeOutcome::AircraftNotReachable);

    QCOMPARE(workflow.machine.GetPhase(), TurnaroundPhase::WaitingSupportedAircraft);
    QVERIFY(!workflow.machine.TakeCheckpoint().has_value());
    QCOMPARE(unreachable.onTurnaroundResumedCalls, 0);
    QVERIFY(unreachable.callLog.empty());

    workflow.machine.Tick();

    QCOMPARE(workflow.machine.GetPhase(), TurnaroundPhase::WaitingSupportedAircraft);
    QCOMPARE(unreachable.consumeSmartSwitchCalls, 0);

    workflow.AttachAircraft();
    const std::optional<TurnaroundCheckpoint> before = workflow.machine.TakeCheckpoint();

    QCOMPARE(workflow.machine.ResumeFrom(BoardingFinishedCheckpoint(TurnaroundPhase::Loading), unreachable, false),
             ResumeOutcome::AircraftNotReachable);

    QVERIFY(before == workflow.machine.TakeCheckpoint());

    workflow.machine.Tick();

    QCOMPARE(unreachable.consumeSmartSwitchCalls, 0);
}

void TurnaroundStateMachineTest::theUnknownPhaseIsRefusedBeforeTheReachabilityIsAsked()
{
    TurnaroundWorkflow workflow;
    FakeAircraft unreachable;
    unreachable.reachable = false;

    QCOMPARE(workflow.machine.ResumeFrom(CheckpointAt(TurnaroundPhase::WaitingNewFlight), unreachable, false),
             ResumeOutcome::UnknownPhase);
}

void TurnaroundStateMachineTest::onlyTheResumablePhasesGiveACheckpoint()
{
    TurnaroundWorkflow early;

    QVERIFY(!early.machine.TakeCheckpoint().has_value());

    early.f.aircraft.engineRunning = true;
    early.machine.AttachAircraft(&early.f.aircraft);
    early.TickTo(TurnaroundPhase::WaitingAircraftReady);

    QVERIFY(!early.machine.TakeCheckpoint().has_value());

    TurnaroundWorkflow walked;
    WalkTheWholeTurnaround(walked);

    QCOMPARE(walked.machine.GetPhase(), TurnaroundPhase::WaitingNewFlight);
    QVERIFY(!walked.machine.TakeCheckpoint().has_value());

    std::set<TurnaroundPhase> phases;
    for (const TurnaroundCheckpoint& checkpoint : walked.checkpoints)
    {
        phases.insert(checkpoint.phase);
        QVERIFY(IsResumablePhase(checkpoint.phase));
    }

    QCOMPARE(phases.size(), kResumablePhaseCount);
}

void TurnaroundStateMachineTest::theCheckpointCarriesTheMemoryOfTheAircraft()
{
    TurnaroundWorkflow workflow;
    workflow.AttachAircraft();
    workflow.f.aircraft.memoryToReturn.PutNumber("door.target", 1.0);
    workflow.f.aircraft.memoryToReturn.PutFlag("closeRequested", true);

    const std::optional<TurnaroundCheckpoint> checkpoint = workflow.machine.TakeCheckpoint();

    QVERIFY(checkpoint.has_value());
    QVERIFY(checkpoint->aircraftMemory == workflow.f.aircraft.memoryToReturn);
    QCOMPARE(checkpoint->phase, TurnaroundPhase::RepositionAircraft);
}

void TurnaroundStateMachineTest::aResumeHandsTheAircraftTheMemoryItSaved()
{
    TurnaroundWorkflow workflow;
    TurnaroundCheckpoint checkpoint = BoardingFinishedCheckpoint(TurnaroundPhase::WaitingReadyToPush);
    checkpoint.aircraftMemory.PutNumber("door.target", 1.0);

    QCOMPARE(workflow.machine.ResumeFrom(checkpoint, workflow.f.aircraft, true), ResumeOutcome::Resumed);

    QVERIFY(workflow.f.aircraft.resumedMemory == checkpoint.aircraftMemory);
    QCOMPARE(workflow.f.aircraft.resumedFacts.phase, TurnaroundPhase::WaitingReadyToPush);
    QVERIFY(workflow.f.aircraft.resumedFacts.boardingFinished);
    QVERIFY(workflow.f.aircraft.resumedFacts.gsxRestartedSinceSave);
}

void TurnaroundStateMachineTest::aCheckpointRoundTripsThroughAResume()
{
    TurnaroundWorkflow walked;
    ReachBoarding(walked);
    walked.f.aircraft.memoryToReturn.PutFlag("closeRequested", true);

    const std::optional<TurnaroundCheckpoint> taken = walked.machine.TakeCheckpoint();
    QVERIFY(taken.has_value());
    QCOMPARE(taken->phase, TurnaroundPhase::Loading);

    TurnaroundWorkflow resumed;
    ReturnTheWorldToAvailable(resumed, kStairsStand, taken->phase);
    resumed.f.aircraft.memoryToReturn = walked.f.aircraft.memoryToReturn;

    QCOMPARE(Resume(resumed, *taken), ResumeOutcome::Resumed);

    const std::optional<TurnaroundCheckpoint> again = resumed.machine.TakeCheckpoint();
    QVERIFY(again.has_value());
    QVERIFY(*again == *taken);
}

void TurnaroundStateMachineTest::resetForgetsTheVerdictsTheRulesAlreadyLogged()
{
    TurnaroundWorkflow workflow;
    CountingRule rule;
    workflow.f.aircraft.rules = {&rule};
    workflow.machine.AttachAircraft(&workflow.f.aircraft);

    workflow.machine.ObserveRules();
    workflow.machine.ObserveRules();

    QCOMPARE(LoggedCount(workflow, "Rule counting-rule would pass"), 1);

    workflow.machine.Reset();
    workflow.machine.AttachAircraft(&workflow.f.aircraft);
    workflow.machine.ObserveRules();

    QCOMPARE(LoggedCount(workflow, "Rule counting-rule would pass"), 2);
}

void TurnaroundStateMachineTest::theMemoryBagRoundTripsAFlagANumberAndATextExactly()
{
    const std::string label = "a|b \"quoted\" caf\xc3\xa9 a\xc3\xa7\xc3\xa3o";

    MemoryBag bag;
    bag.PutFlag("closed", true);
    bag.PutFlag("open", false);
    bag.PutNumber("fraction", 0.1);
    bag.PutNumber("negative", -12345.678901234567);
    bag.PutNumber("huge", 1.0e300);
    bag.PutNumber("whole", 3.0);
    bag.PutText("label", label);

    QVERIFY(bag.Flag("closed", false));
    QVERIFY(!bag.Flag("open", true));
    QVERIFY(bag.Number("fraction", 0.0) == 0.1);
    QVERIFY(bag.Number("negative", 0.0) == -12345.678901234567);
    QVERIFY(bag.Number("huge", 0.0) == 1.0e300);
    QVERIFY(bag.Number("whole", 0.0) == 3.0);
    QVERIFY(bag.Text("label", "") == label);
}

void TurnaroundStateMachineTest::theMemoryBagAnswersTheFallbackForAMissingNameOrAWrongKind()
{
    MemoryBag bag;
    bag.PutText("word", "abc");
    bag.PutText("half", "1.5x");
    bag.PutText("not-a-number", "nan");
    bag.PutText("infinity", "inf");
    bag.PutText("negative-infinity", "-inf");
    bag.PutNumber("number", 2.5);
    bag.PutFlag("flag", true);
    bag.PutText("nobody-asks", "ignored");

    QVERIFY(bag.Flag("missing", true));
    QVERIFY(!bag.Flag("missing", false));
    QCOMPARE(bag.Number("missing", 4.5), 4.5);
    QVERIFY(bag.Text("missing", "fallback") == "fallback");

    QVERIFY(bag.Flag("number", true));
    QVERIFY(!bag.Flag("number", false));
    QVERIFY(bag.Flag("word", true));
    QCOMPARE(bag.Number("word", 9.0), 9.0);
    QCOMPARE(bag.Number("half", 9.0), 9.0);
    QCOMPARE(bag.Number("not-a-number", 9.0), 9.0);
    QCOMPARE(bag.Number("infinity", 9.0), 9.0);
    QCOMPARE(bag.Number("negative-infinity", 9.0), 9.0);
}

void TurnaroundStateMachineTest::theMemoryBagKeepsTheOrderAndReplacesInPlace()
{
    MemoryBag bag;
    bag.PutNumber("first", 1.0);
    bag.PutFlag("second", true);
    bag.PutNumber("first", 2.0);

    QCOMPARE(bag.Entries().size(), std::size_t{2});
    QVERIFY(bag.Entries()[0].first == "first");
    QVERIFY(bag.Entries()[1].first == "second");
    QCOMPARE(bag.Number("first", 0.0), 2.0);
}

void TurnaroundStateMachineTest::theMemoryBagBuiltFromEntriesEqualsTheOneThatWasFilled()
{
    MemoryBag bag;
    bag.PutNumber("first", 1.5);
    bag.PutText("second", "text");

    const MemoryBag rebuilt(bag.Entries());

    QVERIFY(rebuilt == bag);
    QCOMPARE(rebuilt.Number("first", 0.0), 1.5);

    MemoryBag different = bag;
    different.PutText("second", "other");

    QVERIFY(!(different == bag));
}

void TurnaroundStateMachineTest::thePlanRoundTripsThroughTheStatus()
{
    AutomationStatus status;
    status.plannedFuelKg = 11000.0;
    status.plannedZfwKg = 61000.0;
    status.plannedOperatingEmptyKg = 42000.0;
    status.plannedPayloadKg = 14000.0;
    status.plannedCargoKg = 2500.0;
    status.plannedPassengers = 150;
    status.simbriefUnit = WeightUnit::Lb;
    status.plannedOrigin = "CDK2";
    status.plannedDestination = "CYEG";
    status.planGeneratedEpoch = 1790000000;

    const FlightPlan plan = turnaround::PlanOf(status);

    QCOMPARE(plan.fuelKg, 11000.0);
    QCOMPARE(plan.zfwKg, 61000.0);
    QCOMPARE(plan.operatingEmptyKg, 42000.0);
    QCOMPARE(plan.payloadKg.value_or(0.0), 14000.0);
    QCOMPARE(plan.cargoKg.value_or(0.0), 2500.0);
    QCOMPARE(plan.passengers, 150);
    QCOMPARE(plan.unit, WeightUnit::Lb);
    QVERIFY(plan.origin == "CDK2");
    QVERIFY(plan.destination == "CYEG");
    QCOMPARE(plan.generatedEpoch, 1790000000LL);

    AutomationStatus other;
    other.fuelProgress = 42.0;
    other.enabled = true;
    turnaround::ApplyPlan(other, plan);

    QVERIFY(turnaround::PlanOf(other) == plan);
    QCOMPARE(other.plannedFuelKg, 11000.0);
    QCOMPARE(other.plannedZfwKg, 61000.0);
    QCOMPARE(other.plannedOperatingEmptyKg, 42000.0);
    QCOMPARE(other.plannedPayloadKg.value_or(0.0), 14000.0);
    QCOMPARE(other.plannedCargoKg.value_or(0.0), 2500.0);
    QCOMPARE(other.plannedPassengers, 150);
    QCOMPARE(other.simbriefUnit, WeightUnit::Lb);
    QVERIFY(other.plannedOrigin == "CDK2");
    QVERIFY(other.plannedDestination == "CYEG");
    QCOMPARE(other.planGeneratedEpoch, 1790000000LL);
    QCOMPARE(other.fuelProgress, 42.0);
    QVERIFY(other.enabled);
}

void TurnaroundStateMachineTest::everyPhaseNameParsesBackAndAnUnknownNameDoesNot()
{
    for (int index = 0; index < static_cast<int>(TurnaroundPhase::Count); ++index)
    {
        const auto phase = static_cast<TurnaroundPhase>(index);
        const std::optional<TurnaroundPhase> parsed = TurnaroundPhaseFromString(TurnaroundPhaseToString(phase));

        QVERIFY2(parsed.has_value(), TurnaroundPhaseToString(phase));
        QCOMPARE(*parsed, phase);
    }

    QVERIFY(!TurnaroundPhaseFromString("NotAPhase").has_value());
    QVERIFY(!TurnaroundPhaseFromString("Unknown").has_value());
    QVERIFY(!TurnaroundPhaseFromString("").has_value());
}

void TurnaroundStateMachineTest::theResumablePhasesRunFromRepositionToCabinServices()
{
    int resumable = 0;

    for (int index = 0; index < static_cast<int>(TurnaroundPhase::Count); ++index)
    {
        resumable += IsResumablePhase(static_cast<TurnaroundPhase>(index)) ? 1 : 0;
    }

    QCOMPARE(resumable, static_cast<int>(kResumablePhaseCount));
    QVERIFY(!IsResumablePhase(TurnaroundPhase::WaitingSupportedAircraft));
    QVERIFY(!IsResumablePhase(TurnaroundPhase::WaitingAircraftReady));
    QVERIFY(IsResumablePhase(TurnaroundPhase::RepositionAircraft));
    QVERIFY(IsResumablePhase(TurnaroundPhase::CabinServices));
    QVERIFY(!IsResumablePhase(TurnaroundPhase::WaitingNewFlight));
    QVERIFY(!IsResumablePhase(TurnaroundPhase::Count));
    QVERIFY(!IsResumablePhase(static_cast<TurnaroundPhase>(99)));
    QVERIFY(!IsResumablePhase(static_cast<TurnaroundPhase>(-1)));
}

QTEST_APPLESS_MAIN(TurnaroundStateMachineTest)

#include "tst_state_machine.moc"
