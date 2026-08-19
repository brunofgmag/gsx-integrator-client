#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_SIMVARS_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_SIMVARS_H

#include <array>
#include <cstddef>

#include "VariableGateway.h"

namespace simvars
{
    inline constexpr auto kKgUnit = "kg";
    inline constexpr auto kBoolUnit = "Bool";

    inline constexpr auto kSimFuelTotalKg = "FUEL TOTAL QUANTITY WEIGHT";
    inline constexpr auto kSimTotalWeight = "TOTAL WEIGHT";
    inline constexpr auto kSimEmptyWeight = "EMPTY WEIGHT";

    inline constexpr auto kSimParkingBrake = "BRAKE PARKING POSITION";
    inline constexpr auto kSimBeaconLight = "LIGHT BEACON";

    inline constexpr auto kSimEng1Combustion = "ENG COMBUSTION:1";
    inline constexpr auto kSimEng2Combustion = "ENG COMBUSTION:2";
    inline constexpr auto kSimEng3Combustion = "ENG COMBUSTION:3";
    inline constexpr auto kSimEng4Combustion = "ENG COMBUSTION:4";

    inline double EmptyZfwKg(VariableGateway& variables)
    {
        return variables.GetAVar(kSimEmptyWeight, kKgUnit, 0.0);
    }

    inline double CurrentFuelKg(VariableGateway& variables)
    {
        return variables.GetAVar(kSimFuelTotalKg, kKgUnit, 0.0);
    }

    inline double CurrentZfwKg(VariableGateway& variables)
    {
        const double emptyZfwKg = EmptyZfwKg(variables);
        const double totalWeightKg = variables.GetAVar(kSimTotalWeight, kKgUnit, emptyZfwKg);
        const double zfwKg = totalWeightKg - CurrentFuelKg(variables);

        return zfwKg < emptyZfwKg ? emptyZfwKg : zfwKg;
    }

    inline bool AnyEngineCombusting(VariableGateway& variables, const double defaultValue,
                                    const int engineCount)
    {
        constexpr std::array kEngineCombustion =
            {kSimEng1Combustion, kSimEng2Combustion, kSimEng3Combustion, kSimEng4Combustion};

        for (std::size_t engine = 0; engine < static_cast<std::size_t>(engineCount); ++engine)
        {
            if (variables.GetAVar(kEngineCombustion[engine], kBoolUnit, defaultValue) > 0.0)
            {
                return true;
            }
        }

        return false;
    }
}

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_SIMVARS_H
