#ifndef GSX_INTEGRATOR_CLIENT_DOMAIN_PILOTTOUCH_H
#define GSX_INTEGRATOR_CLIENT_DOMAIN_PILOTTOUCH_H

#include <algorithm>
#include <array>
#include "TurnaroundPhase.h"

namespace PilotTouch
{
    inline constexpr std::array kPhases{
        TurnaroundPhase::RequestFuel,
        TurnaroundPhase::WaitingReadyToPush,
        TurnaroundPhase::WaitingForEngines,
        TurnaroundPhase::WaitingNewFlight
    };

    [[nodiscard]] inline constexpr bool Accepts(const TurnaroundPhase phase)
    {
        return std::ranges::find(kPhases, phase) != kPhases.end();
    }
}

#endif // GSX_INTEGRATOR_CLIENT_DOMAIN_PILOTTOUCH_H
