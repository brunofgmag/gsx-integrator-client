#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_GSXLVARS_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_GSXLVARS_H

namespace gsx::lvars
{
    inline constexpr auto kCouatlStarted = "FSDT_GSX_COUATL_STARTED";
    inline constexpr auto kBaggageLoaderFrontState = "FSDT_GSX_VEHICLE_BAGGAGELOADERFRONT_STATE";
    inline constexpr auto kBaggageLoaderRearState = "FSDT_GSX_VEHICLE_BAGGAGELOADERREAR_STATE";
    inline constexpr auto kBaggageLoaderMainState = "FSDT_GSX_VEHICLE_BAGGAGELOADERMAIN_STATE";
    inline constexpr auto kPassengerStairsFrontState = "FSDT_GSX_VEHICLE_PASSENGERSTAIRSFRONT_STATE";
    inline constexpr auto kPassengerStairsMiddleState = "FSDT_GSX_VEHICLE_PASSENGERSTAIRSMIDDLE_STATE";
    inline constexpr auto kPassengerStairsRearState = "FSDT_GSX_VEHICLE_PASSENGERSTAIRSREAR_STATE";
}

namespace gsx::states
{
    inline constexpr double kLoaderWaitingForDoor = 6.0;
    inline constexpr double kLoaderInPosition = 8.0;
    inline constexpr double kLoaderLoading = 9.0;
    inline constexpr double kLoaderRemoving = 4.0;
    inline constexpr double kStairsFinalPosition = 3.0;

    [[nodiscard]] inline bool IsLoaderPresent(const double state)
    {
        return state == kLoaderWaitingForDoor
            || state == kLoaderInPosition
            || state == kLoaderLoading
            || state == kLoaderRemoving;
    }
}

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_GSXLVARS_H
