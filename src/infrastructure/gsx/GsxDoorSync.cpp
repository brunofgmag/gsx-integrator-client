#include "GsxDoorSync.h"

#include "GsxLVars.h"
#include "../probe/ProbeLog.h"
#include "../simvars/VariableGateway.h"

#include <algorithm>
#include <QtCore/QString>
#include <QtCore/QStringList>

namespace
{
    constexpr double kDoorUnknown = -1.0;
    constexpr double kDoorOpen = 1.0;
    constexpr double kDoorClosed = 0.0;

    constexpr double kJetwayDockedValue = 5.0;
    constexpr double kJetwayUnavailableValue = 2.0;

    constexpr std::array kVehicleLVars = {
        gsx::lvars::kJetway,
        gsx::lvars::kPassengerStairsFrontState,
        gsx::lvars::kPassengerStairsMiddleState,
        gsx::lvars::kPassengerStairsRearState,
        gsx::lvars::kCateringFrontState,
        gsx::lvars::kCateringRearState,
        gsx::lvars::kBaggageLoaderFrontState,
        gsx::lvars::kBaggageLoaderRearState,
        gsx::lvars::kBaggageLoaderMainState
    };

    constexpr std::array kAllDoors = {
        GsxDoor::FwdPax, GsxDoor::MidPax, GsxDoor::AftPax,
        GsxDoor::FwdCatering, GsxDoor::AftCatering,
        GsxDoor::FwdCargo, GsxDoor::AftCargo
    };

    constexpr std::array kDoorsHeldForDeparture = {
        GsxDoor::FwdPax, GsxDoor::MidPax, GsxDoor::AftPax,
        GsxDoor::FwdCargo, GsxDoor::AftCargo
    };

    bool IsHeldForDeparture(const GsxDoor door)
    {
        return std::ranges::find(kDoorsHeldForDeparture, door) != kDoorsHeldForDeparture.end();
    }

    const char* DoorName(const GsxDoor door)
    {
        switch (door)
        {
        case GsxDoor::FwdPax: return "FwdPax";
        case GsxDoor::MidPax: return "MidPax";
        case GsxDoor::AftPax: return "AftPax";
        case GsxDoor::FwdCatering: return "FwdCatering";
        case GsxDoor::AftCatering: return "AftCatering";
        case GsxDoor::FwdCargo: return "FwdCargo";
        default: return "AftCargo";
        }
    }
}

GsxDoorSync::GsxDoorSync(VariableGateway* variableGateway) : variableGateway_(variableGateway)
{
    lastTargets_.fill(kDoorUnknown);
}

void GsxDoorSync::Sync(const DoorWriter& write)
{
    ReportProbe();
    ObserveCouatl();

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
                probe::Line(QStringLiteral("write sync  %1 open=1").arg(QLatin1String(DoorName(door))));
                write(door, true);
                lastTarget = kDoorOpen;
            }
        }
        else if (lastTarget == kDoorOpen)
        {
            probe::Line(QStringLiteral("write sync  %1 open=0").arg(QLatin1String(DoorName(door))));
            write(door, false);
            lastTarget = kDoorClosed;
        }
    }
}

void GsxDoorSync::ObserveCouatl()
{
    const bool started = variableGateway_->GetLVar(gsx::lvars::kCouatlStarted, 0.0) >= 1.0;

    if (!started)
    {
        couatlRestarting_ = couatlSeenStarted_;

        return;
    }

    if (couatlRestarting_)
    {
        couatlRestarting_ = false;
        inheritedVehicles_.clear();
        for (const char* lVar : kVehicleLVars)
        {
            inheritedVehicles_.emplace(lVar, variableGateway_->GetLVar(lVar, 0.0));
        }
        probe::Line(QStringLiteral("gsx couatl restarted, vehicle states distrusted"));
    }

    couatlSeenStarted_ = true;
}

