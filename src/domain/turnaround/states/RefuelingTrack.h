#ifndef GSX_INTEGRATOR_CLIENT_DOMAIN_REFUELINGTRACK_H
#define GSX_INTEGRATOR_CLIENT_DOMAIN_REFUELINGTRACK_H

struct TurnaroundContext;

namespace RefuelingTrack
{
    [[nodiscard]] bool HasFuelMoved(const TurnaroundContext& ctx);

    [[nodiscard]] bool IsInterrupted(const TurnaroundContext& ctx);

    [[nodiscard]] bool Advance(TurnaroundContext& ctx);
}

#endif // GSX_INTEGRATOR_CLIENT_DOMAIN_REFUELINGTRACK_H
