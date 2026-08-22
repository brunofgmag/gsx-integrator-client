#include "Pmdg737DataClient.h"
#include <QtCore/QString>
#include "../probe/ProbeLog.h"

namespace
{
    constexpr DWORD kMouseLeftSingle = 0x20000000;

    const PmdgClientDataSpec kChannelSpec{
        "GsxIntegratorPmdg737Data",
        PMDG_NG3_DATA_NAME,
        PMDG_NG3_DATA_ID,
        PMDG_NG3_DATA_DEFINITION,
        PMDG_NG3_DATA_DEFINITION,
        SIMCONNECT_CLIENT_DATA_PERIOD_SECOND,
        SIMCONNECT_CLIENT_DATA_REQUEST_FLAG_DEFAULT,
        "PMDG 737"
    };
}

Pmdg737DataClient::Pmdg737DataClient() : channel_(kChannelSpec)
{
}

unsigned Pmdg737DataClient::DoorEventOffsetFor(const Pmdg737Door door)
{
    switch (door)
    {
    case Pmdg737Door::FwdEntry: return 14005;
    case Pmdg737Door::FwdService: return 14006;
    case Pmdg737Door::AftEntry: return 14007;
    case Pmdg737Door::AftService: return 14008;
    case Pmdg737Door::FwdCargo: return 14013;
    case Pmdg737Door::AftCargo: return 14014;
    case Pmdg737Door::MainCargo: return 14015;
    case Pmdg737Door::EquipmentHatch: return 14016;
    case Pmdg737Door::Airstair: return 14017;
    default: return 0;
    }
}

void Pmdg737DataClient::Poll()
{
    channel_.Poll();
    ReportProbe();
    MaybeProbeToggle();
}

void Pmdg737DataClient::MaybeProbeToggle()
{
    if (probeToggleSent_ || !probe::IsOn() || !channel_.HasData() || !AnyMainBusPowered())
    {
        return;
    }

    const int offset = qEnvironmentVariableIntValue("GSXI_PROBE_TOGGLE");
    if (offset <= 0)
    {
        return;
    }

    probeToggleSent_ = true;
    probe::Line(QStringLiteral("probe pmdg-737 sending SDK event offset=%1").arg(offset));
    channel_.TransmitEvent(static_cast<unsigned>(offset), kMouseLeftSingle);
}

void Pmdg737DataClient::ReportProbe() const
{
    if (!probe::IsOn() || !channel_.HasData())
    {
        return;
    }

    const PMDG_NG3_Data& data = channel_.Data();
    probe::Change("pmdg737.doors",
                  QStringLiteral("sdk   pmdg-737 doors fwdEntry=%1 fwdService=%2 airstair=%3 "
                                 "fwdOverwingL=%4 fwdOverwingR=%5 fwdCargo=%6 equip=%7 "
                                 "aftOverwingL=%8 aftOverwingR=%9 aftCargo=%10 aftEntry=%11 aftService=%12")
                  .arg(data.DOOR_annunFWD_ENTRY).arg(data.DOOR_annunFWD_SERVICE)
                  .arg(data.DOOR_annunAIRSTAIR)
                  .arg(data.DOOR_annunLEFT_FWD_OVERWING).arg(data.DOOR_annunRIGHT_FWD_OVERWING)
                  .arg(data.DOOR_annunFWD_CARGO).arg(data.DOOR_annunEQUIP)
                  .arg(data.DOOR_annunLEFT_AFT_OVERWING).arg(data.DOOR_annunRIGHT_AFT_OVERWING)
                  .arg(data.DOOR_annunAFT_CARGO).arg(data.DOOR_annunAFT_ENTRY)
                  .arg(data.DOOR_annunAFT_SERVICE));

    probe::Change("pmdg737.hyd",
                  QStringLiteral("sdk   pmdg-737 hyd pumpEng=[%1,%2] pumpElec=[%3,%4] "
                                 "lowPressEng=[%5,%6] lowPressElec=[%7,%8] acMain=[%9,%10] gpu=%11 brake=%12")
                  .arg(data.HYD_PumpSw_eng[0]).arg(data.HYD_PumpSw_eng[1])
                  .arg(data.HYD_PumpSw_elec[0]).arg(data.HYD_PumpSw_elec[1])
                  .arg(data.HYD_annunLOW_PRESS_eng[0]).arg(data.HYD_annunLOW_PRESS_eng[1])
                  .arg(data.HYD_annunLOW_PRESS_elec[0]).arg(data.HYD_annunLOW_PRESS_elec[1])
                  .arg(data.ELEC_BusPowered[kAcMain1Bus]).arg(data.ELEC_BusPowered[kAcMain2Bus])
                  .arg(data.ELEC_annunGRD_POWER_AVAILABLE).arg(data.PED_annunParkingBrake));
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
    const unsigned offset = DoorEventOffsetFor(door);
    if (offset == 0)
    {
        return;
    }

    channel_.TransmitEvent(offset, kMouseLeftSingle);
}

void Pmdg737DataClient::SetInFlight(const bool inFlight)
{
    channel_.SetInFlight(inFlight);
}
