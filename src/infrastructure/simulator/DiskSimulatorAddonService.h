#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_DISKSIMULATORADDONSERVICE_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_DISKSIMULATORADDONSERVICE_H

#include <vector>
#include <QtCore/QString>
#include "../../application/ports/SimulatorAddonService.h"
#include "../update/CommbusBundleInstaller.h"
#include "ExeXml.h"

struct SimulatorAddonPaths
{
    QString bundleDir;
    QString communityOverrideDir;
    QString homeDir;
    QString exePath;
};

class DiskSimulatorAddonService final : public SimulatorAddonService
{
public:
    DiskSimulatorAddonService(SimulatorAddonPaths paths, ProcessRunningCheck isProcessRunning);

    [[nodiscard]] bool IsLaunchedWithSimulator() const override;
    [[nodiscard]] LaunchWithSimulatorResult SetLaunchedWithSimulator(bool enabled) override;
    [[nodiscard]] CommbusBundleResult InstallCommbus() override;
    [[nodiscard]] CommbusBundleResult EnableCommbus() override;
    [[nodiscard]] CommbusBundleResult RemoveCommbus() override;

private:
    [[nodiscard]] std::vector<CommbusInstallTarget> CommbusTargets() const;
    [[nodiscard]] std::vector<ExeXmlTarget> ExeXmlTargets() const;
    [[nodiscard]] QString ExeName() const;

    SimulatorAddonPaths paths_;
    ProcessRunningCheck isProcessRunning_;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_DISKSIMULATORADDONSERVICE_H
