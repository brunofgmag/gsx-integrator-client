#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_XDGDIRECTORIES_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_XDGDIRECTORIES_H

#include <QtCore/QString>

[[nodiscard]] QString XdgDataHome();
[[nodiscard]] QString XdgConfigHome();
[[nodiscard]] QString ClientInstallDir();

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_XDGDIRECTORIES_H
