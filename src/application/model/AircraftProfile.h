#ifndef GSX_INTEGRATOR_CLIENT_AIRCRAFTPROFILE_H
#define GSX_INTEGRATOR_CLIENT_AIRCRAFTPROFILE_H

#include <string>
#include "../../domain/model/AutomationSettings.h"
#include "../../domain/ports/Aircraft.h"

enum class FuelRateMode
{
    Recommended = 0,
    Manual = 1,
    Global = 2
};

struct AircraftProfile
{
    bool useGlobal = true;
    FuelRateMode fuelRateMode = FuelRateMode::Recommended;
    double fuelRateKgs = AutomationSettings::kDefaultFuelRateKgs;
    bool skipReposition = false;
    bool callGpu = false;
    bool callGpuOnArrival = false;
    bool callBoardingEarly = false;
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
    double recommendedFuelRateKgs = 0.0;
};

#endif // GSX_INTEGRATOR_CLIENT_AIRCRAFTPROFILE_H
