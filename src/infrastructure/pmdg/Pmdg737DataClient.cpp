#include "Pmdg737DataClient.h"
#include <array>
#include <QtCore/QString>
#include "../probe/ProbeLog.h"

namespace
{
    constexpr DWORD kMouseLeftSingle = 0x20000000;

    constexpr auto kProbeToggleLabel = "PROBE_TOGGLE";

    int Flag(const bool value)
    {
        return value ? 1 : 0;
    }

    struct DoorEvent
    {
        Pmdg737Door door;
        unsigned offset;
        const char* label;
    };

    constexpr std::array kDoorEvents = {
        DoorEvent{.door = Pmdg737Door::FwdEntry, .offset = 14005, .label = "DOOR_FWD_ENTRY"},
        DoorEvent{.door = Pmdg737Door::FwdService, .offset = 14006, .label = "DOOR_FWD_SERVICE"},
        DoorEvent{.door = Pmdg737Door::AftEntry, .offset = 14007, .label = "DOOR_AFT_ENTRY"},
        DoorEvent{.door = Pmdg737Door::AftService, .offset = 14008, .label = "DOOR_AFT_SERVICE"},
        DoorEvent{.door = Pmdg737Door::FwdCargo, .offset = 14013, .label = "DOOR_FWD_CARGO"},
        DoorEvent{.door = Pmdg737Door::AftCargo, .offset = 14014, .label = "DOOR_AFT_CARGO"},
        DoorEvent{.door = Pmdg737Door::MainCargo, .offset = 14015, .label = "DOOR_MAIN_CARGO"},
        DoorEvent{.door = Pmdg737Door::EquipmentHatch, .offset = 14016, .label = "DOOR_EQUIPMENT_HATCH"},
        DoorEvent{.door = Pmdg737Door::Airstair, .offset = 14017, .label = "DOOR_AIRSTAIR"}
    };

    const DoorEvent* FindDoorEvent(const Pmdg737Door door)
    {
        for (const DoorEvent& event : kDoorEvents)
        {
            if (event.door == door)
            {
                return &event;
            }
        }

        return nullptr;
    }

    const PmdgClientDataSpec kChannelSpec{
        .connectionName = "GsxIntegratorPmdg737Data",
        .areaName = PMDG_NG3_DATA_NAME,
        .areaId = PMDG_NG3_DATA_ID,
        .definitionId = PMDG_NG3_DATA_DEFINITION,
        .requestId = PMDG_NG3_DATA_DEFINITION,
        .period = SIMCONNECT_CLIENT_DATA_PERIOD_SECOND,
        .requestFlag = SIMCONNECT_CLIENT_DATA_REQUEST_FLAG_DEFAULT,
        .label = "PMDG 737"
    };
}

Pmdg737DataClient::Pmdg737DataClient() : channel_(kChannelSpec)
{
}

unsigned Pmdg737DataClient::DoorEventOffsetFor(const Pmdg737Door door)
{
    const DoorEvent* event = FindDoorEvent(door);

    return event != nullptr ? event->offset : 0;
}

void Pmdg737DataClient::Poll()
{
    channel_.Poll();
    ReportProbe();
    MaybeProbeToggle();
}

void Pmdg737DataClient::MaybeProbeToggle()
{
    if (probeToggleSent_ || !probe::ActsOnTheSim() || !channel_.HasData() || !AnyMainBusPowered())
    {
        return;
    }

    const int offset = qEnvironmentVariableIntValue("GSXI_PROBE_TOGGLE");
    if (offset <= 0)
    {
        return;
    }

    probeToggleSent_ = true;
    probe::Line(probe::Channel::Writes, QStringLiteral("probe pmdg-737 sending SDK event offset=%1").arg(offset));
    channel_.TransmitEvent(static_cast<unsigned>(offset), kMouseLeftSingle, kProbeToggleLabel);
}

