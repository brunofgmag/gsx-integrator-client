#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_CHILDENVIRONMENT_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_CHILDENVIRONMENT_H

#include <QtCore/QProcessEnvironment>

[[nodiscard]] QProcessEnvironment ChildProcessEnvironment(const QProcessEnvironment& environment);

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_CHILDENVIRONMENT_H
