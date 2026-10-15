#include "SimConnectLink.h"

#include <vector>
#include "../logging/LogMacros.h"
#include "ProtonPrefix.h"
#include "SimConnectXml.h"
#include "SteamLibraries.h"

SimConnectLink PrepareSimConnectLink(const QString& machineHome, const ProcessRunningCheck& isProcessRunning)
{
    const std::vector<ProtonPrefix> prefixes = FindProtonPrefixes(DefaultSteamappsDirs(machineHome));
    const std::vector<SimConnectXmlTarget> targets = SimConnectXmlTargets(prefixes);

    SimConnectLink link{.port = kStaticSimConnectPort, .simulatorsToRestart = {}};
    const ProtonPrefix* active = ActiveProtonPrefix(prefixes, isProcessRunning);

    for (std::size_t index = 0; index < targets.size(); ++index)
    {
        const SimConnectXmlTarget& target = targets[index];
        const SimConnectXmlResult result = EnsureSimConnectPort(target.path, [&target, &isProcessRunning]
        {
            return isProcessRunning(target.processName);
        });

        if (result.status == SimConnectXmlStatus::SimulatorRunning)
        {
            link.simulatorsToRestart.append(target.label);
        }
        else if (result.status == SimConnectXmlStatus::Failed)
        {
            LOG_WARN("Could not read or write the SimConnect port in %s", target.path.toUtf8().constData());
        }

        if (&prefixes[index] == active && result.port != 0)
        {
            link.port = result.port;
        }
    }

    return link;
}
