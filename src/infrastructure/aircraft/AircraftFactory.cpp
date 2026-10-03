#include "AircraftFactory.h"

#include <algorithm>
#include <array>
#include <string>
#include "AircraftIdentity.h"
#include "AircraftRegistry.h"
#include "../logging/LogMacros.h"
#include "../../infrastructure/simvars/VariableGateway.h"

namespace
{
    constexpr int kAircraftStringSize = 256;

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
    std::array<char, kAircraftStringSize> title{};
    const bool titleArrived = context.variableGateway->FetchAircraftName(title.data(), kAircraftStringSize);

    std::array<char, kAircraftStringSize> atcModel{};
    const bool atcModelArrived = context.variableGateway->FetchAtcModel(atcModel.data(), kAircraftStringSize);

    if (!titleArrived || !atcModelArrived)
    {
        return nullptr;
    }

    const AircraftIdentity identity{.title = title.data(), .atcModel = atcModel.data()};
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
        infos.push_back({.id = descriptor->id,
                         .shortCode = descriptor->shortCode,
                         .name = descriptor->name,
                         .refuelBy = descriptor->refuelBy,
                         .recommendedFuelRateKgs = descriptor->fuelRateKgs});
    }
    std::ranges::sort(infos, {}, &AircraftProfileInfo::shortCode);

    return infos;
}
