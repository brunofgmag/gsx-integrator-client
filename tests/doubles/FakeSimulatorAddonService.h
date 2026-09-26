#ifndef GSX_INTEGRATOR_CLIENT_TESTS_FAKESIMULATORADDONSERVICE_H
#define GSX_INTEGRATOR_CLIENT_TESTS_FAKESIMULATORADDONSERVICE_H

#include "../../src/application/ports/SimulatorAddonService.h"

class FakeSimulatorAddonService final : public SimulatorAddonService
{
public:
    [[nodiscard]] bool IsLaunchedWithSimulator() const override
    {
        return launched;
    }

    [[nodiscard]] LaunchWithSimulatorResult SetLaunchedWithSimulator(const bool enabled) override
    {
        ++launchCalls;
        if (!launchResult.noSimulator && launchResult.failedTargets.isEmpty())
        {
            launched = enabled;
        }

        return launchResult;
    }

    [[nodiscard]] CommbusBundleResult InstallCommbus() override
    {
        ++installCalls;

        return installResult;
    }

    [[nodiscard]] CommbusBundleResult EnableCommbus() override
    {
        ++enableCalls;

        return enableResult;
    }

    [[nodiscard]] CommbusBundleResult RemoveCommbus() override
    {
        ++removeCalls;

        return removeResult;
    }

    bool launched = false;
    LaunchWithSimulatorResult launchResult;
    CommbusBundleResult installResult;
    CommbusBundleResult enableResult;
    CommbusBundleResult removeResult;
    int launchCalls = 0;
    int installCalls = 0;
    int enableCalls = 0;
    int removeCalls = 0;
};

#endif // GSX_INTEGRATOR_CLIENT_TESTS_FAKESIMULATORADDONSERVICE_H
