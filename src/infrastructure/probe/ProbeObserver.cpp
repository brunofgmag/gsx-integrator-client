#include "ProbeObserver.h"

#include <array>
#include <QtCore/QDateTime>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include "ProbeLog.h"
#include "../simvars/SimVars.h"
#include "../simvars/VariableGateway.h"
#include "../../domain/ports/Aircraft.h"

using namespace simvars;

namespace
{
    constexpr int kObserveIntervalMs = 1000;

    struct ProbeVar
    {
        const char* label;
        const char* name;
    };

    struct ProbeAVar
    {
        const char* name;
        const char* unit;
    };

    constexpr std::array kMd11StateDoors = {
        ProbeVar{"pax1L", "MD11_EXT_DOOR_PAX_1L"},
        ProbeVar{"pax2L", "MD11_EXT_DOOR_PAX_2L"},
        ProbeVar{"pax4L", "MD11_EXT_DOOR_PAX_4L"},
        ProbeVar{"cargo1R", "MD11_EXT_DOOR_CARGO_1R"},
        ProbeVar{"cargo2R", "MD11_EXT_DOOR_CARGO_2R"},
        ProbeVar{"cargoMain", "MD11_EXT_DOOR_CARGO_MAIN"},
        ProbeVar{"cmdPax1L", "MD11_EXT_DOOR_CMD_PAX_1L"},
        ProbeVar{"cmdPax2L", "MD11_EXT_DOOR_CMD_PAX_2L"},
        ProbeVar{"cmdPax4L", "MD11_EXT_DOOR_CMD_PAX_4L"},
        ProbeVar{"cmdCargo1R", "MD11_EXT_DOOR_CMD_CARGO_1R"},
        ProbeVar{"cmdCargo2R", "MD11_EXT_DOOR_CMD_CARGO_2R"},
        ProbeVar{"cmdCargoMain", "MD11_EXT_DOOR_CMD_CARGO_MAIN"},
        ProbeVar{"cargoMainArm", "MD11_EXT_DOOR_CRG_MAIN_ARM_SW"},
        ProbeVar{"cargoMainOpen", "MD11_EXT_DOOR_CRG_MAIN_OPEN_SW"},
        ProbeVar{"pax1R", "MD11_EXT_DOOR_PAX_1R"},
        ProbeVar{"pax2R", "MD11_EXT_DOOR_PAX_2R"},
        ProbeVar{"pax3L", "MD11_EXT_DOOR_PAX_3L"},
        ProbeVar{"pax3R", "MD11_EXT_DOOR_PAX_3R"},
        ProbeVar{"pax4R", "MD11_EXT_DOOR_PAX_4R"},
        ProbeVar{"cargoBulk", "MD11_EXT_DOOR_CARGO_BULK"},
        ProbeVar{"cmdCargoBulk", "MD11_EXT_DOOR_CMD_CARGO_BULK"},
        ProbeVar{"cargoMainLower", "md11_ext_door_cargo_main"},
        ProbeVar{"cmdCargoMainLower", "md11_ext_door_cmd_cargo_main"}
    };

    constexpr std::array kTolissDoors = {
        ProbeVar{"cargoFwd", "TLS_CARGO_DOOR_MODE_FWD"},
        ProbeVar{"cargoAft", "TLS_CARGO_DOOR_MODE_AFT"},
        ProbeVar{"pax1L", "TLS_PAX_DOOR_MODE_1L"},
        ProbeVar{"pax2L", "TLS_PAX_DOOR_MODE_2L"},
        ProbeVar{"pax3L", "TLS_PAX_DOOR_MODE_3L"},
        ProbeVar{"pax4L", "TLS_PAX_DOOR_MODE_4L"},
        ProbeVar{"pax1R", "TLS_PAX_DOOR_MODE_1R"},
        ProbeVar{"pax2R", "TLS_PAX_DOOR_MODE_2R"},
        ProbeVar{"pax3R", "TLS_PAX_DOOR_MODE_3R"},
        ProbeVar{"pax4R", "TLS_PAX_DOOR_MODE_4R"},
        ProbeVar{"cargoFwdRatio", "TLS_CARGO_DOOR_OPEN_RATIO_FWD"},
        ProbeVar{"cargoAftRatio", "TLS_CARGO_DOOR_OPEN_RATIO_AFT"},
        ProbeVar{"pax1LRatio", "TLS_PAX_DOOR_OPEN_RATIO_1L"},
        ProbeVar{"pax2LRatio", "TLS_PAX_DOOR_OPEN_RATIO_2L"},
        ProbeVar{"pax3LRatio", "TLS_PAX_DOOR_OPEN_RATIO_3L"},
        ProbeVar{"pax4LRatio", "TLS_PAX_DOOR_OPEN_RATIO_4L"},
        ProbeVar{"pax1RRatio", "TLS_PAX_DOOR_OPEN_RATIO_1R"},
        ProbeVar{"pax2RRatio", "TLS_PAX_DOOR_OPEN_RATIO_2R"},
        ProbeVar{"pax3RRatio", "TLS_PAX_DOOR_OPEN_RATIO_3R"},
        ProbeVar{"pax4RRatio", "TLS_PAX_DOOR_OPEN_RATIO_4R"}
    };