void Pmdg737DataClient::ReportProbe() const
{
    if (!probe::IsOn() || !channel_.HasData())
    {
        return;
    }

    const PMDG_NG3_Data& data = channel_.Data();
    probe::Change(probe::Channel::AircraftVendor, "pmdg737.doors",
                  QStringLiteral("sdk   pmdg-737 doors fwdEntry=%1 fwdService=%2 airstair=%3 "
                                 "fwdOverwingL=%4 fwdOverwingR=%5 fwdCargo=%6 equip=%7 "
                                 "aftOverwingL=%8 aftOverwingR=%9 aftCargo=%10 aftEntry=%11 aftService=%12")
                  .arg(Flag(data.DOOR_annunFWD_ENTRY)).arg(Flag(data.DOOR_annunFWD_SERVICE))
                  .arg(Flag(data.DOOR_annunAIRSTAIR))
                  .arg(Flag(data.DOOR_annunLEFT_FWD_OVERWING)).arg(Flag(data.DOOR_annunRIGHT_FWD_OVERWING))
                  .arg(Flag(data.DOOR_annunFWD_CARGO)).arg(Flag(data.DOOR_annunEQUIP))
                  .arg(Flag(data.DOOR_annunLEFT_AFT_OVERWING)).arg(Flag(data.DOOR_annunRIGHT_AFT_OVERWING))
                  .arg(Flag(data.DOOR_annunAFT_CARGO)).arg(Flag(data.DOOR_annunAFT_ENTRY))
                  .arg(Flag(data.DOOR_annunAFT_SERVICE)));

    probe::Change(probe::Channel::AircraftVendor, "pmdg737.hyd",
                  QStringLiteral("sdk   pmdg-737 hyd pumpEng=[%1,%2] pumpElec=[%3,%4] "
                                 "lowPressEng=[%5,%6] lowPressElec=[%7,%8] acMain=[%9,%10] gpu=%11 brake=%12")
                  .arg(Flag(data.HYD_PumpSw_eng[0])).arg(Flag(data.HYD_PumpSw_eng[1]))
                  .arg(Flag(data.HYD_PumpSw_elec[0])).arg(Flag(data.HYD_PumpSw_elec[1]))
                  .arg(Flag(data.HYD_annunLOW_PRESS_eng[0])).arg(Flag(data.HYD_annunLOW_PRESS_eng[1]))
                  .arg(Flag(data.HYD_annunLOW_PRESS_elec[0])).arg(Flag(data.HYD_annunLOW_PRESS_elec[1]))
                  .arg(Flag(data.ELEC_BusPowered[kAcMain1Bus])).arg(Flag(data.ELEC_BusPowered[kAcMain2Bus]))
                  .arg(Flag(data.ELEC_annunGRD_POWER_AVAILABLE)).arg(Flag(data.PED_annunParkingBrake)));
}

bool Pmdg737DataClient::HasData() const
{
    return channel_.HasData();
}

bool Pmdg737DataClient::GroundPowerAvailable() const
{
    return channel_.HasData() && channel_.Data().ELEC_annunGRD_POWER_AVAILABLE;
}

bool Pmdg737DataClient::AnyMainBusPowered() const
{
    return channel_.HasData()
        && (channel_.Data().ELEC_BusPowered[kAcMain1Bus] || channel_.Data().ELEC_BusPowered[kAcMain2Bus]);
}

bool Pmdg737DataClient::BeaconOn() const
{
    return channel_.HasData() && channel_.Data().LTS_AntiCollisionSw;
}

bool Pmdg737DataClient::AirstairAnnunciator() const
{
    return channel_.HasData() && channel_.Data().DOOR_annunAIRSTAIR;
}

bool Pmdg737DataClient::ParkingBrakeOn() const
{
    return channel_.HasData() && channel_.Data().PED_annunParkingBrake;
}

void Pmdg737DataClient::ToggleDoor(const Pmdg737Door door)
{
    const DoorEvent* event = FindDoorEvent(door);
    if (event == nullptr)
    {
        return;
    }

    channel_.TransmitEvent(event->offset, kMouseLeftSingle, event->label);
}

void Pmdg737DataClient::SetInFlight(const bool inFlight)
{
    channel_.SetInFlight(inFlight);
}
