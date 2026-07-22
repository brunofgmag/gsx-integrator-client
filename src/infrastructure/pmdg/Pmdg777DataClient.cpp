#include "Pmdg777DataClient.h"

#include <chrono>
#include <cstring>
#include <string>
#include "../logging/LogMacros.h"

namespace
{
    constexpr auto kConnectionName = "GsxIntegratorPmdgData";

    constexpr SIMCONNECT_CLIENT_DATA_ID kDataAreaId = PMDG_777X_DATA_ID;
    constexpr SIMCONNECT_CLIENT_DATA_DEFINITION_ID kDataDefId = PMDG_777X_DATA_DEFINITION;
    constexpr SIMCONNECT_DATA_REQUEST_ID kDataRequestId = PMDG_777X_DATA_DEFINITION;

    constexpr unsigned kThirdPartyEventBase = 69632;
    constexpr unsigned kLightTestOffset = 118;
    constexpr DWORD kMouseLeftSingle = 0x20000000;
    constexpr DWORD kMouseWheelUp = 0x00004000;
    constexpr DWORD kMouseWheelDown = 0x00002000;

    unsigned DoorEventOffset(const int index)
    {
        return 14011 + static_cast<unsigned>(index);
    }

    std::string EventName(const unsigned offset)
    {
        return "#" + std::to_string(kThirdPartyEventBase + offset);
    }

    long long SteadyNowMs()
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    }
}

Pmdg777DataClient::Pmdg777DataClient()
    : nowMs_(&SteadyNowMs)
{
}

Pmdg777DataClient::~Pmdg777DataClient()
{
    session_.Close();
}

void Pmdg777DataClient::EnsureConnected()
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
        PMDG_777X_DATA_NAME, kDataAreaId, kDataDefId, kDataRequestId,
        static_cast<DWORD>(sizeof(PMDG_777X_Data)),
        SIMCONNECT_CLIENT_DATA_PERIOD_VISUAL_FRAME, SIMCONNECT_CLIENT_DATA_REQUEST_FLAG_CHANGED,
        [this](const void* data, const DWORD size) { OnClientData(data, size); });

    if (!connected_)
    {
        session_.Close();
    }
}

void Pmdg777DataClient::OnClientData(const void* data, const DWORD size)
{
    if (data == nullptr || size < sizeof(PMDG_777X_Data))
    {
        return;
    }

    std::memcpy(&data_, data, sizeof(PMDG_777X_Data));
    if (data_.AircraftModel == 0 && data_.FUEL_QtyLeft <= 0.0f)
    {
        return;
    }

    if (!hasData_)
    {
        LOG_INFO("PMDG 777 ClientData received: model %d", data_.AircraftModel);
    }
    hasData_ = true;
}

void Pmdg777DataClient::Poll()
{
    EnsureConnected();
    session_.Dispatch();

    if (pendingKickRelease_)
    {
        pendingKickRelease_ = false;
        session_.TransmitEvent(EventName(kLightTestOffset).c_str(), kMouseWheelDown);
    }

    if (hasData_ || inFlight_)
    {
        return;
    }

    if (nowMs_() - lastKickMs_ >= kKickIntervalMs)
    {
        KickDataRefresh();
    }
}

bool Pmdg777DataClient::HasData() const
{
    return hasData_;
}

int Pmdg777DataClient::AircraftModel() const
{
    return hasData_ ? static_cast<int>(data_.AircraftModel) : 0;
}

bool Pmdg777DataClient::ExtPowerConnected() const
{
    return hasData_ && (data_.ELEC_annunExtPowr_ON[0] || data_.ELEC_annunExtPowr_ON[1]);
}

bool Pmdg777DataClient::ExtPowerAvailable() const
{
    return hasData_ && (data_.ELEC_annunExtPowr_AVAIL[0] || data_.ELEC_annunExtPowr_AVAIL[1]);
}

bool Pmdg777DataClient::BeaconOn() const
{
    return hasData_ && data_.LTS_Beacon_Sw_ON;
}

bool Pmdg777DataClient::ParkingBrakeOn() const
{
    return hasData_ && data_.BRAKES_ParkingBrakeLeverOn;
}

bool Pmdg777DataClient::ApuRunning() const
{
    return hasData_ && data_.APURunning;
}

bool Pmdg777DataClient::WheelChocksSet() const
{
    return hasData_ && data_.WheelChocksSet;
}

bool Pmdg777DataClient::GroundConnAvailable() const
{
    return hasData_ && data_.GroundConnAvailable;
}

bool Pmdg777DataClient::IrsAligned() const
{
    return hasData_ && data_.IRS_aligned;
}

bool Pmdg777DataClient::HasFmcFlightPlan() const
{
    return hasData_ && (data_.FMC_CruiseAlt > 0 || data_.FMC_flightNumber[0] != '\0');
}

double Pmdg777DataClient::TotalFuelLbs() const
{
    if (!hasData_)
    {
        return 0.0;
    }

    return static_cast<double>(data_.FUEL_QtyCenter) + data_.FUEL_QtyLeft
        + data_.FUEL_QtyRight + data_.FUEL_QtyAux;
}

int Pmdg777DataClient::DoorState(const int index) const
{
    if (!hasData_ || index < 0 || index >= 16)
    {
        return -1;
    }

    return static_cast<int>(data_.DOOR_state[index]);
}

void Pmdg777DataClient::ToggleDoor(const int index)
{
    if (index < 0 || index >= 16)
    {
        return;
    }

    session_.TransmitEvent(EventName(DoorEventOffset(index)).c_str(), kMouseLeftSingle);
}

void Pmdg777DataClient::KickDataRefresh()
{
    lastKickMs_ = nowMs_();
    pendingKickRelease_ = true;
    session_.TransmitEvent(EventName(kLightTestOffset).c_str(), kMouseWheelUp);
}

void Pmdg777DataClient::SetInFlight(const bool inFlight)
{
    inFlight_ = inFlight;
}
