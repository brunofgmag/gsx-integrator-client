#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_DISKSIMULATORADDONSERVICE_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_DISKSIMULATORADDONSERVICE_H

#include <memory>
#include <vector>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include "../../application/ports/SimulatorAddonService.h"
#include "../update/CommbusBundleInstaller.h"
#include "ExeXml.h"
#include "SimulatorLauncher.h"

struct SimulatorAddonPaths
{
    QString bundleDir;
    QString communityOverrideDir;
    QString homeDir;
    QString exePath;
    QStringList userHomes;
};

class DiskSimulatorAddonService final : public SimulatorAddonService
{
public:
    DiskSimulatorAddonService(SimulatorAddonPaths paths, ProcessRunningCheck isProcessRunning,
                              std::unique_ptr<SimulatorLauncher> launcher = nullptr);

    [[nodiscard]] bool CanLaunchWithSimulator() const override;
    [[nodiscard]] bool IsLaunchedWithSimulator() const override;
    [[nodiscard]] LaunchWithSimulatorResult SetLaunchedWithSimulator(bool enabled) override;
    [[nodiscard]] CommbusBundleResult InstallCommbus() override;
    [[nodiscard]] CommbusBundleResult EnableCommbus() override;
    [[nodiscard]] CommbusBundleResult RemoveCommbus() override;

private:
    [[nodiscard]] std::vector<CommbusInstallTarget> CommbusTargets() const;
    [[nodiscard]] std::vector<ExeXmlTarget> ExeXmlTargets() const;
    [[nodiscard]] QString ExeName() const;
    [[nodiscard]] QStringList UserHomes() const;

    SimulatorAddonPaths paths_;
    ProcessRunningCheck isProcessRunning_;
    std::unique_ptr<SimulatorLauncher> launcher_;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_DISKSIMULATORADDONSERVICE_H
