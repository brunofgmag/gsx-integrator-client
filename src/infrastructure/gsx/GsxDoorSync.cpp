#include "GsxDoorSync.h"

#include "GsxLVars.h"
#include "../logging/LogMacros.h"
#include "../probe/ProbeLog.h"
#include "../simvars/SimVars.h"
#include "../simvars/VariableGateway.h"

#include <algorithm>
#include <span>
#include <string>
#include <utility>
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
        gsx::lvars::kBaggageLoaderMainState,
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
        DoorVehicle{.lVar = gsx::lvars::kJetway, .absent = kJetwayUnavailableValue, .serves = IsJetwayDocked},
        DoorVehicle{.lVar = gsx::lvars::kPassengerStairsFrontState, .absent = kNoVehicleState, .serves = gsx::states::AreStairsArriving},
    };
    constexpr std::array kMidPaxVehicles = {
        DoorVehicle{.lVar = gsx::lvars::kPassengerStairsMiddleState, .absent = kNoVehicleState, .serves = gsx::states::AreStairsArriving},
    };
    constexpr std::array kAftPaxVehicles = {
        DoorVehicle{.lVar = gsx::lvars::kPassengerStairsRearState, .absent = kNoVehicleState, .serves = gsx::states::AreStairsArriving},
    };
    constexpr std::array kFwdCateringVehicles = {
        DoorVehicle{.lVar = gsx::lvars::kCateringFrontState, .absent = kNoVehicleState, .serves = gsx::states::IsCateringArriving},
    };
    constexpr std::array kAftCateringVehicles = {
        DoorVehicle{.lVar = gsx::lvars::kCateringRearState, .absent = kNoVehicleState, .serves = gsx::states::IsCateringArriving},
    };
    constexpr std::array kFwdCargoVehicles = {
        DoorVehicle{.lVar = gsx::lvars::kBaggageLoaderFrontState, .absent = kNoVehicleState, .serves = gsx::states::IsLoaderServingTheDoor},
    };
    constexpr std::array kAftCargoVehicles = {
        DoorVehicle{.lVar = gsx::lvars::kBaggageLoaderRearState, .absent = kNoVehicleState, .serves = gsx::states::IsLoaderServingTheDoor},
    };

    constexpr auto kMemoryPrefix = "doorSync.";
    constexpr auto kOpenText = "open";
    constexpr auto kClosedText = "closed";

    constexpr std::array kAllDoors = {
        GsxDoor::FwdPax, GsxDoor::MidPax, GsxDoor::AftPax,
        GsxDoor::FwdCatering, GsxDoor::AftCatering,
        GsxDoor::FwdCargo, GsxDoor::AftCargo,
    };

    constexpr std::array kDoorsHeldForDeparture = {
        GsxDoor::FwdPax, GsxDoor::MidPax, GsxDoor::AftPax,
        GsxDoor::FwdCargo, GsxDoor::AftCargo,
    };

    bool IsHeldForDeparture(const GsxDoor door)
    {
        return std::ranges::find(kDoorsHeldForDeparture, door) != kDoorsHeldForDeparture.end();
    }

    bool IsPlaced(const DoorPosition position)
    {
        return position == DoorPosition::Open || position == DoorPosition::Closed;
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

void DoorTransit::Begin()
{
    ticks_ = 0;
}

bool DoorTransit::IsRunning() const
{
    return ticks_ != kIdle;
}

bool DoorTransit::Holds(const DoorPosition reading, const DoorPosition target) const
{
    return IsRunning() && IsPlaced(reading) && IsPlaced(target) && reading != target;
}

bool DoorTransit::Age(const DoorPosition reading, const DoorPosition target)
{
    if (!IsRunning())
    {
        return false;
    }

    if (reading == target)
    {
        ticks_ = kIdle;

        return false;
    }

    if (++ticks_ <= kBoundTicks)
    {
        return false;
    }

    ticks_ = kIdle;

    return IsPlaced(reading) && IsPlaced(target);
}

GsxDoorSync::GsxDoorSync(VariableReader* variableGateway) : variableGateway_(variableGateway)
{
    lastTargets_.fill(kDoorUnknown);
}

void GsxDoorSync::WatchExit(const GsxDoor door, const int exitIndex)
{
    exits_[static_cast<std::size_t>(door)] = ExitWatch{.index = exitIndex};
}

void GsxDoorSync::ReadPositionsWith(PositionReader reader)
{
    positionReader_ = std::move(reader);
}

bool GsxDoorSync::IsMoving(const GsxDoor door) const
{
    return exits_[static_cast<std::size_t>(door)].moving;
}

DoorPosition GsxDoorSync::PositionOf(const GsxDoor door) const
{
    return positionReader_ ? positionReader_(door) : DoorPosition::NotRead;
}

DoorPosition GsxDoorSync::OwnTargetPosition(const GsxDoor door) const
{
    const double target = lastTargets_[static_cast<std::size_t>(door)];
    if (target == kDoorOpen)
    {
        return DoorPosition::Open;
    }

    return target == kDoorClosed ? DoorPosition::Closed : DoorPosition::NotRead;
}

bool GsxDoorSync::IsInTransit(const GsxDoor door, const DoorPosition position) const
{
    return transits_[static_cast<std::size_t>(door)].Holds(position, OwnTargetPosition(door));
}

void GsxDoorSync::AgeTransit(const GsxDoor door)
{
    DoorTransit& transit = transits_[static_cast<std::size_t>(door)];
    if (!transit.IsRunning())
    {
        return;
    }

    const bool expiredUnobeyed = transit.Age(PositionOf(door), OwnTargetPosition(door));
    if (!expiredUnobeyed)
    {
        return;
    }

    probe::Line(probe::Channel::Writes,
                QStringLiteral("hold  sync  %1 transit over after %2 ticks, reading believed")
                .arg(QLatin1String(DoorName(door))).arg(DoorTransit::kBoundTicks));
    LOG_INFO("Door %s did not reach its commanded position in %d ticks: its reading is believed from now on",
             DoorName(door), DoorTransit::kBoundTicks);
}

bool GsxDoorSync::SettlesWithoutWriting(const GsxDoor door, const bool wantsOpen)
{
    const DoorPosition position = PositionOf(door);
    if (IsInTransit(door, position))
    {
        LogHoldOnce(door, "in transit");

        return true;
    }

    if (position == DoorPosition::Pending)
    {
        LogHoldOnce(door, "unread");

        return true;
    }

    if (position != (wantsOpen ? DoorPosition::Open : DoorPosition::Closed))
    {
        return false;
    }

    probe::Line(probe::Channel::Writes,
                QStringLiteral("skip  sync  %1 open=%2 already there").arg(QLatin1String(DoorName(door))).arg(wantsOpen ? 1 : 0));
    RecordTarget(door, wantsOpen);

    return true;
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
        AgeTransit(door);

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
            LogHoldOnce(door, "moving");

            continue;
        }

        if (SettlesWithoutWriting(door, wantsOpen))
        {
            continue;
        }

        probe::Line(probe::Channel::Writes,
                    QStringLiteral("write sync  %1 open=%2").arg(QLatin1String(DoorName(door))).arg(wantsOpen ? 1 : 0));
        write(door, wantsOpen);
        RecordWrite(door, wantsOpen);
    }
}

