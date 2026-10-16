#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_RUNNINGPROCESS_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_RUNNINGPROCESS_H

#include <functional>
#include <QtCore/QString>

using ProcessRunningCheck = std::function<bool(const QString& processName)>;

[[nodiscard]] bool IsProcessRunning(const QString& exeName);
[[nodiscard]] bool IsProcessRunningUnder(const QString& procRoot, const QString& exeName);

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_RUNNINGPROCESS_H
