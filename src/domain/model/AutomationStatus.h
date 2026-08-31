#ifndef GSX_INTEGRATOR_CLIENT_DOMAIN_AUTOMATIONSTATUS_H
#define GSX_INTEGRATOR_CLIENT_DOMAIN_AUTOMATIONSTATUS_H

#include <string>
#include "FlightPlan.h"

enum class EngineConfirmationBlock : int
{
    None = 0,
    EnginesStopped,
    GsxNotAsking,
    ParkingBrakeReleased,
};

struct AutomationStatus
{
    bool enabled = false;
    bool aircraftSupported = false;
    bool gsxAvailable = false;
    bool fuelRequestStalled = false;
    bool fuelPlanOverCapacity = false;
    bool fuelDidNotStay = false;
    bool servicesStalled = false;
    bool serviceInterrupted = false;
    int servicesWaitSeconds = 0;
    double fuelProgress = 0.0;
    double boardingProgress = 0.0;
    double deboardingProgress = 0.0;
    double plannedFuelKg = 0.0;
    double loadedFuelKg = 0.0;
    double fuelShortfallKg = 0.0;
    double settledFuelKg = 0.0;
    double plannedZfwKg = 0.0;
    int plannedPassengers = 0;
    int boardedPassengers = 0;
    double targetFuelKg = 0.0;
    double targetZfwKg = 0.0;
    int targetPassengers = 0;
    FlightPlanStatus flightPlanStatus = FlightPlanStatus::Idle;
    FlightPlanFailure flightPlanFailure = FlightPlanFailure::None;
    int flightPlanHttpStatus = 0;
    std::string plannedOrigin;
    std::string plannedDestination;
    long long planGeneratedEpoch = 0;
    WeightUnit simbriefUnit = WeightUnit::Kg;
    EngineConfirmationBlock engineConfirmationBlock = EngineConfirmationBlock::None;

    bool operator==(const AutomationStatus&) const = default;

    void Reset() { *this = {}; }
};

#endif // GSX_INTEGRATOR_CLIENT_DOMAIN_AUTOMATIONSTATUS_H
