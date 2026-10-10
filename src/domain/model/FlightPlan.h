#ifndef GSX_INTEGRATOR_CLIENT_DOMAIN_FLIGHTPLAN_H
#define GSX_INTEGRATOR_CLIENT_DOMAIN_FLIGHTPLAN_H

#include <cstdint>
#include <optional>
#include <string>
#include "../support/Weight.h"

enum class FlightPlanStatus : std::uint8_t
{
    Idle = 0,
    Fetching = 1,
    Ready = 2,
    Error = 3,
};

enum class FlightPlanFailure : std::uint8_t
{
    None = 0,
    NotSent = 1,
    Http = 2,
    Parse = 3,
};

struct FlightPlan
{
    double fuelKg = 0.0;
    double zfwKg = 0.0;
    int passengers = 0;
    WeightUnit unit = WeightUnit::Kg;
    std::string origin;
    std::string destination;
    long long generatedEpoch = 0;
    double operatingEmptyKg = 0.0;
    std::optional<double> payloadKg;
    std::optional<double> cargoKg;

    bool operator==(const FlightPlan&) const = default;
};

#endif // GSX_INTEGRATOR_CLIENT_DOMAIN_FLIGHTPLAN_H