double GsxDoorSync::VehicleState(const char* lVar, const double absent) const
{
    const double value = variableGateway_->GetLVar(lVar, absent);

    const auto inherited = inheritedVehicles_.find(lVar);
    if (inherited == inheritedVehicles_.end())
    {
        return value;
    }

    if (inherited->second != value)
    {
        inheritedVehicles_.erase(inherited);

        return value;
    }

    return absent;
}

void GsxDoorSync::ReportProbe() const
{
    if (!probe::IsOn())
    {
        return;
    }

    static constexpr std::array kInputs = {
        gsx::lvars::kCouatlStarted,
        gsx::lvars::kJetway,
        gsx::lvars::kPassengerStairsFrontState,
        gsx::lvars::kPassengerStairsMiddleState,
        gsx::lvars::kPassengerStairsRearState,
        gsx::lvars::kCateringFrontState,
        gsx::lvars::kCateringRearState,
        gsx::lvars::kBaggageLoaderFrontState,
        gsx::lvars::kBaggageLoaderRearState,
        gsx::lvars::kBaggageLoaderMainState
    };

    QStringList values;
    for (const char* name : kInputs)
    {
        values.append(QStringLiteral("%1=%2").arg(QLatin1String(name))
                      .arg(variableGateway_->GetLVar(name, -1.0), 0, 'f', 1));
    }

    QStringList wanted;
    for (const GsxDoor door : kAllDoors)
    {
        wanted.append(QStringLiteral("%1=%2").arg(QLatin1String(DoorName(door)))
                      .arg(IsDesiredOpen(door) ? 1 : 0));
    }

    static constexpr std::array kUnknownToTheClient = {
        "FSDT_GSX_LOADER_EXIT_0",
        "FSDT_GSX_LOADER_EXIT_1",
        "FSDT_GSX_LOADER_EXIT_2",
        "FSDT_GSX_OPERATESTAIRS_STATE",
        "FSDT_GSX_OPERATEJETWAYS_STATE",
        "FSDT_GSX_STAIRS",
        "FSDT_GSX_JETWAY_AIR",
        "FSDT_GSX_JETWAY_POWER",
        "FSDT_GSX_SET_LOADERS_STAY_UNTIL_DEPARTURE",
        "FSDT_GSX_SET_AUTO_STAIRS",
        "FSDT_GSX_SET_DISABLE_REAR_STAIRS"
    };

    QStringList candidates;
    for (const char* name : kUnknownToTheClient)
    {
        candidates.append(QStringLiteral("%1=%2").arg(QLatin1String(name))
                          .arg(variableGateway_->GetLVar(name, -1.0), 0, 'f', 1));
    }

    probe::Change("gsx.candidates", QStringLiteral("gsxc  %1").arg(candidates.join(QLatin1Char(' '))));

    probe::Change("gsx.doorsync", QStringLiteral("dsync %1 | %2")
                  .arg(values.join(QLatin1Char(' ')), wanted.join(QLatin1Char(' '))));
}

void GsxDoorSync::CloseAll(const DoorWriter& write)
{
    for (const GsxDoor door : kAllDoors)
    {
        probe::Line(QStringLiteral("write close %1 open=0").arg(QLatin1String(DoorName(door))));
        write(door, false);
        lastTargets_[static_cast<std::size_t>(door)] = kDoorClosed;
    }
}

void GsxDoorSync::HoldClosedForDeparture(const bool hold)
{
    heldForDeparture_ = hold;
}

bool GsxDoorSync::IsDesiredOpen(const GsxDoor door) const
{
    if (heldForDeparture_ && IsHeldForDeparture(door))
    {
        return false;
    }

    const auto vehicleState = [this](const char* lVar)
    {
        return VehicleState(lVar, 0.0);
    };

    switch (door)
    {
    case GsxDoor::FwdPax:
        return VehicleState(gsx::lvars::kJetway, kJetwayUnavailableValue) == kJetwayDockedValue
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
        return gsx::states::IsLoaderAtDoor(vehicleState(gsx::lvars::kBaggageLoaderFrontState));
    default:
        return gsx::states::IsLoaderAtDoor(vehicleState(gsx::lvars::kBaggageLoaderRearState));
    }
}
