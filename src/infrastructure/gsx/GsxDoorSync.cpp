#include "GsxDoorSync.h"

#include "GsxLVars.h"
#include "../probe/ProbeLog.h"
#include "../simvars/SimVars.h"
#include "../simvars/VariableGateway.h"

#include <algorithm>
#include <span>
#include <string>
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

    constexpr double kNoVehicleState = 0.0;

    struct DoorVehicle
    {
        const char* lVar;
        double absent;
        bool (*serves)(double state);
    };

    bool IsJetwayDocked(const double state)
    {
        return state == kJetwayDockedValue;
    }

    constexpr std::array kFwdPaxVehicles = {
        DoorVehicle{gsx::lvars::kJetway, kJetwayUnavailableValue, IsJetwayDocked},
        DoorVehicle{gsx::lvars::kPassengerStairsFrontState, kNoVehicleState, gsx::states::AreStairsArriving}
    };
    constexpr std::array kMidPaxVehicles = {
        DoorVehicle{gsx::lvars::kPassengerStairsMiddleState, kNoVehicleState, gsx::states::AreStairsArriving}
    };
    constexpr std::array kAftPaxVehicles = {
        DoorVehicle{gsx::lvars::kPassengerStairsRearState, kNoVehicleState, gsx::states::AreStairsArriving}
    };
    constexpr std::array kFwdCateringVehicles = {
        DoorVehicle{gsx::lvars::kCateringFrontState, kNoVehicleState, gsx::states::IsCateringArriving}
    };
    constexpr std::array kAftCateringVehicles = {
        DoorVehicle{gsx::lvars::kCateringRearState, kNoVehicleState, gsx::states::IsCateringArriving}
    };
    constexpr std::array kFwdCargoVehicles = {
        DoorVehicle{gsx::lvars::kBaggageLoaderFrontState, kNoVehicleState, gsx::states::IsLoaderServingTheDoor}
    };
    constexpr std::array kAftCargoVehicles = {
        DoorVehicle{gsx::lvars::kBaggageLoaderRearState, kNoVehicleState, gsx::states::IsLoaderServingTheDoor}
    };

    constexpr auto kMemoryPrefix = "doorSync.";
    constexpr auto kOpenText = "open";
    constexpr auto kClosedText = "closed";

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

    bool IsPassengerDoor(const GsxDoor door)
    {
        return door == GsxDoor::FwdPax || door == GsxDoor::MidPax || door == GsxDoor::AftPax;
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

    std::span<const DoorVehicle> DrivingVehicles(const GsxDoor door)
    {
        switch (door)
        {
        case GsxDoor::FwdPax: return kFwdPaxVehicles;
        case GsxDoor::MidPax: return kMidPaxVehicles;
        case GsxDoor::AftPax: return kAftPaxVehicles;
        case GsxDoor::FwdCatering: return kFwdCateringVehicles;
        case GsxDoor::AftCatering: return kAftCateringVehicles;
        case GsxDoor::FwdCargo: return kFwdCargoVehicles;
        default: return kAftCargoVehicles;
        }
    }

    std::string MemoryEntryName(const GsxDoor door)
    {
        return std::string(kMemoryPrefix).append(DoorName(door));
    }

    const char* TargetText(const double target)
    {
        return target == kDoorOpen ? kOpenText : kClosedText;
    }

    double TargetFromText(const std::string& text)
    {
        if (text == kOpenText)
        {
            return kDoorOpen;
        }

        return text == kClosedText ? kDoorClosed : kDoorUnknown;
    }
}

GsxDoorSync::GsxDoorSync(VariableReader* variableGateway) : variableGateway_(variableGateway)
{
    lastTargets_.fill(kDoorUnknown);
}

void GsxDoorSync::WatchExit(const GsxDoor door, const int exitIndex)
{
    exits_[static_cast<std::size_t>(door)] = ExitWatch{.index = exitIndex};
}

