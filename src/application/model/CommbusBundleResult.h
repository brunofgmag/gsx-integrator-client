#ifndef GSX_INTEGRATOR_CLIENT_COMMBUSBUNDLERESULT_H
#define GSX_INTEGRATOR_CLIENT_COMMBUSBUNDLERESULT_H

#include <vector>
#include <QtCore/QString>

enum class CommbusBundleStatus { Installed, UpToDate, SimRunning, Failed };

struct CommbusBundleTargetOutcome
{
    QString label;
    CommbusBundleStatus status = CommbusBundleStatus::Failed;
};

struct CommbusBundleResult
{
    QString bundledVersion;
    std::vector<CommbusBundleTargetOutcome> targets;
};

#endif // GSX_INTEGRATOR_CLIENT_COMMBUSBUNDLERESULT_H
