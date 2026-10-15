#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_COMMBUSINSTALLPROBE_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_COMMBUSINSTALLPROBE_H

#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include "../simulator/SimulatorCandidates.h"

[[nodiscard]] QString ParseInstalledPackagesPath(const QByteArray& userCfg);
[[nodiscard]] QString InstalledPackagesPathOf(const QString& userHome, const SimulatorCandidate& candidate);
[[nodiscard]] QString ParseManifestVersion(const QByteArray& manifestJson);
[[nodiscard]] QStringList CommunityDirsUnder(const QString& packagesPath);
[[nodiscard]] QStringList CandidateCommunityDirs();
[[nodiscard]] QStringList CandidateCommunityDirsIn(const QStringList& userHomes);
[[nodiscard]] QString DetectInstalledCommbusVersion(const QString& overrideDir = {});

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_COMMBUSINSTALLPROBE_H
