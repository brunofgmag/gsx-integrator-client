#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_SIMCONNECTLINK_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_SIMCONNECTLINK_H

#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QtTypes>
#include "RunningProcess.h"

struct SimConnectLink
{
    quint16 port;
    QStringList simulatorsToRestart;
};

[[nodiscard]] SimConnectLink PrepareSimConnectLink(const QString& machineHome,
                                                   const ProcessRunningCheck& isProcessRunning);

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_SIMCONNECTLINK_H
