#ifndef GSX_INTEGRATOR_CLIENT_DOMAIN_AUTOMATIONSTATUS_H
#define GSX_INTEGRATOR_CLIENT_DOMAIN_AUTOMATIONSTATUS_H

#include "FlightPlan.h"

struct AutomationStatus
{
    bool enabled = false;
    bool aircraftSupported = false;
    bool gsxAvailable = false;
    double fuelProgress = 0.0;
    double boardingProgress = 0.0;
    double deboardingProgress = 0.0;
    double plannedFuelKg = 0.0;
    double loadedFuelKg = 0.0;
    double plannedZfwKg = 0.0;
    int plannedPassengers = 0;
    int boardedPassengers = 0;
    double targetFuelKg = 0.0;
    double targetZfwKg = 0.0;
    int targetPassengers = 0;
    FlightPlanStatus flightPlanStatus = FlightPlanStatus::Idle;
    WeightUnit simbriefUnit = WeightUnit::Kg;

    bool operator==(const AutomationStatus&) const = default;

    void Reset() { *this = {}; }
};

#endif // GSX_INTEGRATOR_CLIENT_DOMAIN_AUTOMATIONSTATUS_H