bool GsxDoorSync::IsMoving(const GsxDoor door) const
{
    return exits_[static_cast<std::size_t>(door)].moving;
}

void GsxDoorSync::Sync(const DoorWriter& write)
{
    Report();

    if (variableGateway_->GetLVar(gsx::lvars::kCouatlStarted, 0.0) < 1.0)
    {
        return;
    }

    for (const GsxDoor door : kAllDoors)
    {
        const auto index = static_cast<std::size_t>(door);
        const double lastTarget = lastTargets_[index];
        inheritedTargets_[index] = inheritedTargets_[index] && !HaveVehiclesArrived(door);

        const bool wantsOpen = IsDesiredOpen(door);
        const bool pending = wantsOpen ? lastTarget != kDoorOpen : lastTarget == kDoorOpen;
        const bool waitingForItsVehicles = !wantsOpen && inheritedTargets_[index] && !IsHeldClosed(door);
        if (!pending || waitingForItsVehicles)
        {
            continue;
        }

        if (IsMoving(door))
        {
            LogHoldOnce(door);

            continue;
        }

        probe::Line(probe::Channel::Writes,
                    QStringLiteral("write sync  %1 open=%2").arg(QLatin1String(DoorName(door))).arg(wantsOpen ? 1 : 0));
        write(door, wantsOpen);
        RecordOwnWrite(door, wantsOpen);
    }
}

void GsxDoorSync::LogHoldOnce(const GsxDoor door)
{
    bool& holdLogged = holdLogged_[static_cast<std::size_t>(door)];
    if (holdLogged)
    {
        return;
    }

    holdLogged = true;
    probe::Line(probe::Channel::Writes,
                QStringLiteral("hold  sync  %1 moving").arg(QLatin1String(DoorName(door))));
}

void GsxDoorSync::RecordOwnWrite(const GsxDoor door, const bool open)
{
    const auto index = static_cast<std::size_t>(door);

    lastTargets_[index] = open ? kDoorOpen : kDoorClosed;
    holdLogged_[index] = false;
    inheritedTargets_[index] = false;
}

void GsxDoorSync::Observe()
{
    SampleExits();

    const bool started = variableGateway_->GetLVar(gsx::lvars::kCouatlStarted, 0.0) >= 1.0;

    if (!started)
    {
        couatlRestarting_ = couatlSeenStarted_;

        return;
    }

    if (couatlRestarting_)
    {
        couatlRestarting_ = false;
        DistrustEveryVehicle();
        probe::Line(probe::Channel::Client, QStringLiteral("gsx couatl restarted, vehicle states distrusted"));
    }

    InheritArrivedVehicles();
    couatlSeenStarted_ = true;
}

void GsxDoorSync::AppendMemory(MemoryBag& memory) const
{
    for (const GsxDoor door : kAllDoors)
    {
        const double target = lastTargets_[static_cast<std::size_t>(door)];
        if (target != kDoorUnknown)
        {
            memory.PutText(MemoryEntryName(door), TargetText(target));
        }
    }
}

void GsxDoorSync::RestoreMemory(const MemoryBag& memory, const bool gsxRestartedSinceSave)
{
    for (const GsxDoor door : kAllDoors)
    {
        const auto index = static_cast<std::size_t>(door);

        lastTargets_[index] = TargetFromText(memory.Text(MemoryEntryName(door), {}));
        inheritedTargets_[index] = lastTargets_[index] == kDoorOpen;
    }

    if (gsxRestartedSinceSave)
    {
        DistrustEveryVehicle();

        return;
    }

    inheritedVehicles_.clear();
    awaitingInheritance_.clear();
}

void GsxDoorSync::DistrustEveryVehicle()
{
    inheritedVehicles_.clear();
    awaitingInheritance_ = std::set<std::string>(kVehicleLVars.begin(), kVehicleLVars.end());
    InheritArrivedVehicles();
}

