#ifndef GSX_INTEGRATOR_CLIENT_DOMAIN_PILOTUNLOCK_H
#define GSX_INTEGRATOR_CLIENT_DOMAIN_PILOTUNLOCK_H

#include <algorithm>
#include <array>
#include "TurnaroundPhase.h"

namespace PilotUnlock
{
    inline constexpr std::array kPhases{
        TurnaroundPhase::WaitingReadyToPush
    };

    [[nodiscard]] inline constexpr bool Accepts(const TurnaroundPhase phase)
    {
        return std::ranges::find(kPhases, phase) != kPhases.end();
    }
}

#endif // GSX_INTEGRATOR_CLIENT_DOMAIN_PILOTUNLOCK_H
