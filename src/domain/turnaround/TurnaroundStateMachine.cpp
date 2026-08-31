#include "TurnaroundStateMachine.h"

#include <algorithm>
#include <format>
#include <utility>
#include "states/WaitingFlightPlanState.h"
#include "states/RequestFuelState.h"
#include "states/RefuelingState.h"
#include "states/RequestBoardingState.h"
#include "states/BoardingState.h"
#include "states/RequestPushbackState.h"
#include "states/CallServicesState.h"
#include "states/CallCateringState.h"
#include "states/OnFlightState.h"
#include "states/RepositionAircraftState.h"
#include "states/WaitingPushbackToStartState.h"
#include "states/WaitingEnginesState.h"
#include "states/WaitingAircraftReadyState.h"
#include "states/WaitingReadyToPushState.h"
#include "states/WaitCateringState.h"
#include "states/PlaceGroundEquipmentState.h"
#include "states/PlaceArrivalGroundEquipmentState.h"
#include "states/RemoveGroundEquipmentState.h"
#include "states/WaitingDepartureState.h"
#include "states/WaitingEngineShutdownState.h"
#include "states/WaitingPowerOnState.h"
#include "states/WaitingSupportedAircraftState.h"
#include "../ports/Aircraft.h"
#include "../ports/DomainLogger.h"
#include "../ports/GsxMenuGateway.h"
#include "../model/AutomationStatus.h"
#include "../model/AutomationSettings.h"
#include "states/DeboardingState.h"
#include "states/CabinServicesState.h"
#include "states/RequestDeboardingState.h"
#include "states/WaitingNewFlightState.h"

namespace
{
    constexpr const char* TouchSurface(const bool fromSwitch, const bool fromApp)
    {
        if (fromSwitch && fromApp)
        {
            return "SmartSwitch and the EFB app";
        }

        return fromSwitch ? "SmartSwitch" : "EFB app";
    }
}

TurnaroundStateMachine::TurnaroundStateMachine(AutomationStatus* status,
                                               const AutomationSettings* settings,
                                               GsxGateway* gsxGateway,
                                               GsxMenuGateway* menuGateway,
                                               DomainLogger* logger,
                                               VariableWriter* variableWriter)
{
    context_.status = status;
    context_.settings = settings;
    context_.gsxGateway = gsxGateway;
    context_.menuGateway = menuGateway;
    context_.logger = logger;
    context_.variableWriter = variableWriter;

    RegisterStates();
}

void TurnaroundStateMachine::RegisterStates()
{
    auto add = [this](std::unique_ptr<TurnaroundState> state)
    {
        const auto index = static_cast<std::size_t>(state->Phase());
        states_[index] = std::move(state);
    };

    add(std::make_unique<WaitingSupportedAircraftState>());
    add(std::make_unique<WaitingAircraftReadyState>());
    add(std::make_unique<RepositionAircraftState>());
    add(std::make_unique<PlaceGroundEquipmentState>());
    add(std::make_unique<CallServicesState>());
    add(std::make_unique<WaitingFlightPlanState>());
    add(std::make_unique<WaitingPowerOnState>());
    add(std::make_unique<CallCateringState>());
    add(std::make_unique<RequestFuelState>());
    add(std::make_unique<RefuelingState>());
    add(std::make_unique<RequestBoardingState>());
    add(std::make_unique<BoardingState>());
    add(std::make_unique<WaitingReadyToPushState>());
    add(std::make_unique<WaitCateringState>());
    add(std::make_unique<RemoveGroundEquipmentState>());
    add(std::make_unique<RequestPushbackState>());
    add(std::make_unique<WaitingPushbackToStartState>());
    add(std::make_unique<WaitingEnginesState>());
    add(std::make_unique<WaitingDepartureState>());
    add(std::make_unique<OnFlightState>());
    add(std::make_unique<WaitingEngineShutdownState>());
    add(std::make_unique<PlaceArrivalGroundEquipmentState>());
    add(std::make_unique<RequestDeboardingState>());
    add(std::make_unique<DeboardingState>());
    add(std::make_unique<CabinServicesState>());
    add(std::make_unique<WaitingNewFlightState>());
}

void TurnaroundStateMachine::Tick()
{
    context_.gsxGateway->Observe();
    Step();
    PublishStatus();
}

void TurnaroundStateMachine::Step()
{
    if (context_.aircraft != nullptr && !context_.data.refuelBaselined)
    {
        context_.data.loadedFuelKg = context_.aircraft->GetCurrentFuelKg();
    }

    ResolvePilotTouch();

    if (ticksRemaining_ > 0)
    {
        --ticksRemaining_;
        if (ticksRemaining_ > 0)
        {
            TurnaroundState* const state = StateFor(phase_);
            if (state != nullptr)
            {
                state->ActOnRules(context_, RuleCadence::Fast);
            }

            return;
        }

        TransitionTo(pendingPhase_, pendingOrigin_);
    }

    const auto transition = EvaluateCurrentPhase();
    if (!transition)
    {
        context_.data.stateTickCount++;
        return;
    }

    if (transition->delayTicks > 0)
    {
        pendingPhase_ = transition->next;
        pendingOrigin_ = transition->origin;
        ticksRemaining_ = transition->delayTicks;
        return;
    }

    TransitionTo(transition->next, transition->origin);
}

