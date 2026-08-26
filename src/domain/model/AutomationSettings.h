#ifndef GSX_INTEGRATOR_CLIENT_DOMAIN_AUTOMATIONSETTINGS_H
#define GSX_INTEGRATOR_CLIENT_DOMAIN_AUTOMATIONSETTINGS_H

enum class GsxPanelMode
{
    Never = 0,
    OnPushback = 1,
    AllRequests = 2
};

enum class CrewChoice
{
    Nobody = 0,
    Crew = 1,
    Pilots = 2,
    Both = 3
};

struct AutomationSettings
{
    static constexpr double kDefaultFuelRateKgs = 20.0;

    int simbriefPilotId = 0;
    double fuelRateKgs = kDefaultFuelRateKgs;
    bool autoSelectGsxChoice = true;
    bool autoDeice = false;
    bool useAircraftStairs = false;
    CrewChoice crewBoarding = CrewChoice::Both;
    CrewChoice crewDeboarding = CrewChoice::Both;
    bool autoStartFlow = true;
    bool autoStartLoading = true;
    bool skipReposition = false;
    bool callGpu = false;
    bool callGpuOnArrival = false;
    bool callCatering = false;
    bool callLavatory = false;
    bool callWater = false;
    bool callCleaning = false;
    GsxPanelMode gsxPanelMode = GsxPanelMode::OnPushback;

    [[nodiscard]] double EffectiveFuelRateKgs() const
    {
        return fuelRateKgs > 0.0 ? fuelRateKgs : kDefaultFuelRateKgs;
    }
};

#endif // GSX_INTEGRATOR_CLIENT_DOMAIN_AUTOMATIONSETTINGS_H
