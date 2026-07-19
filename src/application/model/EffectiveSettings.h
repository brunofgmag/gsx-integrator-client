#ifndef GSX_INTEGRATOR_CLIENT_EFFECTIVESETTINGS_H
#define GSX_INTEGRATOR_CLIENT_EFFECTIVESETTINGS_H

#include <string>
#include "AppSettings.h"

inline AutomationSettings ResolveAutomationSettings(const AppSettings& settings,
                                                    const std::string& aircraftProfileId)
{
    AutomationSettings result;
    result.simbriefPilotId = settings.simbriefPilotId;
    result.fuelRateKgs = settings.fuelRateKgs;
    result.autoSelectGsxChoice = settings.autoSelectGsxChoice;
    result.autoDeice = settings.autoDeice;
    result.crewBoarding = static_cast<CrewBoarding>(settings.crewBoarding);
    result.autoStartFlow = settings.autoStartFlow;
    result.autoStartLoading = settings.autoStartLoading;
    result.skipReposition = settings.skipReposition;
    result.callGpu = settings.callGpu;
    result.callGpuOnArrival = settings.callGpuOnArrival;
    result.callCatering = settings.callCatering;
    result.callLavatory = settings.callLavatory;
    result.callWater = settings.callWater;
    result.callCleaning = settings.callCleaning;
    result.openGsxOnRequests = settings.openGsxOnRequests;

    const auto it = settings.profiles.find(aircraftProfileId);
    if (it == settings.profiles.end() || it->second.useGlobal)
    {
        return result;
    }

    const AircraftProfile& profile = it->second;
    result.fuelRateKgs = profile.fuelRateKgs;
    result.skipReposition = profile.skipReposition;
    result.callGpu = profile.callGpu;
    result.callGpuOnArrival = profile.callGpuOnArrival;
    result.callCatering = profile.callCatering;
    result.callLavatory = profile.callLavatory;
    result.callWater = profile.callWater;
    result.callCleaning = profile.callCleaning;

    return result;
}

#endif // GSX_INTEGRATOR_CLIENT_EFFECTIVESETTINGS_H
