#include "Pmdg777DataClient.h"

#include <chrono>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include "../probe/ProbeLog.h"

namespace
{
    constexpr unsigned kLightTestOffset = 118;
    constexpr DWORD kMouseLeftSingle = 0x20000000;
    constexpr DWORD kMouseWheelUp = 0x00004000;
    constexpr DWORD kMouseWheelDown = 0x00002000;

    constexpr int kDoorCount = 16;

    const PmdgClientDataSpec kChannelSpec{
        "GsxIntegratorPmdgData",
        PMDG_777X_DATA_NAME,
        PMDG_777X_DATA_ID,
        PMDG_777X_DATA_DEFINITION,
        PMDG_777X_DATA_DEFINITION,
        SIMCONNECT_CLIENT_DATA_PERIOD_SECOND,
        SIMCONNECT_CLIENT_DATA_REQUEST_FLAG_DEFAULT,
        "PMDG 777"
    };

    unsigned DoorEventOffset(const int index)
    {
        return 14011 + static_cast<unsigned>(index);
    }

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
        channel_.TransmitEvent(kLightTestOffset, kMouseWheelDown);
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

    channel_.TransmitEvent(DoorEventOffset(index), kMouseLeftSingle);
}

void Pmdg777DataClient::KickDataRefresh()
{
    probe::Line(QStringLiteral("probe pmdg-777 kicking light test, block stale for %1 ms")
                .arg(nowMs_() - lastKickMs_.value_or(nowMs_())));
    lastKickMs_ = nowMs_();
    pendingKickRelease_ = true;
    channel_.TransmitEvent(kLightTestOffset, kMouseWheelUp);
}

void Pmdg777DataClient::SetInFlight(const bool inFlight)
{
    channel_.SetInFlight(inFlight);
}

void Pmdg777DataClient::MaybeProbeToggle()
{
    if (probeToggleSent_ || !probe::IsOn() || !channel_.HasData())
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
    probe::Line(QStringLiteral("probe pmdg-777 toggling door slot=%1 event=%2 was=%3")
                .arg(slot)
                .arg(DoorEventOffset(slot))
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
    for (int index = 0; index < kDoorCount; ++index)
    {
        states.append(QString::number(channel_.Data().DOOR_state[index]));
    }

    probe::Change("pmdg777.doors",
                  QStringLiteral("sdk   pmdg-777 DOOR_state=[%1] cockpit=%2 chocks=%3 brake=%4")
                  .arg(states.join(QLatin1Char(',')))
                  .arg(channel_.Data().DOOR_CockpitDoorOpen)
                  .arg(channel_.Data().WheelChocksSet)
                  .arg(ParkingBrakeOn()));
}
