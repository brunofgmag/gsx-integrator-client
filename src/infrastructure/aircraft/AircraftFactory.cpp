#include "AircraftFactory.h"

#include <algorithm>
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

std::unique_ptr<Aircraft> DetectAircraft(const AircraftContext& context,
                                         const AircraftDescriptor** outDescriptor)
{
    char title[64] = {};
    if (!context.variableGateway->FetchAircraftName(title, sizeof(title)))
    {
        return nullptr;
    }

    char atcModel[64] = {};
    if (!context.variableGateway->FetchAtcModel(atcModel, sizeof(atcModel)))
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

    if (outDescriptor != nullptr)
    {
        *outDescriptor = descriptor;
    }

    std::unique_ptr<Aircraft> aircraft = descriptor->create(context, identity);

    LOG_INFO("Aircraft detected: %s", descriptor->name);

    return aircraft;
}

std::vector<AircraftProfileInfo> SupportedAircraftProfiles()
{
    std::vector<AircraftProfileInfo> infos;
    for (const AircraftDescriptor* descriptor : AircraftRegistry())
    {
        infos.push_back({descriptor->id, descriptor->shortCode, descriptor->name, descriptor->refuelBy});
    }
    std::ranges::sort(infos, {}, &AircraftProfileInfo::shortCode);

    return infos;
}
