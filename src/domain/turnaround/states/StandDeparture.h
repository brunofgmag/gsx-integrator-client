#ifndef GSX_INTEGRATOR_CLIENT_DOMAIN_STANDDEPARTURE_H
#define GSX_INTEGRATOR_CLIENT_DOMAIN_STANDDEPARTURE_H

#include "../TurnaroundContext.h"
#include "../../ports/Aircraft.h"
#include "../../ports/GsxGateway.h"

namespace turnaround
{
    inline constexpr double kTaxiGroundSpeedKnots = 5.0;

    [[nodiscard]] inline bool IsUnderTow(const TurnaroundContext& ctx)
    {
        return ctx.gsxGateway->HasPushbackStarted() && !ctx.gsxGateway->IsPushbackFinished();
    }

    [[nodiscard]] inline bool HasLeftTheStand(const TurnaroundContext& ctx)
    {
        if (!ctx.gsxGateway->IsAircraftOnGround())
        {
            return true;
        }

        if (IsUnderTow(ctx))
        {
            return false;
        }

        return ctx.aircraft->IsEngineRunning()
            && ctx.gsxGateway->GetGroundSpeedKnots() >= kTaxiGroundSpeedKnots;
    }
}

#endif // GSX_INTEGRATOR_CLIENT_DOMAIN_STANDDEPARTURE_H
