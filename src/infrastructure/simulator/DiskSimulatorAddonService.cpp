#include "DiskSimulatorAddonService.h"

#include <algorithm>
#include <utility>
#include <QtCore/QFileInfo>

namespace
{
    constexpr auto kAppDisplayName = "GSX Integrator";
}

DiskSimulatorAddonService::DiskSimulatorAddonService(SimulatorAddonPaths paths, ProcessRunningCheck isProcessRunning)
    : paths_(std::move(paths)),
      isProcessRunning_(std::move(isProcessRunning))
{
}

bool DiskSimulatorAddonService::IsLaunchedWithSimulator() const
{
    const QString exeName = ExeName();

    return std::ranges::any_of(ExeXmlTargets(), [&exeName](const ExeXmlTarget& target)
    {
        return ExeXmlHasEnabledEntry(target.path, exeName);
    });
}

LaunchWithSimulatorResult DiskSimulatorAddonService::SetLaunchedWithSimulator(const bool enabled)
{
    const std::vector<ExeXmlTarget> targets = ExeXmlTargets();

    LaunchWithSimulatorResult result;
    result.noSimulator = targets.empty();
    for (const ExeXmlTarget& target : targets)
    {
        const bool written = enabled
                                 ? ExeXmlAddUpdate(target.path, paths_.exePath, QLatin1String(kAppDisplayName))
                                 : ExeXmlRemove(target.path, ExeName());
        if (!written)
        {
            result.failedTargets.append(target.label);
        }
    }

    return result;
}

CommbusBundleResult DiskSimulatorAddonService::InstallCommbus()
{
    return InstallCommbusBundle(paths_.bundleDir, CommbusTargets(), isProcessRunning_);
}

CommbusBundleResult DiskSimulatorAddonService::EnableCommbus()
{
    return EnableCommbusBundle(paths_.bundleDir, CommbusTargets(), isProcessRunning_);
}

CommbusBundleResult DiskSimulatorAddonService::RemoveCommbus()
{
    return RemoveCommbusBundle(CommbusTargets(), isProcessRunning_);
}

std::vector<CommbusInstallTarget> DiskSimulatorAddonService::CommbusTargets() const
{
    return ResolveCommbusInstallTargets(paths_.communityOverrideDir, paths_.homeDir);
}

std::vector<ExeXmlTarget> DiskSimulatorAddonService::ExeXmlTargets() const
{
    return CandidateExeXmlTargets(paths_.homeDir);
}

QString DiskSimulatorAddonService::ExeName() const
{
    return QFileInfo(paths_.exePath).fileName();
}
