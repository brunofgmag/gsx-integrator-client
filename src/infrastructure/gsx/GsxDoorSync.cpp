#include "GsxDoorSync.h"

#include "GsxLVars.h"
#include "../simvars/VariableGateway.h"

namespace
{
    constexpr double kDoorUnknown = -1.0;
    constexpr double kDoorOpen = 1.0;
    constexpr double kDoorClosed = 0.0;

    constexpr double kJetwayDockedValue = 5.0;
    constexpr double kJetwayUnavailableValue = 2.0;

    constexpr std::array kAllDoors = {
        GsxDoor::FwdPax, GsxDoor::MidPax, GsxDoor::AftPax,
        GsxDoor::FwdCatering, GsxDoor::AftCatering,
        GsxDoor::FwdCargo, GsxDoor::AftCargo
    };
}

GsxDoorSync::GsxDoorSync(VariableGateway* variableGateway) : variableGateway_(variableGateway)
{
    lastTargets_.fill(kDoorUnknown);
}

void GsxDoorSync::Sync(const DoorWriter& write)
{
    if (variableGateway_->GetLVar(gsx::lvars::kCouatlStarted, 0.0) < 1.0)
    {
        return;
    }

    for (const GsxDoor door : kAllDoors)
    {
        double& lastTarget = lastTargets_[static_cast<std::size_t>(door)];
        if (IsDesiredOpen(door))
        {
            if (lastTarget != kDoorOpen)
            {
                write(door, true);
                lastTarget = kDoorOpen;
            }
        }
        else if (lastTarget == kDoorOpen)
        {
            write(door, false);
            lastTarget = kDoorClosed;
        }
    }
}

void GsxDoorSync::CloseAll(const DoorWriter& write)
{
    for (const GsxDoor door : kAllDoors)
    {
        write(door, false);
        lastTargets_[static_cast<std::size_t>(door)] = kDoorClosed;
    }
}

bool GsxDoorSync::IsDesiredOpen(const GsxDoor door) const
{
    const auto vehicleState = [this](const char* lVar)
    {
        return variableGateway_->GetLVar(lVar, 0.0);
    };

    switch (door)
    {
    case GsxDoor::FwdPax:
        return variableGateway_->GetLVar(gsx::lvars::kJetway, kJetwayUnavailableValue) == kJetwayDockedValue
            || vehicleState(gsx::lvars::kPassengerStairsFrontState) == gsx::states::kStairsFinalPosition;
    case GsxDoor::MidPax:
        return vehicleState(gsx::lvars::kPassengerStairsMiddleState) == gsx::states::kStairsFinalPosition;
    case GsxDoor::AftPax:
        return vehicleState(gsx::lvars::kPassengerStairsRearState) == gsx::states::kStairsFinalPosition;
    case GsxDoor::FwdCatering:
        return gsx::states::IsCateringAtDoor(vehicleState(gsx::lvars::kCateringFrontState));
    case GsxDoor::AftCatering:
        return gsx::states::IsCateringAtDoor(vehicleState(gsx::lvars::kCateringRearState));
    case GsxDoor::FwdCargo:
        return gsx::states::IsLoaderPresent(vehicleState(gsx::lvars::kBaggageLoaderFrontState));
    default:
        return gsx::states::IsLoaderPresent(vehicleState(gsx::lvars::kBaggageLoaderRearState));
    }
}
