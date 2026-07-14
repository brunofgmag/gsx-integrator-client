#ifndef GSX_INTEGRATOR_CLIENT_AIRCRAFTPROFILE_H
#define GSX_INTEGRATOR_CLIENT_AIRCRAFTPROFILE_H

#include <string>
#include "../../domain/model/AutomationSettings.h"
#include "../../domain/ports/Aircraft.h"

struct AircraftProfile
{
    bool useGlobal = true;
    double fuelRateKgs = AutomationSettings::kDefaultFuelRateKgs;
    bool skipReposition = false;
    bool callGpu = false;
    bool callCatering = false;
    bool callLavatory = false;
    bool callWater = false;
    bool callCleaning = false;
};

struct AircraftProfileInfo
{
    std::string id;
    std::string shortCode;
    std::string name;
    RefuelBy refuelBy = RefuelBy::Gsx;
};

#endif // GSX_INTEGRATOR_CLIENT_AIRCRAFTPROFILE_H
