#include "TurnaroundStateMachine.h"

#include <format>
#include "states/WaitingFlightPlanState.h"
#include "states/RequestFuelState.h"
#include "states/RefuelingState.h"
#include "states/RequestBoardingState.h"
#include "states/BoardingState.h"
#include "states/RequestPushbackState.h"
#include "states/CallServicesState.h"
#include "states/OnFlightState.h"
#include "states/RepositionAircraftState.h"
#include "states/WaitingPushbackToStartState.h"
#include "states/WaitingEnginesState.h"
#include "states/WaitingAircraftReadyState.h"
#include "states/WaitingReadyToPushState.h"
#include "states/WaitCateringState.h"
#include "states/DisconnectGpuState.h"
#include "states/WaitingDepartureState.h"
#include "states/WaitingEngineShutdownState.h"
#include "states/WaitingPowerOnState.h"
#include "states/WaitingSupportedAircraftState.h"
#include "../ports/Aircraft.h"
#include "../ports/DomainLogger.h"
#include "../model/AutomationStatus.h"
#include "../model/AutomationSettings.h"
#include "states/DeboardingState.h"
#include "states/CabinServicesState.h"
#include "states/RequestDeboardingState.h"
#include "states/WaitingNewFlightState.h"

TurnaroundStateMachine::TurnaroundStateMachine(AutomationStatus* status,
                                               const AutomationSettings* settings,
                                               GsxGateway* gsxGateway,
                                               GsxMenuGateway* menuGateway,
                                               DomainLogger* logger)
{
    context_.status = status;
    context_.settings = settings;
    context_.gsxGateway = gsxGateway;
    context_.menuGateway = menuGateway;
    context_.logger = logger;

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
    add(std::make_unique<WaitingFlightPlanState>());
    add(std::make_unique<RepositionAircraftState>());
    add(std::make_unique<CallServicesState>());
    add(std::make_unique<WaitingPowerOnState>());
    add(std::make_unique<RequestFuelState>());
    add(std::make_unique<RefuelingState>());
    add(std::make_unique<RequestBoardingState>());
    add(std::make_unique<BoardingState>());
    add(std::make_unique<WaitingReadyToPushState>());
    add(std::make_unique<WaitCateringState>());
    add(std::make_unique<DisconnectGpuState>());
    add(std::make_unique<RequestPushbackState>());
    add(std::make_unique<WaitingPushbackToStartState>());
    add(std::make_unique<WaitingEnginesState>());
    add(std::make_unique<WaitingDepartureState>());
    add(std::make_unique<OnFlightState>());
    add(std::make_unique<WaitingEngineShutdownState>());
    add(std::make_unique<RequestDeboardingState>());
    add(std::make_unique<DeboardingState>());
    add(std::make_unique<CabinServicesState>());
    add(std::make_unique<WaitingNewFlightState>());
}

void TurnaroundStateMachine::Tick()
{
    Step();
    PublishStatus();
}

void TurnaroundStateMachine::Step()
{
    if (context_.aircraft != nullptr && !context_.data.refuelBaselined)
    {
        context_.data.loadedFuelKg = context_.aircraft->GetCurrentFuelKg();
    }

    context_.smartSwitchPressed = context_.aircraft != nullptr && context_.aircraft->ConsumeSmartSwitch();
    if (context_.smartSwitchPressed && context_.logger)
    {
        context_.logger->LogInfo(
            std::format("SmartSwitch pressed (phase: {})", TurnaroundPhaseToString(phase_))
        );
    }

    if (ticksRemaining_ > 0)
    {
        --ticksRemaining_;
        if (ticksRemaining_ > 0)
        {
            return;
        }

        TransitionTo(pendingPhase_);
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
        ticksRemaining_ = transition->delayTicks;
        return;
    }

    TransitionTo(transition->next);
}

void TurnaroundStateMachine::PublishStatus() const
{
    if (context_.status == nullptr)
    {
        return;
    }

    context_.status->loadedFuelKg = context_.data.loadedFuelKg;
    context_.status->fuelProgress = context_.data.fuelProgress;
    context_.status->boardingProgress = context_.data.boardingProgress;
    context_.status->deboardingProgress = context_.data.deboardingProgress;
    context_.status->boardedPassengers = context_.data.boardedPassengers;
}

void TurnaroundStateMachine::AttachAircraft(Aircraft* aircraft)
{
    context_.aircraft = aircraft;
}

void TurnaroundStateMachine::Reset()
{
    context_.aircraft = nullptr;
    context_.smartSwitchPressed = false;
    context_.data.Reset();
    phase_ = TurnaroundPhase::WaitingSupportedAircraft;
    pendingPhase_ = TurnaroundPhase::WaitingSupportedAircraft;
    ticksRemaining_ = 0;
}

std::optional<TurnaroundTransition> TurnaroundStateMachine::EvaluateCurrentPhase()
{
    TurnaroundState* state = StateFor(phase_);
    if (state == nullptr)
    {
        return std::nullopt;
    }

    return state->Evaluate(context_);
}

void TurnaroundStateMachine::TransitionTo(const TurnaroundPhase phase)
{
    if (context_.logger)
    {
        context_.logger->LogInfo(
            std::format("Transitioning: {} -> {}", TurnaroundPhaseToString(phase_), TurnaroundPhaseToString(phase))
        );
    }

    if (phase == TurnaroundPhase::WaitingNewFlight)
    {
        context_.data.Reset();
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
}
