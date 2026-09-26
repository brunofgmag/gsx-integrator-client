#ifndef GSX_INTEGRATOR_CLIENT_DOMAIN_BOARDINGTRACK_H
#define GSX_INTEGRATOR_CLIENT_DOMAIN_BOARDINGTRACK_H

struct TurnaroundContext;

namespace BoardingTrack
{
    [[nodiscard]] bool IsInterrupted(const TurnaroundContext& ctx);

    [[nodiscard]] bool Advance(TurnaroundContext& ctx);
}

#endif // GSX_INTEGRATOR_CLIENT_DOMAIN_BOARDINGTRACK_H
