#ifndef GSX_INTEGRATOR_CLIENT_DOMAIN_AUTOMATIONSETTINGS_H
#define GSX_INTEGRATOR_CLIENT_DOMAIN_AUTOMATIONSETTINGS_H

struct AutomationSettings
{
    static constexpr double kDefaultFuelRateKgs = 60.0;

    int simbriefPilotId = 0;
    double fuelRateKgs = kDefaultFuelRateKgs;
    bool autoSelectGsxChoice = true;
    bool autoStartFlow = false;

    [[nodiscard]] double EffectiveFuelRateKgs() const
    {
        return fuelRateKgs > 0.0 ? fuelRateKgs : kDefaultFuelRateKgs;
    }
};

#endif // GSX_INTEGRATOR_CLIENT_DOMAIN_AUTOMATIONSETTINGS_H
