#ifndef GSX_INTEGRATOR_CLIENT_DOMAIN_PLANCONVERSION_H
#define GSX_INTEGRATOR_CLIENT_DOMAIN_PLANCONVERSION_H

#include "AutomationStatus.h"
#include "FlightPlan.h"

namespace turnaround
{
    [[nodiscard]] inline FlightPlan PlanOf(const AutomationStatus& status)
    {
        FlightPlan plan;
        plan.fuelKg = status.plannedFuelKg;
        plan.zfwKg = status.plannedZfwKg;
        plan.passengers = status.plannedPassengers;
        plan.unit = status.simbriefUnit;
        plan.origin = status.plannedOrigin;
        plan.destination = status.plannedDestination;
        plan.generatedEpoch = status.planGeneratedEpoch;
        plan.operatingEmptyKg = status.plannedOperatingEmptyKg;
        plan.payloadKg = status.plannedPayloadKg;
        plan.cargoKg = status.plannedCargoKg;

        return plan;
    }

    inline void ApplyPlan(AutomationStatus& status, const FlightPlan& plan)
    {
        status.plannedFuelKg = plan.fuelKg;
        status.plannedZfwKg = plan.zfwKg;
        status.plannedOperatingEmptyKg = plan.operatingEmptyKg;
        status.plannedPayloadKg = plan.payloadKg;
        status.plannedCargoKg = plan.cargoKg;
        status.plannedPassengers = plan.passengers;
        status.simbriefUnit = plan.unit;
        status.plannedOrigin = plan.origin;
        status.plannedDestination = plan.destination;
        status.planGeneratedEpoch = plan.generatedEpoch;
    }
}

#endif // GSX_INTEGRATOR_CLIENT_DOMAIN_PLANCONVERSION_H
