#ifndef GSX_INTEGRATOR_CLIENT_INTEGRATORSNAPSHOT_H
#define GSX_INTEGRATOR_CLIENT_INTEGRATORSNAPSHOT_H

#include <cmath>
#include <string>
#include "../../domain/model/FlightPlan.h"
#include "../../domain/turnaround/TurnaroundPhase.h"
#include "../../domain/turnaround/TurnaroundTransition.h"

struct SnapshotDouble
{
    double value = 0.0;

    constexpr SnapshotDouble() = default;
    constexpr SnapshotDouble(const double initial) : value(initial) {}

    constexpr operator double() const { return value; }

    static constexpr double kEpsilon = 0.001;

    friend constexpr bool operator==(const SnapshotDouble lhs, const SnapshotDouble rhs)
    {
        return lhs.value - rhs.value <= kEpsilon && rhs.value - lhs.value <= kEpsilon;
    }

    friend constexpr bool operator==(const SnapshotDouble lhs, const double rhs)
    {
        return lhs.value - rhs <= kEpsilon && rhs - lhs.value <= kEpsilon;
    }
};

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
    bool pmdgOptionsConflict = false;
    bool pmdgOptionsFixable = false;
    bool cargoDoorStuck = false;
    bool fuelRequestStalled = false;
    bool cargoAircraft = false;
    bool efbFlightPlan = false;

    std::string aircraftName;
    std::string aircraftProfileId;
    TurnaroundPhase phase = TurnaroundPhase::WaitingFlightPlan;
    FlightPlanStatus flightPlanStatus = FlightPlanStatus::Idle;
    std::string simbriefRefusal;

    SnapshotDouble fuelProgress;
    SnapshotDouble boardingProgress;
    SnapshotDouble deboardingProgress;
    SnapshotDouble plannedFuelKg;
    SnapshotDouble loadedFuelKg;
    SnapshotDouble plannedZfwKg;
    int plannedPax = 0;
    int boardedPax = 0;
    SnapshotDouble targetFuelKg;
    SnapshotDouble targetZfwKg;
    int targetPax = 0;
    int delayTicksRemaining = 0;
    int autoWeightUnit = 0;

    bool operator==(const IntegratorSnapshot&) const = default;
};

inline bool AreEquivalent(const IntegratorSnapshot& lhs, const IntegratorSnapshot& rhs)
{
    return lhs == rhs;
}

#endif // GSX_INTEGRATOR_CLIENT_INTEGRATORSNAPSHOT_H
