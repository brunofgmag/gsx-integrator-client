#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_SIMULATORWATCHERFILES_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_SIMULATORWATCHERFILES_H

#include <QtCore/QString>
#include <QtCore/QStringList>

inline constexpr auto kWatcherUnitName = "gsx-integrator-client.service";
inline constexpr auto kWatcherScriptName = "gsx-integrator-watcher.sh";
inline constexpr auto kClientAppImageName = "gsx-integrator-client.AppImage";

struct SimulatorWatcherFiles
{
    QString unit;
    QString script;
};

[[nodiscard]] SimulatorWatcherFiles WatcherFilesFor(const QString& scriptPath, const QStringList& processNames,
                                                    const QString& clientOverride = {});

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_SIMULATORWATCHERFILES_H
