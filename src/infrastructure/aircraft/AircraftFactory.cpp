#include "AircraftFactory.h"

#include <string>
#include "AircraftIdentity.h"
#include "AircraftRegistry.h"
#include "../logging/LogMacros.h"
#include "../../infrastructure/simvars/VariableGateway.h"

namespace
{
    bool ShouldLogUnsupported(const AircraftIdentity& identity)
    {
        static std::string lastTitle;
        if (identity.title == lastTitle)
        {
            return false;
        }
        lastTitle = identity.title;
        return true;
    }
}

std::unique_ptr<Aircraft> DetectAircraft(VariableGateway* variableGateway, AutomationStatus* status)
{
    char title[64] = {};
    if (!variableGateway->FetchAircraftName(title, sizeof(title)))
    {
        return nullptr;
    }

    char atcModel[64] = {};
    if (!variableGateway->FetchAtcModel(atcModel, sizeof(atcModel)))
    {
        atcModel[0] = '\0';
    }

    const AircraftIdentity identity{title, atcModel};
    const AircraftDescriptor* descriptor = MatchAircraft(AircraftRegistry(), identity);
    if (descriptor == nullptr)
    {
        if (ShouldLogUnsupported(identity))
        {
            LOG_INFO("Unsupported aircraft: title='%s' atcModel='%s'",
                     identity.title.c_str(), identity.atcModel.c_str());
        }
        return nullptr;
    }

    std::unique_ptr<Aircraft> aircraft = descriptor->create(variableGateway, status, identity);
    LOG_INFO("Aircraft detected: %s", aircraft->GetName());
    return aircraft;
}
