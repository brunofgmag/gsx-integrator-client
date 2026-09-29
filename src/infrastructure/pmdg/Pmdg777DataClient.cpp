#include "Pmdg777DataClient.h"

#include <array>
#include <chrono>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include "../probe/ProbeLog.h"

namespace
{
    constexpr unsigned kLightTestOffset = 118;
    constexpr auto kLightTestLabel = "LIGHT_TEST";
    constexpr DWORD kMouseLeftSingle = 0x20000000;
    constexpr DWORD kMouseWheelUp = 0x00004000;
    constexpr DWORD kMouseWheelDown = 0x00002000;

    constexpr int kDoorCount = 16;
    constexpr int kKickLogEvery = 10;

    int Flag(const bool value)
    {
        return value ? 1 : 0;
    }

    const PmdgClientDataSpec kChannelSpec{
        .connectionName = "GsxIntegratorPmdgData",
        .areaName = PMDG_777X_DATA_NAME,
        .areaId = PMDG_777X_DATA_ID,
        .definitionId = PMDG_777X_DATA_DEFINITION,
        .requestId = PMDG_777X_DATA_DEFINITION,
        .period = SIMCONNECT_CLIENT_DATA_PERIOD_SECOND,
        .requestFlag = SIMCONNECT_CLIENT_DATA_REQUEST_FLAG_DEFAULT,
        .label = "PMDG 777"
    };

    struct DoorEvent
    {
        unsigned offset;
        const char* label;
    };

    constexpr std::array<DoorEvent, kDoorCount> kDoorEvents = {{
        {.offset = 14011, .label = "DOOR_ENTRY_1L"},
        {.offset = 14012, .label = "DOOR_ENTRY_1R"},
        {.offset = 14013, .label = "DOOR_ENTRY_2L"},
        {.offset = 14014, .label = "DOOR_ENTRY_2R"},
        {.offset = 14015, .label = "DOOR_ENTRY_3L"},
        {.offset = 14016, .label = "DOOR_ENTRY_3R"},
        {.offset = 14017, .label = "DOOR_ENTRY_4L"},
        {.offset = 14018, .label = "DOOR_ENTRY_4R"},
        {.offset = 14019, .label = "DOOR_ENTRY_5L"},
        {.offset = 14020, .label = "DOOR_ENTRY_5R"},
        {.offset = 14021, .label = "DOOR_CARGO_FWD"},
        {.offset = 14022, .label = "DOOR_CARGO_AFT"},
        {.offset = 14023, .label = "DOOR_CARGO_MAIN"},
        {.offset = 14024, .label = "DOOR_CARGO_BULK"},
        {.offset = 14025, .label = "DOOR_AVIONICS_ACCESS"},
        {.offset = 14026, .label = "DOOR_EE_ACCESS"}
    }};

    long long SteadyNowMs()
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    }
}

Pmdg777DataClient::Pmdg777DataClient()
    : channel_(kChannelSpec),
      nowMs_(&SteadyNowMs)
{
}

void Pmdg777DataClient::Poll()
{
    channel_.Poll();
    ReportProbe();
    MaybeProbeToggle();

    if (pendingKickRelease_)
    {
        pendingKickRelease_ = false;
        channel_.TransmitEvent(kLightTestOffset, kMouseWheelDown, kLightTestLabel);
    }

    if (channel_.HasData() || channel_.InFlight())
    {
        return;
    }

    if (!lastKickMs_.has_value())
    {
        lastKickMs_ = nowMs_();
    }

    if (nowMs_() - *lastKickMs_ >= kKickIntervalMs)
    {
        KickDataRefresh();
    }
}

bool Pmdg777DataClient::HasData() const
{
    return channel_.HasData();
}

bool Pmdg777DataClient::ExtPowerConnected() const
{
    return channel_.HasData()
        && (channel_.Data().ELEC_annunExtPowr_ON[0] || channel_.Data().ELEC_annunExtPowr_ON[1]);
}

bool Pmdg777DataClient::ExtPowerAvailable() const
{
    return channel_.HasData()
        && (channel_.Data().ELEC_annunExtPowr_AVAIL[0] || channel_.Data().ELEC_annunExtPowr_AVAIL[1]);
}