void TurnaroundStateMachine::ResolvePilotTouch()
{
    const bool fromSwitch = context_.aircraft != nullptr && context_.aircraft->ConsumeSmartSwitch();
    const bool fromApp = std::exchange(appTouchPending_, false);

    context_.pilotTouched = fromSwitch || fromApp;
    if (!context_.pilotTouched || context_.logger == nullptr)
    {
        return;
    }

    context_.logger->LogInfo(
        std::format("Pilot touch from the {} (phase: {})",
                    TouchSurface(fromSwitch, fromApp),
                    TurnaroundPhaseToString(phase_))
    );
}

void TurnaroundStateMachine::PublishStatus() const
{
    if (context_.status == nullptr)
    {
        return;
    }

    context_.status->loadedFuelKg = context_.data.loadedFuelKg;
    context_.status->fuelRequestStalled = context_.data.fuelRequestStalled;
    context_.status->fuelPlanOverCapacity = context_.data.fuelPlanOverCapacity;
    context_.status->fuelDidNotStay = context_.data.fuelDidNotStay && !context_.data.fuelStayDismissed;
    context_.status->fuelShortfallKg = context_.data.fuelShortfallKg;
    context_.status->settledFuelKg = context_.data.settledFuelKg;
    context_.status->engineConfirmationBlock = context_.data.engineConfirmationBlock;
    context_.status->servicesStalled = context_.data.servicesStalled;
    context_.status->serviceInterrupted = context_.data.serviceInterrupted;
    context_.status->servicesWaitSeconds = context_.data.servicesWaitSeconds;
    context_.status->fuelProgress = context_.data.fuelProgress;
    context_.status->boardingProgress = context_.data.boardingProgress;
    context_.status->deboardingProgress = context_.data.deboardingProgress;
    context_.status->boardedPassengers = context_.data.boardedPassengers;
    context_.status->targetFuelKg = context_.data.plannedFuelKg;
    context_.status->targetZfwKg = context_.data.plannedZfwKg;
    context_.status->targetPassengers = context_.data.plannedPassengers;
}

void TurnaroundStateMachine::AttachAircraft(Aircraft* aircraft)
{
    context_.aircraft = aircraft;
}

void TurnaroundStateMachine::Reset()
{
    context_.aircraft = nullptr;
    context_.pilotTouched = false;
    appTouchPending_ = false;
    context_.data.Reset();
    phase_ = TurnaroundPhase::WaitingSupportedAircraft;
    pendingPhase_ = TurnaroundPhase::WaitingSupportedAircraft;
    pendingOrigin_ = TransitionOrigin::Reading;
    lastTransitionOrigin_ = TransitionOrigin::Reading;
    ticksRemaining_ = 0;
}

#ifndef NDEBUG
void TurnaroundStateMachine::DebugSkipPhase(const int delta)
{
    const int target = std::clamp(static_cast<int>(phase_) + delta,
                                  0, static_cast<int>(TurnaroundPhase::Count) - 1);
    ticksRemaining_ = 0;
    TransitionTo(static_cast<TurnaroundPhase>(target), TransitionOrigin::Reading);
}
#endif

std::optional<TurnaroundTransition> TurnaroundStateMachine::EvaluateCurrentPhase()
{
    TurnaroundState* state = StateFor(phase_);
    if (state == nullptr)
    {
        return std::nullopt;
    }

    return state->Evaluate(context_);
}

void TurnaroundStateMachine::TransitionTo(const TurnaroundPhase phase, const TransitionOrigin origin)
{
    lastTransitionOrigin_ = origin;

    if (context_.logger)
    {
        context_.logger->LogInfo(
            std::format("Transitioning: {} -> {}{}",
                        TurnaroundPhaseToString(phase_),
                        TurnaroundPhaseToString(phase),
                        origin == TransitionOrigin::Pilot ? " (unlocked by the pilot)" : "")
        );
    }

    if (phase == TurnaroundPhase::WaitingNewFlight)
    {
        context_.data.Reset();

        if (context_.menuGateway != nullptr)
        {
            context_.menuGateway->OnTurnaroundTurned();
        }
    }

    if (phase == TurnaroundPhase::WaitingForEngines && context_.menuGateway != nullptr)
    {
        context_.menuGateway->OnPushbackStarted();
    }

    if (phase == TurnaroundPhase::WaitingSupportedAircraft
        && phase_ == TurnaroundPhase::WaitingNewFlight
        && context_.settings != nullptr && !context_.settings->autoStartFlow
        && context_.status != nullptr)
    {
        context_.status->enabled = false;
    }

    phase_ = phase;
    context_.data.stateTickCount = 0;
    context_.data.serviceInterrupted = false;
}

void TurnaroundStateMachine::TickSlowRules()
{
    TurnaroundState* state = StateFor(phase_);
    if (state == nullptr)
    {
        return;
    }

    state->ActOnRules(context_, RuleCadence::Slow);
}

void TurnaroundStateMachine::ObserveRules()
{
    TurnaroundState* state = StateFor(phase_);
    if (state == nullptr)
    {
        return;
    }

    state->ObserveRules(context_, RuleCadence::Fast);
}

void TurnaroundStateMachine::ObserveSlowRules()
{
    TurnaroundState* state = StateFor(phase_);
    if (state == nullptr)
    {
        return;
    }

    state->ObserveRules(context_, RuleCadence::Slow);
}
