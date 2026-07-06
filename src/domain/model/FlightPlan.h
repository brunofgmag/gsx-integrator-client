#ifndef GSX_INTEGRATOR_CLIENT_DOMAIN_FLIGHTPLAN_H
#define GSX_INTEGRATOR_CLIENT_DOMAIN_FLIGHTPLAN_H

enum class FlightPlanStatus : int
{
    Idle = 0,
    Fetching = 1,
    Ready = 2,
    Error = 3,
};

struct FlightPlan
{
    double fuelKg = 0.0;
    double zfwKg = 0.0;
    int passengers = 0;
};

#endif // GSX_INTEGRATOR_CLIENT_DOMAIN_FLIGHTPLAN_H
