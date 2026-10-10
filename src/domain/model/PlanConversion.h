#ifndef GSX_INTEGRATOR_CLIENT_DOMAIN_PLANCONVERSION_H
#define GSX_INTEGRATOR_CLIENT_DOMAIN_PLANCONVERSION_H

#include "AutomationStatus.h"
#include "FlightPlan.h"

namespace turnaround
{
    [[nodiscard]] inline FlightPlan PlanOf(const AutomationStatus& status)
    {
        FlightPlan plan;
        auto& [fuelKg, zfwKg, passengers, unit, origin, destination, generatedEpoch, operatingEmptyKg, payloadKg, cargoKg] = plan;
        fuelKg = status.plannedFuelKg;
        zfwKg = status.plannedZfwKg;
        passengers = status.plannedPassengers;
        unit = status.simbriefUnit;
        origin = status.plannedOrigin;
        destination = status.plannedDestination;
        generatedEpoch = status.planGeneratedEpoch;
        operatingEmptyKg = status.plannedOperatingEmptyKg;
        payloadKg = status.plannedPayloadKg;
        cargoKg = status.plannedCargoKg;

        return plan;
    }

    inline void ApplyPlan(AutomationStatus& status, const FlightPlan& plan)
    {
        const auto& [fuelKg, zfwKg, passengers, unit, origin, destination, generatedEpoch, operatingEmptyKg, payloadKg, cargoKg] = plan;
        status.plannedFuelKg = fuelKg;
        status.plannedZfwKg = zfwKg;
        status.plannedOperatingEmptyKg = operatingEmptyKg;
        status.plannedPayloadKg = payloadKg;
        status.plannedCargoKg = cargoKg;
        status.plannedPassengers = passengers;
        status.simbriefUnit = unit;
        status.plannedOrigin = origin;
        status.plannedDestination = destination;
        status.planGeneratedEpoch = generatedEpoch;
    }
}

#endif // GSX_INTEGRATOR_CLIENT_DOMAIN_PLANCONVERSION_H