    constexpr std::array kIflyDoors = {
        ProbeVar{"cargoFwd", "Animation_FWD_Cargo_VAL"},
        ProbeVar{"cargoAft", "Animation_AFT_Cargo_VAL"}
    };

    constexpr std::array kIflyPaxCandidates = {
        ProbeVar{"fwdEntry", "ANIMATION_FWD_ENTRY_VAL"},
        ProbeVar{"fwdService", "ANIMATION_FWD_SERVICE_VAL"},
        ProbeVar{"aftEntry", "ANIMATION_AFT_ENTRY_VAL"},
        ProbeVar{"aftService", "ANIMATION_AFT_SERVICE_VAL"},
        ProbeVar{"lFwdOverwing", "ANIMATION_L_FWD_OVERWING_VAL"},
        ProbeVar{"rFwdOverwing", "ANIMATION_R_FWD_OVERWING_VAL"},
        ProbeVar{"lAftOverwing", "ANIMATION_L_AFT_OVERWING_VAL"},
        ProbeVar{"rAftOverwing", "ANIMATION_R_AFT_OVERWING_VAL"},
        ProbeVar{"doorflagLFwd", "ANIMATION_DOORFLAG_L_FWD_VAL"},
        ProbeVar{"doorflagLAft", "ANIMATION_DOORFLAG_L_AFT_VAL"},
        ProbeVar{"doorflagRFwd", "ANIMATION_DOORFLAG_R_FWD_VAL"},
        ProbeVar{"doorflagRAft", "ANIMATION_DOORFLAG_R_AFT_VAL"},
        ProbeVar{"cargoFwdUpper", "ANIMATION_FWD_CARGO_VAL"},
        ProbeVar{"cargoAftUpper", "ANIMATION_AFT_CARGO_VAL"},
        ProbeVar{"cargoFwdAlt", "ANIMATION_CARGO_FWD_VAL"},
        ProbeVar{"cargoAftAlt", "ANIMATION_CARGO_AFT_VAL"}
    };

    constexpr std::array kPmdg737Doors = {
        ProbeVar{"mainCargo", "MainCargoDoor"}
    };

    constexpr std::array kSimExits = {
        ProbeAVar{"EXIT OPEN:0", "percent"},
        ProbeAVar{"EXIT OPEN:1", "percent"},
        ProbeAVar{"EXIT OPEN:2", "percent"},
        ProbeAVar{"EXIT OPEN:3", "percent"},
        ProbeAVar{"EXIT OPEN:4", "percent"},
        ProbeAVar{"EXIT OPEN:5", "percent"}
    };

    struct ProbeProfile
    {
        const char* brakeLVar = nullptr;
        const char* chocksLVar = nullptr;
        const ProbeVar* doors = nullptr;
        std::size_t doorCount = 0;
        const ProbeVar* candidates = nullptr;
        std::size_t candidateCount = 0;
    };

    bool StartsWith(const std::string& value, const char* prefix)
    {
        return value.rfind(prefix, 0) == 0;
    }

    ProbeProfile ProfileFor(const std::string& profileId)
    {
        if (StartsWith(profileId, "tfdi-md11"))
        {
            return {
                "MD11_THR_PARK_LVR", "MD11_EXT_CHOCKS",
                kMd11StateDoors.data(), kMd11StateDoors.size(), nullptr, 0
            };
        }

        if (StartsWith(profileId, "toliss"))
        {
            return {
                "PARKINGBRAKE_POSITION", nullptr,
                kTolissDoors.data(), kTolissDoors.size(), nullptr, 0
            };
        }

        if (StartsWith(profileId, "ifly"))
        {
            return {
                "VC_Parking_Brake_SW_VAL", "iFly_NLG_Chock_Display_VAL",
                kIflyDoors.data(), kIflyDoors.size(),
                kIflyPaxCandidates.data(), kIflyPaxCandidates.size()
            };
        }

        if (StartsWith(profileId, "fenix"))
        {
            return {"S_MIP_PARKING_BRAKE", "B_CONFIG_CHOCKS", nullptr, 0, nullptr, 0};
        }

        if (StartsWith(profileId, "pmdg-737"))
        {
            return {
                nullptr, "NGXWheelChocks",
                kPmdg737Doors.data(), kPmdg737Doors.size(), nullptr, 0
            };
        }

        return {};
    }

