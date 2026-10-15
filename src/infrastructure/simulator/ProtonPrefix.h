#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PROTONPREFIX_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PROTONPREFIX_H

#include <vector>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include "RunningProcess.h"
#include "SimulatorCandidates.h"

struct ProtonPrefix
{
    const SimulatorCandidate* simulator;
    QString root;
};

struct ProtonKnownFolders
{
    QString home;
    QString roaming;
    QString local;
};

[[nodiscard]] std::vector<ProtonPrefix> FindProtonPrefixes(const QStringList& steamappsDirs);
[[nodiscard]] ProtonKnownFolders ProtonKnownFoldersOf(const QString& prefixRoot);
[[nodiscard]] QString ProtonPrefixRootOfUserHome(const QString& userHome);
[[nodiscard]] QString ProtonPathFromWindows(const QString& prefixRoot, const QString& windowsPath);
[[nodiscard]] const ProtonPrefix* ActiveProtonPrefix(const std::vector<ProtonPrefix>& prefixes,
                                                     const ProcessRunningCheck& isProcessRunning);

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PROTONPREFIX_H
