#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_SIMVARS_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_SIMVARS_H

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
}

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_SIMVARS_H
