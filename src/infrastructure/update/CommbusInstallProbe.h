#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_COMMBUSINSTALLPROBE_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_COMMBUSINSTALLPROBE_H

#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtCore/QStringList>

[[nodiscard]] QString ParseInstalledPackagesPath(const QByteArray& userCfg);
[[nodiscard]] QString ParseManifestVersion(const QByteArray& manifestJson);
[[nodiscard]] QStringList CandidateCommunityDirs();
[[nodiscard]] QString DetectInstalledCommbusVersion(const QString& overrideDir = {});

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_COMMBUSINSTALLPROBE_H
