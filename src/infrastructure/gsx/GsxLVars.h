#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_GSXLVARS_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_GSXLVARS_H

#include <string_view>

#include "../../domain/ports/GsxGateway.h"

namespace gsx::services
{
    [[nodiscard]] inline const char* Id(const GroundService service)
    {
        switch (service)
        {
        case GroundService::Catering: return "Catering";
        case GroundService::Lavatory: return "Lavatory";
        case GroundService::Water: return "Water";
        case GroundService::Cleaning: return "Cleaning";
        case GroundService::Gpu: return "GPU";
        case GroundService::Departure: return "Departure";
        default: return nullptr;
        }
    }

    [[nodiscard]] inline bool ASecondPickUndoesIt(const std::string_view serviceId)
    {
        return serviceId == "OperateStairs"
            || serviceId == "OperateJetways"
            || serviceId == "GPU";
    }
}

namespace gsx::lvars
{
    inline constexpr auto kCouatlStarted = "FSDT_GSX_COUATL_STARTED";
    inline constexpr auto kBaggageLoaderFrontState = "FSDT_GSX_VEHICLE_BAGGAGELOADERFRONT_STATE";
    inline constexpr auto kBaggageLoaderRearState = "FSDT_GSX_VEHICLE_BAGGAGELOADERREAR_STATE";
    inline constexpr auto kBaggageLoaderMainState = "FSDT_GSX_VEHICLE_BAGGAGELOADERMAIN_STATE";
    inline constexpr auto kPassengerStairsFrontState = "FSDT_GSX_VEHICLE_PASSENGERSTAIRSFRONT_STATE";
    inline constexpr auto kPassengerStairsMiddleState = "FSDT_GSX_VEHICLE_PASSENGERSTAIRSMIDDLE_STATE";
    inline constexpr auto kPassengerStairsRearState = "FSDT_GSX_VEHICLE_PASSENGERSTAIRSREAR_STATE";
    inline constexpr auto kCateringFrontState = "FSDT_GSX_VEHICLE_CATERINGVEHICLEFRONT_STATE";
    inline constexpr auto kCateringRearState = "FSDT_GSX_VEHICLE_CATERINGVEHICLEREAR_STATE";

    inline constexpr auto kRefuelingState = "FSDT_GSX_REFUELING_STATE";
    inline constexpr auto kFuelHoseConnected = "FSDT_GSX_FUELHOSE_CONNECTED";
    inline constexpr auto kFuelCounter = "FSDT_GSX_FUEL_COUNTER";
    inline constexpr auto kFuelCounterMax = "FSDT_GSX_FUEL_COUNTER_MAX";
    inline constexpr auto kBoardingState = "FSDT_GSX_BOARDING_STATE";
    inline constexpr auto kDeboardingState = "FSDT_GSX_DEBOARDING_STATE";
    inline constexpr auto kPushbackVehicleState = "FSDT_GSX_VEHICLE_PUSHBACK_STATE";
    inline constexpr auto kPushbackStatus = "FSDT_GSX_PUSHBACK_STATUS";
    inline constexpr auto kDeiceState = "FSDT_GSX_DEICE_STATE";

    inline constexpr auto kMaxPassengers = "FSDT_GSX_MAX_NUMPASSENGERS";
    inline constexpr auto kNumPassengersBoardingTotal = "FSDT_GSX_NUMPASSENGERS_BOARDING_TOTAL";
    inline constexpr auto kNumPassengersDeboardingTotal = "FSDT_GSX_NUMPASSENGERS_DEBOARDING_TOTAL";
    inline constexpr auto kBoardingCargo = "FSDT_GSX_BOARDING_CARGO";
    inline constexpr auto kBoardingCargoPercent = "FSDT_GSX_BOARDING_CARGO_PERCENT";
    inline constexpr auto kDeboardingCargoPercent = "FSDT_GSX_DEBOARDING_CARGO_PERCENT";

