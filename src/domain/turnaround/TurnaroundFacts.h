#ifndef GSX_INTEGRATOR_CLIENT_DOMAIN_TURNAROUNDFACTS_H
#define GSX_INTEGRATOR_CLIENT_DOMAIN_TURNAROUNDFACTS_H

#include "TurnaroundPhase.h"

struct TurnaroundFacts
{
    TurnaroundPhase phase = TurnaroundPhase::WaitingSupportedAircraft;
    bool loadingStarted = false;
    bool refuelFinished = false;
    bool boardingFinished = false;
    bool departureDoorsHeld = false;
    bool passengerDoorsHeld = false;
    bool arrivalDoorsClosed = false;
    bool gsxRestartedSinceSave = false;
    double plannedFuelKg = 0.0;
    double plannedZfwKg = 0.0;
    double emptyZfwKg = 0.0;
    int plannedPassengers = 0;
};

#endif // GSX_INTEGRATOR_CLIENT_DOMAIN_TURNAROUNDFACTS_H
