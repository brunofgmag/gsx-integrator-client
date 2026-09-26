#ifndef GSX_INTEGRATOR_CLIENT_SIMULATORADDONSERVICE_H
#define GSX_INTEGRATOR_CLIENT_SIMULATORADDONSERVICE_H

#include "../model/CommbusBundleResult.h"
#include "../model/LaunchWithSimulatorResult.h"

class SimulatorAddonService
{
public:
    virtual ~SimulatorAddonService() = default;

    [[nodiscard]] virtual bool IsLaunchedWithSimulator() const = 0;
    [[nodiscard]] virtual LaunchWithSimulatorResult SetLaunchedWithSimulator(bool enabled) = 0;
    [[nodiscard]] virtual CommbusBundleResult InstallCommbus() = 0;
    [[nodiscard]] virtual CommbusBundleResult EnableCommbus() = 0;
    [[nodiscard]] virtual CommbusBundleResult RemoveCommbus() = 0;
};

#endif // GSX_INTEGRATOR_CLIENT_SIMULATORADDONSERVICE_H
