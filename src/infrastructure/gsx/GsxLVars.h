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

    inline constexpr auto kRefuelingState = "FSDT_GSX_REFUELING_STATE";
    inline constexpr auto kFuelHoseConnected = "FSDT_GSX_FUELHOSE_CONNECTED";
    inline constexpr auto kFuelCounter = "FSDT_GSX_FUEL_COUNTER";
    inline constexpr auto kFuelCounterMax = "FSDT_GSX_FUEL_COUNTER_MAX";
    inline constexpr auto kBoardingState = "FSDT_GSX_BOARDING_STATE";
    inline constexpr auto kDeboardingState = "FSDT_GSX_DEBOARDING_STATE";
    inline constexpr auto kPushbackVehicleState = "FSDT_GSX_VEHICLE_PUSHBACK_STATE";
    inline constexpr auto kPushbackStatus = "FSDT_GSX_PUSHBACK_STATUS";

    inline constexpr auto kMaxPassengers = "FSDT_GSX_MAX_NUMPASSENGERS";
    inline constexpr auto kNumPassengersBoardingTotal = "FSDT_GSX_NUMPASSENGERS_BOARDING_TOTAL";
    inline constexpr auto kNumPassengersDeboardingTotal = "FSDT_GSX_NUMPASSENGERS_DEBOARDING_TOTAL";
    inline constexpr auto kBoardingCargoPercent = "FSDT_GSX_BOARDING_CARGO_PERCENT";
    inline constexpr auto kDeboardingCargoPercent = "FSDT_GSX_DEBOARDING_CARGO_PERCENT";

    inline constexpr auto kAutomationFuel = "FSDT_GSX_AUTOMATION_FUEL";
    inline constexpr auto kAutomationPayload = "FSDT_GSX_AUTOMATION_PAYLOAD";

    inline constexpr auto kSimbriefSuccess = "FSDT_GSX_SIMBRIEF_SUCCESS";

    inline constexpr auto kJetway = "FSDT_GSX_JETWAY";
    inline constexpr auto kStairs = "FSDT_GSX_STAIRS";
    inline constexpr auto kRepositioning = "FSDT_GSX_REPOSITIONING";
    inline constexpr auto kGpuConnected = "FSDT_GSX_GPU_CONNECTED";

    inline constexpr auto kGoodEngineStart = "FSDT_GSX_SETTINGS_GOOD_ENGINE_START";
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
