#ifndef GSX_INTEGRATOR_CLIENT_DOMAIN_TURNAROUNDSTATEMACHINE_H
#define GSX_INTEGRATOR_CLIENT_DOMAIN_TURNAROUNDSTATEMACHINE_H

#include <array>
#include <cstddef>
#include <memory>
#include <optional>
#include "TurnaroundContext.h"
#include "TurnaroundTransition.h"
#include "states/TurnaroundState.h"
#include "TurnaroundPhase.h"

class TurnaroundStateMachine
{
public:
    TurnaroundStateMachine(AutomationStatus* status,
                           const AutomationSettings* settings,
                           GsxGateway* gsxGateway,
                           GsxMenuGateway* menuGateway,
                           DomainLogger* logger);

    void Tick();
    void AttachAircraft(Aircraft* aircraft);
    void Reset();
    void ConfirmLoading() { context_.data.loadingConfirmed = true; }

    [[nodiscard]] TurnaroundPhase GetPhase() const { return phase_; }
    [[nodiscard]] bool IsLoadingConfirmed() const { return context_.data.loadingConfirmed; }

private:
    static constexpr std::size_t kPhaseCount = static_cast<std::size_t>(TurnaroundPhase::Count);

    void RegisterStates();
    void Step();
    void PublishStatus() const;
    void TransitionTo(TurnaroundPhase phase);
    [[nodiscard]] std::optional<TurnaroundTransition> EvaluateCurrentPhase();

    [[nodiscard]] TurnaroundState* StateFor(const TurnaroundPhase phase) const
    {
        return states_[static_cast<std::size_t>(phase)].get();
    }

    TurnaroundContext context_;
    std::array<std::unique_ptr<TurnaroundState>, kPhaseCount> states_{};

    TurnaroundPhase phase_ = TurnaroundPhase::WaitingSupportedAircraft;
    TurnaroundPhase pendingPhase_ = TurnaroundPhase::WaitingSupportedAircraft;
    int ticksRemaining_ = 0;
};

#endif // GSX_INTEGRATOR_CLIENT_DOMAIN_TURNAROUNDSTATEMACHINE_H
