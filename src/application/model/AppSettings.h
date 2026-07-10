#ifndef GSX_INTEGRATOR_CLIENT_APPSETTINGS_H
#define GSX_INTEGRATOR_CLIENT_APPSETTINGS_H

#include <string>
#include "../../domain/model/AutomationSettings.h"

struct AppSettings
{
    int simbriefPilotId = 0;
    double fuelRateKgs = AutomationSettings::kDefaultFuelRateKgs;
    bool autoSelectGsxChoice = true;
    bool autoStartFlow = false;
    int themeMode = 2;
    std::string language = "system";
    int updateMode = 1;
    bool closeToTray = true;
    bool minimizeToTray = true;
    bool trayTipShown = false;
};

#endif // GSX_INTEGRATOR_CLIENT_APPSETTINGS_H
