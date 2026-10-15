#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_SYSTEMDSIMULATORLAUNCHER_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_SYSTEMDSIMULATORLAUNCHER_H

#include <functional>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include "SimulatorLauncher.h"

struct SystemctlOutcome
{
    bool started = false;
    int exitCode = -1;

    [[nodiscard]] bool Succeeded() const { return started && exitCode == 0; }
};

using SystemctlRunner = std::function<SystemctlOutcome(const QStringList& arguments)>;

struct SimulatorWatcherLocations
{
    QString unitDir;
    QString scriptPath;
    QString clientPath;
    QString installedClientPath;
    QStringList processNames;
};

[[nodiscard]] SystemctlOutcome RunSystemctlUser(const QStringList& arguments);
[[nodiscard]] SimulatorWatcherLocations DefaultWatcherLocations();
[[nodiscard]] QString ClientLaunchPath();

class SystemdSimulatorLauncher final : public SimulatorLauncher
{
public:
    SystemdSimulatorLauncher(SimulatorWatcherLocations locations, SystemctlRunner systemctl);

    [[nodiscard]] bool IsAvailable() const override;
    [[nodiscard]] bool IsEnabled() const override;
    [[nodiscard]] LaunchWithSimulatorResult Set(bool enabled) override;

private:
    [[nodiscard]] QString UnitPath() const;
    [[nodiscard]] bool Enable() const;
    [[nodiscard]] bool Disable() const;

    SimulatorWatcherLocations locations_;
    SystemctlRunner systemctl_;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_SYSTEMDSIMULATORLAUNCHER_H
