#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_COMMBUSBUNDLEINSTALLER_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_COMMBUSBUNDLEINSTALLER_H

#include <functional>
#include <vector>
#include <QtCore/QString>
#include "../../application/model/CommbusBundleResult.h"

struct CommbusInstallTarget
{
    QString label;
    QString communityPath;
    QString processName;
};

using ProcessRunningCheck = std::function<bool(const QString& processName)>;

[[nodiscard]] std::vector<CommbusInstallTarget> DetectCommbusInstallTargets(const QString& homeDir);
[[nodiscard]] std::vector<CommbusInstallTarget> ResolveCommbusInstallTargets(const QString& overrideDir,
                                                                          const QString& homeDir);
[[nodiscard]] QString InstalledCommbusPackageVersion(const QString& communityPath);
[[nodiscard]] CommbusBundleResult InstallCommbusBundle(const QString& bundleDir,
                                                       const std::vector<CommbusInstallTarget>& targets,
                                                       const ProcessRunningCheck& isProcessRunning);
[[nodiscard]] CommbusBundleResult EnableCommbusBundle(const QString& bundleDir,
                                                      const std::vector<CommbusInstallTarget>& targets,
                                                      const ProcessRunningCheck& isProcessRunning);
[[nodiscard]] CommbusBundleResult RemoveCommbusBundle(const std::vector<CommbusInstallTarget>& targets,
                                                      const ProcessRunningCheck& isProcessRunning);
[[nodiscard]] bool IsProcessRunning(const QString& exeName);

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_COMMBUSBUNDLEINSTALLER_H