    inline constexpr auto kAutomationFuel = "FSDT_GSX_AUTOMATION_FUEL";
    inline constexpr auto kAutomationPayload = "FSDT_GSX_AUTOMATION_PAYLOAD";
    inline constexpr auto kAutomationDoors = "FSDT_GSX_AUTOMATION_DOORS";

    inline constexpr auto kSimbriefSuccess = "FSDT_GSX_SIMBRIEF_SUCCESS";

    inline constexpr auto kAircraftCargo1Toggle = "FSDT_GSX_AIRCRAFT_CARGO_1_TOGGLE";
    inline constexpr auto kAircraftCargo2Toggle = "FSDT_GSX_AIRCRAFT_CARGO_2_TOGGLE";
    inline constexpr auto kAircraftExit1Toggle = "FSDT_GSX_AIRCRAFT_EXIT_1_TOGGLE";
    inline constexpr auto kAircraftExit4Toggle = "FSDT_GSX_AIRCRAFT_EXIT_4_TOGGLE";
    inline constexpr auto kAircraftService1Toggle = "FSDT_GSX_AIRCRAFT_SERVICE_1_TOGGLE";
    inline constexpr auto kAircraftService2Toggle = "FSDT_GSX_AIRCRAFT_SERVICE_2_TOGGLE";

    inline constexpr auto kJetway = "FSDT_GSX_JETWAY";
    inline constexpr auto kOperateJetwaysState = "FSDT_GSX_OPERATEJETWAYS_STATE";
    inline constexpr auto kStairs = "FSDT_GSX_STAIRS";
    inline constexpr auto kRepositioning = "FSDT_GSX_REPOSITIONING";
    inline constexpr auto kGpuConnected = "FSDT_GSX_GPU_CONNECTED";
    inline constexpr auto kGpuState = "FSDT_GSX_GPU_STATE";

    inline constexpr auto kGoodEngineStart = "FSDT_GSX_SETTINGS_GOOD_ENGINE_START";
}

namespace gsx::states
{
    inline constexpr double kVehicleDispatched = 2.0;
    inline constexpr double kVehicleApproaching = 5.0;
    inline constexpr double kLoaderWaitingForDoor = 6.0;
    inline constexpr double kLoaderUnloading = 7.0;
    inline constexpr double kLoaderInPosition = 8.0;
    inline constexpr double kLoaderLoading = 9.0;
    inline constexpr double kLoaderFinishing = 10.0;
    inline constexpr double kLoaderRetracting = 11.0;
    inline constexpr double kLoaderRemoving = 4.0;
    inline constexpr double kStairsWaitingForDoor = 6.0;
    inline constexpr double kStairsFinalPosition = 3.0;
    inline constexpr double kCateringWaitingForDoor = 6.0;
    inline constexpr double kCateringInProgress = 7.0;

    [[nodiscard]] inline bool IsLoaderAtDoor(const double state)
    {
        return state == kLoaderWaitingForDoor
            || state == kLoaderUnloading
            || state == kLoaderInPosition
            || state == kLoaderLoading
            || state == kLoaderFinishing;
    }

    [[nodiscard]] inline bool IsLoaderPresent(const double state)
    {
        return IsLoaderAtDoor(state) || state == kLoaderRetracting || state == kLoaderRemoving;
    }

    [[nodiscard]] inline bool IsCateringAtDoor(const double state)
    {
        return state == kCateringWaitingForDoor
            || state == kCateringInProgress;
    }

    [[nodiscard]] inline bool IsLoaderArriving(const double state)
    {
        return IsLoaderAtDoor(state);
    }

    [[nodiscard]] inline bool IsCateringArriving(const double state)
    {
        return state == kVehicleApproaching || IsCateringAtDoor(state);
    }

    [[nodiscard]] inline bool AreStairsArriving(const double state)
    {
        return state == kVehicleApproaching
            || state == kStairsWaitingForDoor
            || state == kStairsFinalPosition;
    }

    [[nodiscard]] inline bool AreStairsDocked(const double state)
    {
        return state == kStairsFinalPosition;
    }
}

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_GSXLVARS_H