bool Pmdg777DataClient::BeaconOn() const
{
    return channel_.HasData() && channel_.Data().LTS_Beacon_Sw_ON;
}

bool Pmdg777DataClient::ParkingBrakeOn() const
{
    return channel_.HasData() && channel_.Data().BRAKES_ParkingBrakeLeverOn;
}

bool Pmdg777DataClient::ApuRunning() const
{
    return channel_.HasData() && channel_.Data().APURunning;
}

bool Pmdg777DataClient::WheelChocksSet() const
{
    return channel_.HasData() && channel_.Data().WheelChocksSet;
}

bool Pmdg777DataClient::HasFmcFlightPlan() const
{
    return channel_.HasData()
        && (channel_.Data().FMC_CruiseAlt > 0 || channel_.Data().FMC_flightNumber[0] != '\0');
}

int Pmdg777DataClient::DoorState(const int index) const
{
    if (!channel_.HasData() || index < 0 || index >= kDoorCount)
    {
        return -1;
    }

    return static_cast<int>(channel_.Data().DOOR_state[index]);
}

void Pmdg777DataClient::ToggleDoor(const int index)
{
    if (index < 0 || index >= kDoorCount)
    {
        return;
    }

    const DoorEvent& event = kDoorEvents[index];

    channel_.TransmitEvent(event.offset, kMouseLeftSingle, event.label);
}

void Pmdg777DataClient::KickDataRefresh()
{
    ++kickCount_;
    probe::Change(probe::Channel::Writes, "pmdg777.kick",
                  QString::number(kickCount_ / kKickLogEvery),
                  QStringLiteral("probe pmdg-777 kicking light test, block stale, kicks=%1").arg(kickCount_));
    lastKickMs_ = nowMs_();
    pendingKickRelease_ = true;
    channel_.TransmitEvent(kLightTestOffset, kMouseWheelUp, kLightTestLabel);
}

void Pmdg777DataClient::SetInFlight(const bool inFlight)
{
    channel_.SetInFlight(inFlight);
}

void Pmdg777DataClient::MaybeProbeToggle()
{
    if (probeToggleSent_ || !probe::ActsOnTheSim() || !channel_.HasData())
    {
        return;
    }

    if (!ExtPowerConnected() && !ApuRunning())
    {
        return;
    }

    bool ok = false;
    const int slot = qEnvironmentVariableIntValue("GSXI_PROBE_DOOR", &ok);
    if (!ok || slot < 0 || slot >= kDoorCount)
    {
        return;
    }

    probeToggleSent_ = true;
    probe::Line(probe::Channel::Writes, QStringLiteral("probe pmdg-777 toggling door slot=%1 event=%2 was=%3")
                .arg(slot)
                .arg(kDoorEvents[slot].offset)
                .arg(DoorState(slot)));
    ToggleDoor(slot);
}

void Pmdg777DataClient::ReportProbe() const
{
    if (!probe::IsOn() || !channel_.HasData())
    {
        return;
    }

    QStringList states;
    for (const unsigned char state : channel_.Data().DOOR_state)
    {
        states.append(QString::number(state));
    }

    const PMDG_777X_Data& data = channel_.Data();
    probe::Change(probe::Channel::AircraftVendor, "pmdg777.doors",
                  QStringLiteral("sdk   pmdg-777 DOOR_state=[%1] cockpit=%2 chocks=%3 brake=%4 "
                                 "extAvail=[%5,%6] extOn=[%7,%8]")
                  .arg(states.join(QLatin1Char(',')))
                  .arg(Flag(data.DOOR_CockpitDoorOpen))
                  .arg(Flag(data.WheelChocksSet))
                  .arg(Flag(ParkingBrakeOn()))
                  .arg(Flag(data.ELEC_annunExtPowr_AVAIL[0])).arg(Flag(data.ELEC_annunExtPowr_AVAIL[1]))
                  .arg(Flag(data.ELEC_annunExtPowr_ON[0])).arg(Flag(data.ELEC_annunExtPowr_ON[1])));
}
