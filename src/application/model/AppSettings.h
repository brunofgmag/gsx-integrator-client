#ifndef GSX_INTEGRATOR_CLIENT_APPSETTINGS_H
#define GSX_INTEGRATOR_CLIENT_APPSETTINGS_H

#include <map>
#include <string>
#include "AircraftProfile.h"
#include "../../domain/model/AutomationSettings.h"

struct AppSettings
{
    int simbriefPilotId = 0;
    double fuelRateKgs = AutomationSettings::kDefaultFuelRateKgs;
    bool autoSelectGsxChoice = true;
    bool autoStartFlow = false;
    bool autoStartLoading = true;
    bool skipReposition = false;
    bool callGpu = false;
    bool callCatering = false;
    bool callLavatory = false;
    bool callWater = false;
    bool callCleaning = false;
    int themeMode = 2;
    std::string language = "system";
    int updateMode = 1;
    bool closeToTray = true;
    bool minimizeToTray = true;
    bool trayTipShown = false;
    std::map<std::string, AircraftProfile> profiles;
};

#endif // GSX_INTEGRATOR_CLIENT_APPSETTINGS_H
