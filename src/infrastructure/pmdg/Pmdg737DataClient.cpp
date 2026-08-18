#include "Pmdg737DataClient.h"

#include <cstring>
#include <string>
#include "../logging/LogMacros.h"

namespace
{
    constexpr auto kConnectionName = "GsxIntegratorPmdg737Data";

    constexpr SIMCONNECT_CLIENT_DATA_ID kDataAreaId = PMDG_NG3_DATA_ID;
    constexpr SIMCONNECT_CLIENT_DATA_DEFINITION_ID kDataDefId = PMDG_NG3_DATA_DEFINITION;
    constexpr SIMCONNECT_DATA_REQUEST_ID kDataRequestId = PMDG_NG3_DATA_DEFINITION;

    constexpr unsigned kThirdPartyEventBase = 69632;
    constexpr DWORD kMouseLeftSingle = 0x20000000;

    std::string EventName(const unsigned offset)
    {
        return "#" + std::to_string(kThirdPartyEventBase + offset);
    }
}

Pmdg737DataClient::Pmdg737DataClient() = default;

Pmdg737DataClient::~Pmdg737DataClient()
{
    session_.Close();
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

void Pmdg737DataClient::EnsureConnected()
{
    if (connected_)
    {
        return;
    }

    if (!session_.Open(kConnectionName))
    {
        return;
    }

    connected_ = session_.RequestClientDataArea(
        PMDG_NG3_DATA_NAME, kDataAreaId, kDataDefId, kDataRequestId,
        static_cast<DWORD>(sizeof(PMDG_NG3_Data)),
        SIMCONNECT_CLIENT_DATA_PERIOD_SECOND, SIMCONNECT_CLIENT_DATA_REQUEST_FLAG_DEFAULT,
        [this](const void* data, const DWORD size) { OnClientData(data, size); });

    if (!connected_)
    {
        session_.Close();
    }
}

void Pmdg737DataClient::OnClientData(const void* data, const DWORD size)
{
    if (data == nullptr || size < sizeof(PMDG_NG3_Data))
    {
        return;
    }

    std::memcpy(&data_, data, sizeof(PMDG_NG3_Data));
    if (data_.AircraftModel == 0 && data_.FUEL_QtyLeft <= 0.0f)
    {
        return;
    }

    if (!hasData_)
    {
        LOG_INFO("PMDG 737 ClientData received: model %d", data_.AircraftModel);
    }
    hasData_ = true;
}

void Pmdg737DataClient::Poll()
{
    EnsureConnected();
    session_.Dispatch();
}

bool Pmdg737DataClient::HasData() const
{
    return hasData_;
}

bool Pmdg737DataClient::GroundPowerAvailable() const
{
    return hasData_ && data_.ELEC_annunGRD_POWER_AVAILABLE;
}

bool Pmdg737DataClient::AnyMainBusPowered() const
{
    return hasData_ && (data_.ELEC_BusPowered[kAcMain1Bus] || data_.ELEC_BusPowered[kAcMain2Bus]);
}

bool Pmdg737DataClient::BeaconOn() const
{
    return hasData_ && data_.LTS_AntiCollisionSw;
}

bool Pmdg737DataClient::ParkingBrakeOn() const
{
    return hasData_ && data_.PED_annunParkingBrake;
}

void Pmdg737DataClient::ToggleDoor(const Pmdg737Door door)
{
    const unsigned offset = DoorEventOffsetFor(door);
    if (offset == 0)
    {
        return;
    }

    session_.TransmitEvent(EventName(offset).c_str(), kMouseLeftSingle);
}

void Pmdg737DataClient::SetInFlight(const bool inFlight)
{
    inFlight_ = inFlight;
}