void GsxDoorSync::InheritArrivedVehicles() const
{
    std::erase_if(awaitingInheritance_, [this](const std::string& lVar)
    {
        if (!variableGateway_->HasReceivedLVar(lVar))
        {
            return false;
        }

        inheritedVehicles_.emplace(lVar, variableGateway_->GetLVar(lVar, 0.0));

        return true;
    });
}

void GsxDoorSync::SampleExits()
{
    for (ExitWatch& exit : exits_)
    {
        if (exit.index < 0)
        {
            continue;
        }

        const std::string name = simvars::SimExitOpen(exit.index);
        if (!variableGateway_->HasReceivedAVar(name, simvars::kPercentUnit))
        {
            exit.moving = false;
            continue;
        }

        const double position = variableGateway_->GetAVar(name, simvars::kPercentUnit);
        exit.moving = exit.sampled && position != exit.lastPosition;
        exit.lastPosition = position;
        exit.sampled = true;
    }
}

double GsxDoorSync::VehicleState(const char* lVar, const double absent) const
{
    InheritArrivedVehicles();

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

void GsxDoorSync::Report() const
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

    static constexpr std::array kCandidates = {
        gsx::lvars::kLoaderExit0,
        gsx::lvars::kLoaderExit1,
        gsx::lvars::kLoaderExit2,
        gsx::lvars::kOperateStairsState,
        gsx::lvars::kOperateJetwaysState,
        gsx::lvars::kStairs,
        gsx::lvars::kJetwayAir,
        gsx::lvars::kJetwayPower,
        gsx::lvars::kSetLoadersStayUntilDeparture,
        gsx::lvars::kSetAutoStairs,
        gsx::lvars::kSetDisableRearStairs
    };

    QStringList candidates;
    for (const char* name : kCandidates)
    {
        candidates.append(QStringLiteral("%1=%2").arg(QLatin1String(name))
                          .arg(variableGateway_->GetLVar(name, -1.0), 0, 'f', 1));
    }

    probe::Change(probe::Channel::GsxLVars, "gsx.candidates",
                  QStringLiteral("gsxc  %1").arg(candidates.join(QLatin1Char(' '))));

    probe::Change(probe::Channel::GsxLVars, "gsx.doorsync",
                  QStringLiteral("dsync %1 | %2")
                  .arg(values.join(QLatin1Char(' ')), wanted.join(QLatin1Char(' '))));
}

void GsxDoorSync::CloseAll(const DoorWriter& write)
{
    for (const GsxDoor door : kAllDoors)
    {
        probe::Line(probe::Channel::Writes,
                    QStringLiteral("write close %1 open=0").arg(QLatin1String(DoorName(door))));
        write(door, false);
        RecordOwnWrite(door, false);
    }
}

void GsxDoorSync::HoldClosedForDeparture(const bool hold)
{
    heldForDeparture_ = hold;
    if (!hold)
    {
        passengerDoorsHeld_ = false;
    }
}

void GsxDoorSync::HoldPassengerDoorsClosed(const bool hold)
{
    passengerDoorsHeld_ = hold;
}

bool GsxDoorSync::IsHeldClosed(const GsxDoor door) const
{
    return (heldForDeparture_ && IsHeldForDeparture(door)) || (passengerDoorsHeld_ && IsPassengerDoor(door));
}

bool GsxDoorSync::HaveVehiclesArrived(const GsxDoor door) const
{
    return std::ranges::all_of(DrivingVehicles(door), [this](const DoorVehicle& vehicle)
    {
        return variableGateway_->HasReceivedLVar(vehicle.lVar);
    });
}

bool GsxDoorSync::IsDesiredOpen(const GsxDoor door) const
{
    if (IsHeldClosed(door))
    {
        return false;
    }

    return std::ranges::any_of(DrivingVehicles(door), [this](const DoorVehicle& vehicle)
    {
        return vehicle.serves(VehicleState(vehicle.lVar, vehicle.absent));
    });
}
