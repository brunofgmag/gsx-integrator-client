#ifndef GSX_INTEGRATOR_CLIENT_EFFECTIVESETTINGS_H
#define GSX_INTEGRATOR_CLIENT_EFFECTIVESETTINGS_H

#include <string>
#include "AppSettings.h"

inline double ResolveFuelRateKgs(const FuelRateMode mode, const double manualKgs, const double recommendedKgs)
{
    const double rate = mode == FuelRateMode::Manual ? manualKgs : recommendedKgs;

    return rate > 0.0 ? rate : AutomationSettings::kDefaultFuelRateKgs;
}

inline AutomationSettings ResolveAutomationSettings(const AppSettings& settings,
                                                    const std::string& aircraftProfileId,
                                                    const bool aircraftCarriesItsOwnStairs,
                                                    const double aircraftRecommendedFuelRateKgs)
{
    AutomationSettings result;
    result.simbriefPilotId = settings.simbriefPilotId;
    result.fuelRateKgs = ResolveFuelRateKgs(settings.fuelRateMode, settings.fuelRateKgs,
                                            aircraftRecommendedFuelRateKgs);
    result.autoSelectGsxChoice = settings.autoSelectGsxChoice;
    result.autoDeice = settings.autoDeice;
    result.useAircraftStairs = settings.useAircraftStairs || aircraftCarriesItsOwnStairs;
    result.crewBoarding = static_cast<CrewChoice>(settings.crewBoarding);
    result.crewDeboarding = static_cast<CrewChoice>(settings.crewDeboarding);
    result.autoStartFlow = settings.autoStartFlow;
    result.autoStartLoading = settings.autoStartLoading;
    result.skipReposition = settings.skipReposition;
    result.callGpu = settings.callGpu;
    result.callGpuOnArrival = settings.callGpuOnArrival;
    result.callBoardingEarly = settings.callBoardingEarly;
    result.callCatering = settings.callCatering;
    result.callLavatory = settings.callLavatory;
    result.callWater = settings.callWater;
    result.callCleaning = settings.callCleaning;
    result.gsxPanelMode = static_cast<GsxPanelMode>(settings.gsxPanelMode);

    const auto it = settings.profiles.find(aircraftProfileId);
    if (it == settings.profiles.end() || it->second.useGlobal)
    {
        return result;
    }

    const AircraftProfile& profile = it->second;
    if (profile.fuelRateMode != FuelRateMode::Global)
    {
        result.fuelRateKgs = ResolveFuelRateKgs(profile.fuelRateMode, profile.fuelRateKgs,
                                                aircraftRecommendedFuelRateKgs);
    }
    result.skipReposition = profile.skipReposition;
    result.callGpu = profile.callGpu;
    result.callGpuOnArrival = profile.callGpuOnArrival;
    result.callBoardingEarly = profile.callBoardingEarly;
    result.callCatering = profile.callCatering;
    result.callLavatory = profile.callLavatory;
    result.callWater = profile.callWater;
    result.callCleaning = profile.callCleaning;

    return result;
}

#endif // GSX_INTEGRATOR_CLIENT_EFFECTIVESETTINGS_H
