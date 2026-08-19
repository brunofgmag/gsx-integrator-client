#include "Pmdg737DataClient.h"

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
