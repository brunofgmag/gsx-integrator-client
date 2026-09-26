#ifndef GSX_INTEGRATOR_CLIENT_LAUNCHWITHSIMULATORRESULT_H
#define GSX_INTEGRATOR_CLIENT_LAUNCHWITHSIMULATORRESULT_H

#include <QtCore/QStringList>

struct LaunchWithSimulatorResult
{
    bool noSimulator = false;
    QStringList failedTargets;
};

#endif // GSX_INTEGRATOR_CLIENT_LAUNCHWITHSIMULATORRESULT_H
