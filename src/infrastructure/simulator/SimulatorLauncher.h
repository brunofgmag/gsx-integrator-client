#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_SIMULATORLAUNCHER_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_SIMULATORLAUNCHER_H

#include "../../application/model/LaunchWithSimulatorResult.h"

class SimulatorLauncher
{
public:
    virtual ~SimulatorLauncher() = default;

    [[nodiscard]] virtual bool IsAvailable() const = 0;
    [[nodiscard]] virtual bool IsEnabled() const = 0;
    [[nodiscard]] virtual LaunchWithSimulatorResult Set(bool enabled) = 0;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_SIMULATORLAUNCHER_H
