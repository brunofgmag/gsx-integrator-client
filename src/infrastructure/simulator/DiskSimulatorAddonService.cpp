#include "DiskSimulatorAddonService.h"

#include <algorithm>
#include <iterator>
#include <utility>

#include "WindowsFileName.h"

namespace
{
    constexpr auto kAppDisplayName = "GSX Integrator";
}

DiskSimulatorAddonService::DiskSimulatorAddonService(SimulatorAddonPaths paths, ProcessRunningCheck isProcessRunning,
                                                     std::unique_ptr<SimulatorLauncher> launcher)
    : paths_(std::move(paths)),
      isProcessRunning_(std::move(isProcessRunning)),
      launcher_(std::move(launcher))
{
}

bool DiskSimulatorAddonService::CanLaunchWithSimulator() const
{
    return launcher_ == nullptr || launcher_->IsAvailable();
}

bool DiskSimulatorAddonService::IsLaunchedWithSimulator() const
{
    if (launcher_ != nullptr)
    {
        return launcher_->IsEnabled();
    }

    const QString exeName = ExeName();

    return std::ranges::any_of(ExeXmlTargets(), [&exeName](const ExeXmlTarget& target)
    {
        return ExeXmlHasEnabledEntry(target.path, exeName);
    });
}

LaunchWithSimulatorResult DiskSimulatorAddonService::SetLaunchedWithSimulator(const bool enabled)
{
    if (launcher_ != nullptr)
    {
        return launcher_->Set(enabled);
    }

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
    std::vector<CommbusInstallTarget> targets;
    for (const QString& userHome : UserHomes())
    {
        for (CommbusInstallTarget& target : ResolveCommbusInstallTargets(paths_.communityOverrideDir, userHome))
        {
            const bool known = std::ranges::any_of(targets, [&target](const CommbusInstallTarget& existing)
            {
                return existing.communityPath == target.communityPath;
            });
            if (!known)
            {
                targets.push_back(std::move(target));
            }
        }
    }

    return targets;
}

std::vector<ExeXmlTarget> DiskSimulatorAddonService::ExeXmlTargets() const
{
    std::vector<ExeXmlTarget> targets;
    for (const QString& userHome : UserHomes())
    {
        std::vector<ExeXmlTarget> found = CandidateExeXmlTargets(userHome);
        targets.insert(targets.end(), std::make_move_iterator(found.begin()), std::make_move_iterator(found.end()));
    }

    return targets;
}

QStringList DiskSimulatorAddonService::UserHomes() const
{
    return paths_.userHomes.isEmpty() ? QStringList{paths_.homeDir} : paths_.userHomes;
}

QString DiskSimulatorAddonService::ExeName() const
{
    return WindowsFileName(paths_.exePath);
}