void GsxDoorSync::LogHoldOnce(const GsxDoor door, const char* reason)
{
    bool& holdLogged = holdLogged_[static_cast<std::size_t>(door)];
    if (holdLogged)
    {
        return;
    }

    holdLogged = true;
    probe::Line(probe::Channel::Writes,
                QStringLiteral("hold  sync  %1 %2").arg(QLatin1String(DoorName(door)), QLatin1String(reason)));
}

void GsxDoorSync::RecordTarget(const GsxDoor door, const bool open)
{
    const auto index = static_cast<std::size_t>(door);

    lastTargets_[index] = open ? kDoorOpen : kDoorClosed;
    holdLogged_[index] = false;
    inheritedTargets_[index] = false;
}

void GsxDoorSync::RecordWrite(const GsxDoor door, const bool open)
{
    RecordTarget(door, open);
    transits_[static_cast<std::size_t>(door)].Begin();
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
    transits_ = {};

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
        gsx::lvars::kBaggageLoaderMainState,
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
        gsx::lvars::kSetDisableRearStairs,
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
        const DoorPosition position = PositionOf(door);
        if (position == DoorPosition::Closed && !IsInTransit(door, position))
        {
            probe::Line(probe::Channel::Writes,
                        QStringLiteral("skip  close %1 already closed").arg(QLatin1String(DoorName(door))));
            RecordTarget(door, false);

            continue;
        }

        probe::Line(probe::Channel::Writes,
                    QStringLiteral("write close %1 open=0").arg(QLatin1String(DoorName(door))));
        write(door, false);
        RecordWrite(door, false);
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
