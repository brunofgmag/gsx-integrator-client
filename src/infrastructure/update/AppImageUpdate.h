#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_APPIMAGEUPDATE_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_APPIMAGEUPDATE_H

#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QtTypes>

struct AppImageUpdateTarget
{
    QString appImagePath;
    QString stagedPath;
    QString error;
};

[[nodiscard]] QString StagedAppImagePathFor(const QString& appImageEnvironment);
[[nodiscard]] AppImageUpdateTarget ResolveAppImageUpdateTarget(const QString& appImageEnvironment);
[[nodiscard]] bool InstallStagedAppImage(const QString& stagedPath, const QString& appImagePath);
[[nodiscard]] bool RecordInstalledVersion(const QString& appImagePath, const QString& dataHome,
                                          const QString& version);
[[nodiscard]] QStringList AppImageRelaunchArguments(qint64 appPid, const QString& appImagePath);
[[nodiscard]] bool RelaunchAppImage(qint64 appPid, const QString& appImagePath);

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_APPIMAGEUPDATE_H
