#ifndef GSX_INTEGRATOR_CLIENT_INTEGRATORSNAPSHOT_H
#define GSX_INTEGRATOR_CLIENT_INTEGRATORSNAPSHOT_H

#include <cmath>
#include <string>
#include "../../domain/model/FlightPlan.h"
#include "../../domain/turnaround/TurnaroundPhase.h"

struct IntegratorSnapshot
{
    bool connected = false;
    bool sessionActive = false;
    bool automationEnabled = false;
    bool gsxAvailable = false;
    bool aircraftSupported = false;
    bool canToggleAutomation = false;
    bool canStartLoading = false;
    bool canReloadSimbrief = false;
    bool refuelByGsx = false;
    bool refuelBySelf = false;
    bool gsxProfileConflict = false;
    bool gsxProfileFixable = false;
    bool cargoAircraft = false;

    std::string aircraftName;
    std::string aircraftProfileId;
    TurnaroundPhase phase = TurnaroundPhase::WaitingFlightPlan;
    FlightPlanStatus flightPlanStatus = FlightPlanStatus::Idle;

    double fuelProgress = 0.0;
    double boardingProgress = 0.0;
    double deboardingProgress = 0.0;
    double plannedFuelKg = 0.0;
    double loadedFuelKg = 0.0;
    double plannedZfwKg = 0.0;
    int plannedPax = 0;
    int boardedPax = 0;
    double targetFuelKg = 0.0;
    double targetZfwKg = 0.0;
    int targetPax = 0;
    int delayTicksRemaining = 0;
};

inline bool AreEquivalent(const IntegratorSnapshot& lhs, const IntegratorSnapshot& rhs)
{
    const auto nearlyEqual = [](const double first, const double second)
    {
        constexpr double epsilon = 0.001;
        return std::abs(first - second) <= epsilon;
    };

    return lhs.connected == rhs.connected &&
        lhs.sessionActive == rhs.sessionActive &&
        lhs.automationEnabled == rhs.automationEnabled &&
        lhs.gsxAvailable == rhs.gsxAvailable &&
        lhs.aircraftSupported == rhs.aircraftSupported &&
        lhs.canToggleAutomation == rhs.canToggleAutomation &&
        lhs.canStartLoading == rhs.canStartLoading &&
        lhs.canReloadSimbrief == rhs.canReloadSimbrief &&
        lhs.refuelByGsx == rhs.refuelByGsx &&
        lhs.refuelBySelf == rhs.refuelBySelf &&
        lhs.gsxProfileConflict == rhs.gsxProfileConflict &&
        lhs.gsxProfileFixable == rhs.gsxProfileFixable &&
        lhs.cargoAircraft == rhs.cargoAircraft &&
        lhs.aircraftName == rhs.aircraftName &&
        lhs.aircraftProfileId == rhs.aircraftProfileId &&
        lhs.phase == rhs.phase &&
        lhs.flightPlanStatus == rhs.flightPlanStatus &&
        nearlyEqual(lhs.fuelProgress, rhs.fuelProgress) &&
        nearlyEqual(lhs.boardingProgress, rhs.boardingProgress) &&
        nearlyEqual(lhs.deboardingProgress, rhs.deboardingProgress) &&
        nearlyEqual(lhs.plannedFuelKg, rhs.plannedFuelKg) &&
        nearlyEqual(lhs.loadedFuelKg, rhs.loadedFuelKg) &&
        nearlyEqual(lhs.plannedZfwKg, rhs.plannedZfwKg) &&
        lhs.plannedPax == rhs.plannedPax &&
        lhs.boardedPax == rhs.boardedPax &&
        nearlyEqual(lhs.targetFuelKg, rhs.targetFuelKg) &&
        nearlyEqual(lhs.targetZfwKg, rhs.targetZfwKg) &&
        lhs.targetPax == rhs.targetPax &&
        lhs.delayTicksRemaining == rhs.delayTicksRemaining;
}

#endif // GSX_INTEGRATOR_CLIENT_INTEGRATORSNAPSHOT_H