    const char* StatusText(const DoorStatus status)
    {
        switch (status)
        {
        case DoorStatus::AnyOpen: return "AnyOpen";
        case DoorStatus::AllClosed: return "AllClosed";
        default: return "Unknown";
        }
    }

    QString Number(const double value)
    {
        return QString::number(value, 'f', 3);
    }
}

const ProbeObserver::Track& ProbeObserver::Follow(VariableGateway& variables, const char* name)
{
    const double value = variables.GetLVar(name, 0.0);
    Track& track = tracks_[name];
    if (!track.seen)
    {
        track.seen = true;
        track.min = value;
        track.max = value;
    }
    else
    {
        track.min = value < track.min ? value : track.min;
        track.max = value > track.max ? value : track.max;
    }

    return track;
}

void ProbeObserver::Observe(const Aircraft& aircraft, VariableGateway& variables,
                            const std::string& profileId)
{
    if (!probe::IsOn())
    {
        return;
    }

    const long long now = QDateTime::currentMSecsSinceEpoch();
    if (now - lastObservedMs_ < kObserveIntervalMs)
    {
        return;
    }
    lastObservedMs_ = now;

    const ProbeProfile profile = ProfileFor(profileId);
    const QString id = QString::fromStdString(profileId);

    char title[256] = {};
    char atcModel[256] = {};
    variables.FetchAircraftName(title, sizeof title);
    variables.FetchAtcModel(atcModel, sizeof atcModel);
    probe::Change("identity", QStringLiteral("plane %1 title='%2' atcModel='%3'")
                  .arg(id, QString::fromLatin1(title), QString::fromLatin1(atcModel)));

    probe::Change("door", QStringLiteral("door  %1 GetDoorStatus=%2 IsParkingBrakeSet=%3")
                  .arg(id, QLatin1String(StatusText(aircraft.GetDoorStatus())))
                  .arg(aircraft.IsParkingBrakeSet() ? 1 : 0));

    if (profile.brakeLVar != nullptr)
    {
        const double vendor = variables.GetLVar(profile.brakeLVar, 0.0);
        const double sim = variables.GetAVar(kSimParkingBrake, kBoolUnit, 0.0);
        probe::Change("brake", QStringLiteral("brake %1 %2=%3 sim=%4 and=%5 or=%6")
                      .arg(id, QLatin1String(profile.brakeLVar), Number(vendor), Number(sim))
                      .arg(vendor > 0.0 && sim > 0.0 ? 1 : 0)
                      .arg(vendor > 0.0 || sim > 0.0 ? 1 : 0));
    }

    if (profile.chocksLVar != nullptr)
    {
        probe::Change("chocks", QStringLiteral("chock %1 %2=%3")
                      .arg(id, QLatin1String(profile.chocksLVar),
                           Number(variables.GetLVar(profile.chocksLVar, 0.0))));
    }

    for (std::size_t i = 0; i < profile.doorCount; ++i)
    {
        const ProbeVar& door = profile.doors[i];
        const Track& track = Follow(variables, door.name);
        probe::Change(std::string("door.") + door.name,
                      QStringLiteral("var   %1 %2 %3=%4 recv=%5 span=[%6..%7]")
                      .arg(id, QLatin1String(door.label), QLatin1String(door.name),
                           Number(variables.GetLVar(door.name, 0.0)))
                      .arg(variables.HasReceivedLVar(door.name) ? 1 : 0)
                      .arg(Number(track.min), Number(track.max)));
    }

    int flat = 0;
    for (std::size_t i = 0; i < profile.candidateCount; ++i)
    {
        const ProbeVar& candidate = profile.candidates[i];
        const Track& track = Follow(variables, candidate.name);
        if (track.max <= track.min)
        {
            ++flat;

            continue;
        }

        probe::Change(std::string("cand.") + candidate.name,
                      QStringLiteral("cand  %1 %2=%3 span=[%4..%5] MOVED")
                      .arg(id, QLatin1String(candidate.name),
                           Number(variables.GetLVar(candidate.name, 0.0)),
                           Number(track.min), Number(track.max)));
    }

    if (profile.candidateCount > 0)
    {
        probe::Change("cand.flat", QStringLiteral("cand  %1 flat=%2 of %3")
                      .arg(id).arg(flat).arg(profile.candidateCount));
    }

    QStringList exits;
    for (const ProbeAVar& exit : kSimExits)
    {
        const double value = variables.GetAVar(exit.name, exit.unit, -1.0);
        exits.append(QStringLiteral("%1=%2").arg(QLatin1String(exit.name), Number(value)));
    }
    probe::Change("exits", QStringLiteral("exit  %1 %2").arg(id, exits.join(QLatin1Char(' '))));
}
