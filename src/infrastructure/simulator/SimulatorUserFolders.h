#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_SIMULATORUSERFOLDERS_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_SIMULATORUSERFOLDERS_H

#include <filesystem>
#include <optional>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include "RunningProcess.h"

[[nodiscard]] QStringList SimulatorUserHomes(const QString& machineHome);
[[nodiscard]] QStringList SimulatorUserHomes();
[[nodiscard]] QString SimulatorLocalPath(const QString& userHome, const QString& windowsPath);
[[nodiscard]] QString ActiveSimulatorRoamingAppData(const QString& machineHome,
                                                    const ProcessRunningCheck& isProcessRunning);
[[nodiscard]] QString SimulatorRoamingAppDataDir();
[[nodiscard]] std::optional<std::filesystem::path> SimulatorRoamingAppData();

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_SIMULATORUSERFOLDERS_H
