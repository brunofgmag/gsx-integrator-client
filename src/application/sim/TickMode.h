#ifndef GSX_INTEGRATOR_CLIENT_TICKMODE_H
#define GSX_INTEGRATOR_CLIENT_TICKMODE_H

enum class TickMode
{
    Idle,
    ObserveOnly,
    Driving
};

namespace TickModeResolution
{
    inline TickMode Resolve(const bool automationEnabled, const bool gsxAvailable, const bool actsOnTheSim)
    {
        if (automationEnabled && gsxAvailable)
        {
            return TickMode::Driving;
        }

        return automationEnabled || actsOnTheSim ? TickMode::ObserveOnly : TickMode::Idle;
    }
}

#endif // GSX_INTEGRATOR_CLIENT_TICKMODE_H
